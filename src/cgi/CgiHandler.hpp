#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../http/HTTPRequest.hpp"
#include "../http/HTTPResponse.hpp"
#include <map>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

class CgiHandler {
public:
  CgiHandler(const HTTPRequest &request, const std::string &scriptPath,
             const std::string &scriptName, const std::string &interpreter);
  ~CgiHandler();

  bool spawn(int &inWriteFd, int &outReadFd, pid_t &pid) const;
  static void parseOutput(const std::vector<char> &output,
                          HttpResponse &response);

private:
  const HTTPRequest &_request;
  std::string _scriptPath;
  std::string _scriptName;
  std::string _interpreter;
  std::map<std::string, std::string> _env;

  void _initEnv();
  char **_createEnvArray() const;
  void _freeEnvArray(char **env) const;
  std::string _getInterpreter() const;
};

#endif
