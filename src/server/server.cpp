/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:15 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/17 21:57:39 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "client.hpp"

Server::Server() {
  Logger::log("Creating default server on port 8080");

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    Logger::error("socket() failed: " + std::string(strerror(errno)));
    throw std::runtime_error(
        "There is a problem in the Server->socket function");
  }
  Logger::debug("socket() created FD: " + Helpers::toString(server_fd));

  /*
  When a server closes a connection, the port it was using enters a TIME_WAIT
  state for a brief period to ensure all packets on the network have cleared
  out. By default, attempting to bind() to that same port immediately results in
  an "Address already in use" error. Enabling SO_REUSEADDR explicitly tells the
  operating system that it is safe to reuse the address even if it's technically
  still in that waiting state.
  */
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

  // we have two categories of flags associated with file handling :
  // File descriptor flags : associated with single specific FD eithing a
  // process's FD table File status  flags : associated to the open file
  // description Here the fctl is setting the file status flags  behaviour to be
  // none blocking.
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
  // SO_REUSEPORT for kernel-level load balancing
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0)
    Logger::warn("setsockopt(SO_REUSEPORT) failed (non-fatal): " +
                 std::string(strerror(errno)));
  // TCP_NODELAY to disable Nagle's algorithm
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

  // Multi-accept loop: drain all pending connections in one epoll wakeup
  while (true) {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int client_fd = accept(server_fd, (sockaddr *)&clientAddr, &clientLen);

    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break; // No more pending connections
      if (errno == EINTR)
        continue; // Interrupted, retry
      Logger::error("accept() failed: " + std::string(strerror(errno)));
      break;
    }
    if (Logger::isDebugEnabled())
      Logger::debug("accept() returned client FD: " +
                    Helpers::toString(client_fd));

    // Make non-blocking
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
      Logger::error("fcntl(O_NONBLOCK) failed for client FD: " +
                    Helpers::toString(client_fd));
      close(client_fd);
      continue;
    }

    // Enable TCP_NODELAY on client socket
    int yes = 1;
    if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) <
        0) {
      if (Logger::isDebugEnabled())
        Logger::debug("TCP_NODELAY failed for client FD: " +
                      Helpers::toString(client_fd));
    }

    clients[client_fd] = new Client(client_fd);

    // Edge-triggered epoll with EPOLLRDHUP for efficient event handling
    EpollManager::getInstance().add(client_fd, EPOLLIN | EPOLLRDHUP | EPOLLET);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, INET_ADDRSTRLEN);
    Logger::info("Client connected: " + std::string(ip) + ":" +
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
  char buf[16384]; // 16KB stack buffer for efficiency

  // Read loop - drain until EAGAIN (edge-triggered)
  while (true) {
    ssize_t n = read(fd, buf, sizeof(buf));

    if (n > 0) {
      c->request.addData(buf, n); // Zero-copy overload
      if (Logger::isDebugEnabled())
        Logger::debug("Added " + Helpers::toString(n) +
                      " bytes to request buffer");

      // Pipelining loop: process all complete requests
      while (c->request.isComplete() && !c->request.hasError()) {
        if (Logger::isDebugEnabled())
          Logger::debug("Request complete on FD: " + Helpers::toString(fd));

        // Process this request
        RequestHandler handler(config);
        handler.process(c->request, c->response);

        // Append response to write buffer (don't overwrite previous responses)
        std::string res = c->response.toString();
        c->writeBuffer.insert(c->writeBuffer.end(), res.begin(), res.end());
        c->wantWrite = true;

        // Check Connection: close header
        std::string conn = c->request.getHeader("connection");
        if (conn == "close") {
          c->closeAfterWrite = true;
        }

        // Reset for next request (preserves leftover data in buffer)
        c->request.reset();
        c->response = HttpResponse(); // Reset response for next request
      }

      if (c->request.hasError())
        break;

    } else if (n == 0) {
      Logger::info("Client FD: " + Helpers::toString(fd) +
                   " disconnected (read 0 bytes)");
      closeClient(fd);
      return;
    } else {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break; // No more data available
      Logger::error("read() failed on FD: " + Helpers::toString(fd) + ": " +
                    std::string(strerror(errno)));
      closeClient(fd);
      return;
    }
  }

  if (c->request.hasError()) {
    Logger::warn("Request has error, closing client FD: " +
                 Helpers::toString(fd));
    closeClient(fd);
    return;
  }

  // If we have data to write, switch to EPOLLOUT
  if (c->wantWrite && !c->writeBuffer.empty()) {
    EpollManager::getInstance().mod(fd, EPOLLOUT | EPOLLRDHUP | EPOLLET);
    if (Logger::isDebugEnabled())
      Logger::debug("Modified FD: " + Helpers::toString(fd) +
                    " to EPOLLOUT for writing");
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

  // Drain write buffer until EAGAIN (edge-triggered requires full drain)
  while (c->writeOffset < total) {
    ssize_t n =
        write(fd, &c->writeBuffer[0] + c->writeOffset, total - c->writeOffset);
    if (n > 0) {
      c->writeOffset += n;
      if (Logger::isDebugEnabled())
        Logger::debug("write() sent " + Helpers::toString(n) +
                      " bytes on FD: " + Helpers::toString(fd));
    } else if (n == 0) {
      Logger::warn("write() returned 0 on FD: " + Helpers::toString(fd) +
                   ", closing connection");
      closeClient(fd);
      return;
    } else if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break; // Socket buffer full, wait for next EPOLLOUT
      if (errno == EINTR)
        continue; // Interrupted, retry
      Logger::error("write() failed on FD: " + Helpers::toString(fd));
      closeClient(fd);
      return;
    }
  }

  if (c->writeOffset >= total) {
    // Buffer fully sent - reset for next response
    c->writeBuffer.clear();
    c->writeOffset = 0;
    c->wantWrite = false;

    if (c->closeAfterWrite) {
      Logger::info("Connection: close - closing FD: " + Helpers::toString(fd));
      closeClient(fd);
      return;
    }

    EpollManager::getInstance().mod(fd, EPOLLIN | EPOLLRDHUP | EPOLLET);
    Logger::info("Response fully sent on FD: " + Helpers::toString(fd));
  } else {
    // Still have data, keep EPOLLOUT
    EpollManager::getInstance().mod(fd, EPOLLOUT | EPOLLRDHUP | EPOLLET);
    if (Logger::isDebugEnabled())
      Logger::debug("Partial write, " +
                    Helpers::toString(total - c->writeOffset) +
                    " bytes remaining");
  }
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
    delete clients[fd];
    clients.erase(fd);
    Logger::info("Client FD: " + Helpers::toString(fd) +
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
