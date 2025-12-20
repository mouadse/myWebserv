#include "CgiHandler.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

CgiHandler::CgiHandler(const HTTPRequest &request,
                       const std::string &scriptPath,
                       const std::string &scriptName,
                       const std::string &interpreter)
    : _request(request), _scriptPath(scriptPath), _scriptName(scriptName),
      _interpreter(interpreter) {
  _initEnv();
}

CgiHandler::~CgiHandler() {}

std::string CgiHandler::_getInterpreter() const {
  if (!_interpreter.empty())
    return _interpreter;

  size_t dotPos = _scriptPath.find_last_of('.');
  if (dotPos == std::string::npos)
    return "";

  std::string ext = _scriptPath.substr(dotPos);
  if (ext == ".py")
    return "/usr/bin/python3";
  if (ext == ".pl")
    return "/usr/bin/perl";
  if (ext == ".php")
    return "/usr/bin/php";
  if (ext == ".rb")
    return "/usr/bin/ruby";
  if (ext == ".sh")
    return "/bin/bash";
  return "";
}

void CgiHandler::_initEnv() {
  _env["GATEWAY_INTERFACE"] = "CGI/1.1";
  _env["SERVER_PROTOCOL"] = "HTTP/1.1";
  _env["SERVER_SOFTWARE"] = "SimpleWebServer/1.0";

  _env["REQUEST_METHOD"] = _request.getMethod();
  std::string requestUri = _request.getTarget();
  const std::string queryString = _request.getQueryString();
  if (!queryString.empty())
    requestUri += "?" + queryString;
  _env["REQUEST_URI"] = requestUri;
  _env["SCRIPT_FILENAME"] = _scriptPath;
  _env["SCRIPT_NAME"] = _scriptName;
  _env["QUERY_STRING"] = queryString;

  if (_request.hasBody()) {
    std::stringstream ss;
    ss << _request.getBody().size();
    _env["CONTENT_LENGTH"] = ss.str();
    _env["CONTENT_TYPE"] = _request.getHeader("Content-Type");
  }

  _env["HTTP_COOKIE"] = _request.getHeader("Cookie");
  _env["HTTP_USER_AGENT"] = _request.getHeader("User-Agent");
  _env["HTTP_ACCEPT"] = _request.getHeader("Accept");
  _env["HTTP_HOST"] = _request.getHeader("Host");
}

char **CgiHandler::_createEnvArray() const {
  std::vector<std::string> envStrings;
  envStrings.reserve(_env.size());
  for (std::map<std::string, std::string>::const_iterator it = _env.begin();
       it != _env.end(); ++it) {
    envStrings.push_back(it->first + "=" + it->second);
  }

  char **env = new char *[envStrings.size() + 1];
  int i = 0;
  try {
    for (i = 0; i < (int)envStrings.size(); ++i) {
      env[i] = new char[envStrings[i].size() + 1];
      std::strcpy(env[i], envStrings[i].c_str());
    }
  } catch (...) {
    for (int j = 0; j < i; ++j) {
      delete[] env[j];
    }
    delete[] env;
    throw;
  }
  env[i] = NULL;
  return env;
}

void CgiHandler::_freeEnvArray(char **env) const {
  for (int i = 0; env[i] != NULL; ++i) {
    delete[] env[i];
  }
  delete[] env;
}

void CgiHandler::execute(HttpResponse &response) {
  int pipeIn[2];
  int pipeOut[2];

  if (pipe(pipeIn) < 0) {
    response.setStatus(500, "Internal Server Error");
    return;
  }
  if (pipe(pipeOut) < 0) {
    close(pipeIn[0]);
    close(pipeIn[1]);
    response.setStatus(500, "Internal Server Error");
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);
    response.setStatus(500, "Internal Server Error");
    return;
  }

  if (pid == 0) {
    dup2(pipeIn[0], STDIN_FILENO);
    dup2(pipeOut[1], STDOUT_FILENO);

    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);

    char **env = _createEnvArray();

    std::string interpreter = _getInterpreter();
    const char *argv[3];

    if (!interpreter.empty()) {
      argv[0] = interpreter.c_str();
      argv[1] = _scriptPath.c_str();
      argv[2] = NULL;
      execve(interpreter.c_str(), const_cast<char *const *>(argv), env);
    } else {
      argv[0] = _scriptPath.c_str();
      argv[1] = NULL;
      execve(_scriptPath.c_str(), const_cast<char *const *>(argv), env);
    }

    exit(1);
  }

  close(pipeIn[0]);
  close(pipeOut[1]);

  std::vector<char> body = _request.getBody();
  size_t written = 0;
  std::vector<char> outputBuffer;
  char buffer[4096];

  struct pollfd pfd[2];
  pfd[0].fd = pipeIn[1];
  pfd[0].events = POLLOUT;
  pfd[1].fd = pipeOut[0];
  pfd[1].events = POLLIN;

  bool inputClosed = false;
  bool timedOut = false;

  struct timeval start, now;
  gettimeofday(&start, NULL);

  while (true) {
    gettimeofday(&now, NULL);
    long elapsed = (now.tv_sec - start.tv_sec) * 1000 +
                   (now.tv_usec - start.tv_usec) / 1000;

    if (elapsed >= 5000) {
      kill(pid, SIGKILL);
      timedOut = true;
      break;
    }

    int ret = poll(pfd, 2, 5000 - elapsed);

    if (ret < 0)
      break;
    if (ret == 0) {
      kill(pid, SIGKILL);
      timedOut = true;
      break;
    }

    if (!inputClosed && (pfd[0].revents & POLLOUT)) {
      if (written < body.size()) {
        const char *dataPtr = body.empty() ? NULL : &body[0];
        ssize_t w = write(pipeIn[1], dataPtr + written, body.size() - written);
        if (w > 0)
          written += w;
        else if (w < 0)
          inputClosed = true;
      } else {
        close(pipeIn[1]);
        pfd[0].fd = -1;
        inputClosed = true;
      }
    } else if (!inputClosed && (pfd[0].revents & (POLLERR | POLLHUP))) {
      close(pipeIn[1]);
      pfd[0].fd = -1;
      inputClosed = true;
    }

    if (pfd[1].revents & POLLIN) {
      ssize_t r = read(pipeOut[0], buffer, sizeof(buffer));
      if (r > 0) {
        outputBuffer.insert(outputBuffer.end(), buffer, buffer + r);
      } else {
        break;
      }
    } else if (pfd[1].revents & (POLLHUP | POLLERR)) {
      break;
    }

    if (inputClosed && (pfd[1].revents & POLLIN) == 0 &&
        (pfd[1].revents & POLLHUP))
      break;
  }

  if (!inputClosed)
    close(pipeIn[1]);
  close(pipeOut[0]);

  int status;
  waitpid(pid, &status, 0);

  if (timedOut) {
    response.setStatus(504, "Gateway Timeout");
    return;
  }
  if (WIFSIGNALED(status) || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    response.setStatus(502, "Bad Gateway");
    return;
  }

  std::string rawOutput(outputBuffer.begin(), outputBuffer.end());
  size_t headerEnd = rawOutput.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    headerEnd = rawOutput.find("\n\n");
  }

  std::string bodyContent;
  bool statusSet = false;
  if (headerEnd != std::string::npos) {
    std::string headers = rawOutput.substr(0, headerEnd);
    if (rawOutput[headerEnd] == '\r')
      bodyContent = rawOutput.substr(headerEnd + 4);
    else
      bodyContent = rawOutput.substr(headerEnd + 2);

    std::stringstream hs(headers);
    std::string line;
    while (std::getline(hs, line)) {
      if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
      size_t colon = line.find(':');
      if (colon != std::string::npos) {
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);

        while (!val.empty() && val[0] == ' ')
          val.erase(0, 1);

        if (key == "Status") {
          std::stringstream vs(val);
          int code;
          std::string msg;
          vs >> code;
          std::getline(vs, msg);
          if (!msg.empty() && msg[0] == ' ')
            msg.erase(0, 1);
          response.setStatus(code, msg);
          statusSet = true;
        } else {
          response.setHeader(key, val);
        }
      }
    }
  } else {
    bodyContent = rawOutput;
  }

  if (!statusSet && response.hasHeader("Location"))
    response.setStatus(302, "Found");

  response.setBody(bodyContent);
}
