#include "CgiHandler.hpp"
#include <cstring>
#include <fcntl.h>
#include <sstream>
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

bool CgiHandler::spawn(int &inWriteFd, int &outReadFd, pid_t &pid) const {
  int pipeIn[2];
  int pipeOut[2];

  inWriteFd = -1;
  outReadFd = -1;
  pid = -1;

  if (pipe(pipeIn) < 0)
    return false;
  if (pipe(pipeOut) < 0) {
    close(pipeIn[0]);
    close(pipeIn[1]);
    return false;
  }

  pid = fork();
  if (pid < 0) {
    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);
    return false;
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

    _freeEnvArray(env);
    _exit(1);
  }

  close(pipeIn[0]);
  close(pipeOut[1]);

  // Set non-blocking mode on parent-side pipe FDs (rules.md: fcntl with
  // F_SETFL, O_NONBLOCK allowed)
  fcntl(pipeIn[1], F_SETFL, O_NONBLOCK);
  fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);

  // Close-on-exec to prevent leaking FDs if CGI forks again (rules.md:
  // FD_CLOEXEC allowed)
  fcntl(pipeIn[1], F_SETFD, FD_CLOEXEC);
  fcntl(pipeOut[0], F_SETFD, FD_CLOEXEC);

  inWriteFd = pipeIn[1];
  outReadFd = pipeOut[0];
  return true;
}

void CgiHandler::parseOutput(const std::vector<char> &output,
                             HttpResponse &response) {
  std::string rawOutput(output.begin(), output.end());
  size_t headerEnd = rawOutput.find("\r\n\r\n");
  if (headerEnd == std::string::npos)
    headerEnd = rawOutput.find("\n\n");

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
