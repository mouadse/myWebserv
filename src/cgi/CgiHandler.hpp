#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include <vector>
#include <unistd.h>
#include "../http/HTTPRequest.hpp"
#include "../http/HTTPResponse.hpp"

class CgiHandler {
public:
    CgiHandler(const HTTPRequest &request, const std::string &scriptPath,
               const std::string &scriptName, const std::string &interpreter);
    ~CgiHandler();

    void execute(HttpResponse &response);

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
