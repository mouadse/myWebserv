/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:15 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/20 21:53:56 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "../cgi/CgiHandler.hpp"
#include "client.hpp"
#include <signal.h>
#include <sstream>
#include <sys/time.h>
#include <sys/wait.h>

namespace {
void setHtmlError(HttpResponse &response, int code, const std::string &message,
                  const std::string &detail) {
  response.setStatus(code, message);
  std::stringstream body;
  body << "<html><head><title>" << code << " " << message << "</title></head>"
       << "<body><h1>" << code << " " << message << "</h1>";
  if (!detail.empty()) {
    body << "<p>" << detail << "</p>";
  }
  body << "</body></html>";
  response.setBody(body.str());
  response.setHeader("Content-Type", "text/html");
}

bool startFileStream(Client *c, const HttpResponse &response) {
  if (c == NULL)
    return false;
  int fd = open(response.getFilePath().c_str(), O_RDONLY);
  if (fd < 0)
    return false;
  if (response.getFileOffset() > 0) {
    if (lseek(fd, response.getFileOffset(), SEEK_SET) < 0) {
      close(fd);
      return false;
    }
  }
  c->streamFd = fd;
  c->streamRemaining = response.getFileLength();
  c->streamBufferOffset = 0;
  c->streamBufferSize = 0;
  c->streaming = true;
  return true;
}

void stopFileStream(Client *c) {
  if (c->streamFd >= 0)
    close(c->streamFd);
  c->streamFd = -1;
  c->streamRemaining = 0;
  c->streamBufferOffset = 0;
  c->streamBufferSize = 0;
  c->streaming = false;
}

void logWriteFailure(Client *c, int fd, const char *callName) {
  if (c != NULL && c->fileResponse) {
    if (Logger::isDebugEnabled())
      Logger::debug(std::string(callName) +
                    "() failed on FD: " + Helpers::toString(fd) +
                    " Error: " + std::string(strerror(errno)));
  } else {
    Logger::error(std::string(callName) +
                  "() failed on FD: " + Helpers::toString(fd) +
                  " Error: " + std::string(strerror(errno)));
  }
}

const long long CGI_TIMEOUT_MS = 5000;

long long nowMs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<long long>(tv.tv_sec) * 1000 +
         static_cast<long long>(tv.tv_usec) / 1000;
}

void removeCgiFd(Server &server, int &fd) {
  if (fd < 0)
    return;
  try {
    EpollManager::getInstance().remove(fd);
  } catch (...) {
  }
  close(fd);
  server.cgiFds.erase(fd);
  fd = -1;
}

void closeCgiInput(Server &server, Client *c) {
  if (c == NULL)
    return;
  removeCgiFd(server, c->cgiInFd);
  c->cgiInputClosed = true;
}

void closeCgiOutput(Server &server, Client *c) {
  if (c == NULL)
    return;
  removeCgiFd(server, c->cgiOutFd);
  c->cgiOutputClosed = true;
}

void resetCgiState(Client *c) {
  if (c == NULL)
    return;
  c->cgiActive = false;
  c->cgiPid = -1;
  c->cgiInFd = -1;
  c->cgiOutFd = -1;
  c->cgiInput.clear();
  c->cgiInputOffset = 0;
  c->cgiOutput.clear();
  c->cgiInputClosed = false;
  c->cgiOutputClosed = false;
  c->cgiExited = false;
  c->cgiExitStatus = 0;
  c->cgiTimedOut = false;
  c->cgiStripBody = false;
  c->cgiStartMs = 0;
}

void updateCgiExitStatus(Client *c) {
  if (c == NULL || c->cgiExited || c->cgiPid <= 0)
    return;
  int status = 0;
  pid_t ret = waitpid(c->cgiPid, &status, WNOHANG);
  if (ret == c->cgiPid) {
    c->cgiExited = true;
    c->cgiExitStatus = status;
  }
}

bool cgiDone(Client *c) {
  if (c == NULL)
    return false;
  return c->cgiOutputClosed && c->cgiExited;
}

void finalizeCgi(Server &server, Client *c) {
  if (c == NULL || !c->cgiActive)
    return;

  if (c->cgiTimedOut) {
    c->response.setStatus(504, "Gateway Timeout");
  } else if (!c->cgiExited || WIFSIGNALED(c->cgiExitStatus) ||
             !WIFEXITED(c->cgiExitStatus) ||
             WEXITSTATUS(c->cgiExitStatus) != 0) {
    c->response.setStatus(502, "Bad Gateway");
  } else {
    CgiHandler::parseOutput(c->cgiOutput, c->response);
  }

  if (c->cgiStripBody) {
    std::string originalLength;
    try {
      originalLength = c->response.getHeader("Content-Length");
    } catch (...) {
      originalLength.clear();
    }
    c->response.setBody("");
    if (!originalLength.empty())
      c->response.setHeader("Content-Length", originalLength);
  }

  c->response.toBuffer(c->writeBuffer);
  c->wantWrite = true;

  try {
    EpollManager::getInstance().mod(c->fd, EPOLLOUT | EPOLLRDHUP);
  } catch (...) {
  }

  closeCgiInput(server, c);
  closeCgiOutput(server, c);
  resetCgiState(c);
  c->request.reset();
  c->response = HttpResponse();
}

bool startCgi(Server &server, Client *c, const CgiTask &task) {
  if (c == NULL)
    return false;

  int inFd = -1;
  int outFd = -1;
  pid_t pid = -1;
  CgiHandler handler(c->request, task.scriptPath, task.scriptName,
                     task.interpreter);

  if (!handler.spawn(inFd, outFd, pid))
    return false;

  c->cgiActive = true;
  c->cgiPid = pid;
  c->cgiInFd = inFd;
  c->cgiOutFd = outFd;
  c->cgiInput = c->request.getBody();
  c->cgiInputOffset = 0;
  c->cgiOutput.clear();
  c->cgiInputClosed = false;
  c->cgiOutputClosed = false;
  c->cgiExited = false;
  c->cgiExitStatus = 0;
  c->cgiTimedOut = false;
  c->cgiStripBody = task.stripBody;
  c->cgiStartMs = nowMs();

  try {
    if (!c->cgiInput.empty()) {
      EpollManager::getInstance().add(inFd, EPOLLOUT);
      server.cgiFds[inFd] = c;
    } else {
      close(inFd);
      c->cgiInFd = -1;
      c->cgiInputClosed = true;
    }

    EpollManager::getInstance().add(outFd, EPOLLIN);
    server.cgiFds[outFd] = c;
  } catch (...) {
    kill(pid, SIGKILL);
    updateCgiExitStatus(c);
    closeCgiInput(server, c);
    closeCgiOutput(server, c);
    resetCgiState(c);
    return false;
  }

  try {
    EpollManager::getInstance().mod(c->fd, EPOLLRDHUP);
  } catch (...) {
  }

  return true;
}
} // namespace

Server::Server() {
  Logger::log("Creating default server on port 8080");

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    Logger::error("socket() failed: " + std::string(strerror(errno)));
    throw std::runtime_error(
        "There is a problem in the Server->socket function");
  }
  Logger::debug("socket() created FD: " + Helpers::toString(server_fd));

  int yes = 1;
  int setSock =
      setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if (setSock < 0) {
    Logger::error("setsockopt(SO_REUSEADDR) failed: " +
                  std::string(strerror(errno)));
    throw std::runtime_error(
        "There is a problem in the Server->SetSocket function");
  }
  Logger::debug("SO_REUSEADDR enabled for FD: " + Helpers::toString(server_fd));

  sockaddr_in addr = {};
  addr.sin_port = htons(8080);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    Logger::error("bind() to 0.0.0.0:8080 failed: " +
                  std::string(strerror(errno)));
    throw std::runtime_error("There is a problem in the Server->Bind function");
  }
  Logger::info("Server bound to 0.0.0.0:8080 (FD: " +
               Helpers::toString(server_fd) + ")");

  if (listen(server_fd, 128) < 0) {
    Logger::error("listen() failed: " + std::string(strerror(errno)));
    throw std::runtime_error(
        "There is a problem in the Server->Listen function");
  }
  Logger::debug("listen() called with backlog 128");

  if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
    Logger::error("fcntl(O_NONBLOCK) failed: " + std::string(strerror(errno)));
    throw std::runtime_error(
        "There is a problem in the Server->Fcntl1 function");
  }
  Logger::debug("FD: " + Helpers::toString(server_fd) +
                " set to non-blocking mode");

  EpollManager::getInstance().add(server_fd, EPOLLIN);
  Logger::debug("Server FD: " + Helpers::toString(server_fd) +
                " added to epoll with EPOLLIN");
  Logger::info("Default server initialized successfully");
}

Server::Server(const WebserverConfig &cfg)
    : _port(cfg.getPort()), _host(cfg.getHost()), config(cfg) {

  Logger::debug("Creating server with port: " + Helpers::toString(_port) +
                ", host: " + Helpers::toString(_host));
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    Logger::error("socket() failed: " + std::string(strerror(errno)));
    throw std::runtime_error(
        "There is a problem in the Server->socket function");
  }
  Logger::debug("socket() created FD: " + Helpers::toString(server_fd));

  int yes = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    Logger::error("setsockopt(SO_REUSEADDR) failed: " +
                  std::string(strerror(errno)));
    throw std::runtime_error(
        "There is a problem in the Server->SetSocket function");
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0)
    Logger::warn("setsockopt(SO_REUSEPORT) failed (non-fatal): " +
                 std::string(strerror(errno)));
  if (setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) < 0)
    Logger::warn("setsockopt(TCP_NODELAY) failed (non-fatal): " +
                 std::string(strerror(errno)));
  Logger::debug("Socket options set for FD: " + Helpers::toString(server_fd));

  sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(_port);
  addr.sin_addr.s_addr = _host;

  if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    Logger::error("bind() failed: " + std::string(strerror(errno)));
    throw std::runtime_error("There is a problem in the Server->Bind function");
  }
  if (listen(server_fd, 128) < 0) {
    Logger::error("listen() failed: " + std::string(strerror(errno)));
    throw std::runtime_error(
        "There is a problem in the Server->Listen function");
  }
  if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
    Logger::error("fcntl(O_NONBLOCK) failed: " + std::string(strerror(errno)));
    throw std::runtime_error(
        "There is a problem in the Server->Fcntl1 function");
  }
  Logger::debug("FD: " + Helpers::toString(server_fd) +
                " set to non-blocking mode");

  EpollManager::getInstance().add(server_fd, EPOLLIN);

  Logger::info("[Server] Listening on " + cfg.getHostString() + ":" +
               Helpers::toString(_port) + "\n");
}

void Server::acceptClient() {
  if (Logger::isDebugEnabled())
    Logger::debug("acceptClient() called on server FD: " +
                  Helpers::toString(server_fd));

  while (true) {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int client_fd = accept(server_fd, (sockaddr *)&clientAddr, &clientLen);

    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      if (errno == EINTR)
        continue;
      Logger::error("accept() failed: " + std::string(strerror(errno)));
      break;
    }
    if (Logger::isDebugEnabled())
      Logger::debug("accept() returned client FD: " +
                    Helpers::toString(client_fd));

    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
      Logger::error("fcntl(O_NONBLOCK) failed for client FD: " +
                    Helpers::toString(client_fd));
      close(client_fd);
      continue;
    }

    int yes = 1;
    if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) <
        0) {
      if (Logger::isDebugEnabled())
        Logger::debug("TCP_NODELAY failed for client FD: " +
                      Helpers::toString(client_fd));
    }

    clients[client_fd] = new Client(client_fd);

    EpollManager::getInstance().add(client_fd, EPOLLIN | EPOLLRDHUP);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, INET_ADDRSTRLEN);
    if (Logger::isDebugEnabled())
      Logger::debug("Client connected: " + std::string(ip) + ":" +
                    Helpers::toString(ntohs(clientAddr.sin_port)) +
                    " (FD: " + Helpers::toString(client_fd) + ")");
  }
  if (Logger::isDebugEnabled())
    Logger::debug("Total clients connected: " +
                  Helpers::toString(clients.size()));
}

void Server::readClient(int fd) {
  std::map<int, Client *>::iterator it = clients.find(fd);
  if (it == clients.end() || it->second == NULL) {
    Logger::error("readClient() called for unknown or null FD: " +
                  Helpers::toString(fd));
    return;
  }
  Client *c = it->second;
  if (c->cgiActive)
    return;
  char buf[16384];

  ssize_t n = read(fd, buf, sizeof(buf));

  if (n > 0) {
    c->request.addData(buf, n);
    if (Logger::isDebugEnabled())
      Logger::debug("Added " + Helpers::toString(n) +
                    " bytes to request buffer");
    while (c->request.isComplete() && !c->request.hasError()) {
      if (Logger::isDebugEnabled())
        Logger::debug("Request complete on FD: " + Helpers::toString(fd));
      RequestHandler handler(config);
      CgiTask cgiTask;
      bool cgiPending = handler.process(c->request, c->response, cgiTask);
      if (cgiPending) {
        if (!startCgi(*this, c, cgiTask)) {
          c->response.setStatus(500, "Internal Server Error");
        } else {
          std::string conn = c->request.getHeader("connection");
          if (conn == "close")
            c->closeAfterWrite = true;
          c->request.reset();
          break;
        }
      }

      if (c->response.hasFileBody() && c->response.getFileLength() > 0) {
        if (!startFileStream(c, c->response)) {
          setHtmlError(c->response, 500, "Internal Server Error",
                       "Could not open file.");
        }
      }
      bool isFile = c->streaming;
      if (!isFile && c->response.hasHeader("Accept-Ranges"))
        isFile = true;
      if (isFile)
        c->fileResponse = true;
      c->response.toBuffer(c->writeBuffer);
      c->wantWrite = true;
      std::string conn = c->request.getHeader("connection");
      if (conn == "close") {
        c->closeAfterWrite = true;
      }
      c->request.reset();
      c->response = HttpResponse();
    }

    if (c->request.hasError()) {
      Logger::warn("Request has error, closing client FD: " +
                   Helpers::toString(fd));
      closeClient(fd);
      return;
    }
    if (c->wantWrite && !c->writeBuffer.empty()) {
      EpollManager::getInstance().mod(fd, EPOLLOUT | EPOLLRDHUP);
      if (Logger::isDebugEnabled())
        Logger::debug("Modified FD: " + Helpers::toString(fd) +
                      " to EPOLLOUT for writing");
    }

  } else if (n == 0) {
    if (Logger::isDebugEnabled())
      Logger::debug("Client FD: " + Helpers::toString(fd) +
                    " disconnected (read 0 bytes)");
    closeClient(fd);
  } else {
    Logger::error("read() failed on FD: " + Helpers::toString(fd) +
                  " Error: " + std::string(strerror(errno)));
    closeClient(fd);
  }
}

bool Server::hasCgiFd(int fd) const {
  return cgiFds.find(fd) != cgiFds.end();
}

void Server::handleCgiEvent(int fd, uint32_t events) {
  std::map<int, Client *>::iterator it = cgiFds.find(fd);
  if (it == cgiFds.end() || it->second == NULL)
    return;

  Client *c = it->second;
  if (!c->cgiActive)
    return;

  if (fd == c->cgiInFd) {
    if (events & (EPOLLERR | EPOLLHUP)) {
      closeCgiInput(*this, c);
    } else if (events & EPOLLOUT) {
      if (c->cgiInputOffset < c->cgiInput.size()) {
        const char *dataPtr = c->cgiInput.empty() ? NULL : &c->cgiInput[0];
        ssize_t w = write(c->cgiInFd, dataPtr + c->cgiInputOffset,
                          c->cgiInput.size() - c->cgiInputOffset);
        if (w > 0) {
          c->cgiInputOffset += static_cast<size_t>(w);
        } else {
          closeCgiInput(*this, c);
        }
      }
      if (c->cgiInputOffset >= c->cgiInput.size())
        closeCgiInput(*this, c);
    }
  }

  if (fd == c->cgiOutFd) {
    if (events & EPOLLIN) {
      char buffer[4096];
      ssize_t r = read(c->cgiOutFd, buffer, sizeof(buffer));
      if (r > 0) {
        c->cgiOutput.insert(c->cgiOutput.end(), buffer, buffer + r);
      } else if (r == 0) {
        closeCgiOutput(*this, c);
      } else {
        closeCgiOutput(*this, c);
      }
    }
    if (events & (EPOLLHUP | EPOLLERR))
      closeCgiOutput(*this, c);
  }

  updateCgiExitStatus(c);
  if (cgiDone(c))
    finalizeCgi(*this, c);
}

int Server::nextCgiTimeoutMs() const {
  long long now = nowMs();
  long long minRemaining = -1;

  for (std::map<int, Client *>::const_iterator it = clients.begin();
       it != clients.end(); ++it) {
    Client *c = it->second;
    if (c == NULL || !c->cgiActive)
      continue;
    long long elapsed = now - c->cgiStartMs;
    long long remaining = CGI_TIMEOUT_MS - elapsed;
    if (remaining < 0)
      remaining = 0;
    if (minRemaining < 0 || remaining < minRemaining)
      minRemaining = remaining;
  }

  if (minRemaining < 0)
    return -1;
  return static_cast<int>(minRemaining);
}

void Server::checkCgiTimeouts() {
  long long now = nowMs();

  for (std::map<int, Client *>::iterator it = clients.begin();
       it != clients.end(); ++it) {
    Client *c = it->second;
    if (c == NULL || !c->cgiActive)
      continue;

    if (!c->cgiTimedOut && now - c->cgiStartMs >= CGI_TIMEOUT_MS) {
      c->cgiTimedOut = true;
      if (c->cgiPid > 0)
        kill(c->cgiPid, SIGKILL);
    }

    updateCgiExitStatus(c);

    if (c->cgiTimedOut) {
      finalizeCgi(*this, c);
    } else if (cgiDone(c)) {
      finalizeCgi(*this, c);
    }
  }
}

void Server::writeClient(int fd) {
  if (Logger::isDebugEnabled())
    Logger::debug("writeClient() called for FD: " + Helpers::toString(fd));

  std::map<int, Client *>::iterator it = clients.find(fd);
  if (it == clients.end() || it->second == NULL) {
    Logger::error("writeClient() called for unknown or null FD: " +
                  Helpers::toString(fd));
    return;
  }
  Client *c = it->second;
  size_t total = c->writeBuffer.size();

  if (c->writeOffset < total) {
    ssize_t n = send(fd, &c->writeBuffer[0] + c->writeOffset,
                     total - c->writeOffset, MSG_NOSIGNAL);
    if (n > 0) {
      c->writeOffset += n;
      if (Logger::isDebugEnabled())
        Logger::debug("send() sent " + Helpers::toString(n) +
                      " bytes on FD: " + Helpers::toString(fd));
    } else if (n == 0) {
      Logger::warn("send() returned 0 on FD: " + Helpers::toString(fd) +
                   ", closing connection");
      closeClient(fd);
      return;
    } else {
      logWriteFailure(c, fd, "send");
      closeClient(fd);
      return;
    }
  }

  if (c->writeOffset < total) {
    return;
  }

  if (total > 0) {
    c->writeBuffer.clear();
    c->writeOffset = 0;

    if (c->streaming) {
      return;
    }
  }
  if (c->streaming) {
    if (c->streamBufferOffset >= c->streamBufferSize) {
      if (c->streamRemaining == 0) {
        stopFileStream(c);
      } else {
        size_t toRead = c->streamBuffer.size();
        if (toRead > c->streamRemaining)
          toRead = c->streamRemaining;
        ssize_t r = read(c->streamFd, &c->streamBuffer[0], toRead);
        if (r > 0) {
          c->streamBufferSize = static_cast<size_t>(r);
          c->streamBufferOffset = 0;
        } else if (r == 0) {
          Logger::warn("read() returned 0 on file FD: " +
                       Helpers::toString(c->streamFd));
          closeClient(fd);
          return;
        } else {
          Logger::error("read() failed on file FD: " +
                        Helpers::toString(c->streamFd));
          closeClient(fd);
          return;
        }
      }
    }
    if (c->streaming && c->streamBufferSize > 0) {
      ssize_t n =
          send(fd, &c->streamBuffer[0] + c->streamBufferOffset,
               c->streamBufferSize - c->streamBufferOffset, MSG_NOSIGNAL);
      if (n > 0) {
        c->streamBufferOffset += n;
        c->streamRemaining -= static_cast<size_t>(n);
      } else if (n == 0) {
        Logger::warn("send() returned 0 on FD: " + Helpers::toString(fd) +
                     ", closing connection");
        closeClient(fd);
        return;
      } else {
        logWriteFailure(c, fd, "send");
        closeClient(fd);
        return;
      }
      if (c->streamBufferOffset < c->streamBufferSize) {
        return;
      }
    }
    if (c->streaming) {
      return;
    }
  }
  c->wantWrite = false;
  c->fileResponse = false;
  if (c->closeAfterWrite) {
    if (Logger::isDebugEnabled())
      Logger::debug("Connection: close - closing FD: " + Helpers::toString(fd));
    closeClient(fd);
    return;
  }

  EpollManager::getInstance().mod(fd, EPOLLIN | EPOLLRDHUP);
  if (Logger::isDebugEnabled())
    Logger::debug("Response fully sent on FD: " + Helpers::toString(fd));
}

void Server::closeClient(int fd) {
  if (Logger::isDebugEnabled())
    Logger::debug("closeClient() called for FD: " + Helpers::toString(fd));

  try {
    EpollManager::getInstance().remove(fd);
    if (Logger::isDebugEnabled())
      Logger::debug("FD: " + Helpers::toString(fd) + " removed from epoll");
  } catch (const std::exception &e) {
    Logger::warn("Failed to remove FD from epoll during closeClient(): " +
                 Helpers::toString(fd) + " (" + e.what() + ")");
  }

  close(fd);
  if (Logger::isDebugEnabled())
    Logger::debug("close() called on FD: " + Helpers::toString(fd));

  if (clients.count(fd)) {
    Client *c = clients[fd];
    if (c->streamFd >= 0)
      stopFileStream(c);
    if (c->cgiPid > 0)
      kill(c->cgiPid, SIGKILL);
    updateCgiExitStatus(c);
    closeCgiInput(*this, c);
    closeCgiOutput(*this, c);
    resetCgiState(c);
    delete c;
    clients.erase(fd);
    if (Logger::isDebugEnabled())
      Logger::debug("Client FD: " + Helpers::toString(fd) +
                    " closed and cleaned up");
    if (Logger::isDebugEnabled())
      Logger::debug("Remaining clients: " + Helpers::toString(clients.size()));
  } else {
    Logger::warn("closeClient() called for non-existent FD: " +
                 Helpers::toString(fd));
  }
}

bool Server::hasClient(int fd) const {
  bool exists = clients.find(fd) != clients.end();
  if (Logger::isDebugEnabled())
    Logger::debug("hasClient(FD: " + Helpers::toString(fd) +
                  ") returns: " + (exists ? "true" : "false"));
  return exists;
}
