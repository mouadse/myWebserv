#include "../include/ConfigParser.hpp"
#include "../include/ConfigFile.hpp"
#include "../include/ParserUtils.hpp"

#include <cctype>
#include <iostream>
#include <sstream>

namespace {
/**
 * @brief Splits a string into a vector of tokens based on delimiters.
 *
 * @param line The string to split.
 * @param sep The delimiters characters.
 * @return std::vector<std::string> The list of tokens.
 */
std::vector<std::string> splitParameters(const std::string &line,
                                         const std::string &sep) {
  std::vector<std::string> tokens;
  std::string::size_type start = 0;
  std::string::size_type end = 0;

  while (1) {
    end = line.find_first_of(sep, start);
    if (end == std::string::npos)
      break;
    std::string tmp = line.substr(start, end - start);
    if (!tmp.empty()) // Avoid adding empty strings if multiple delimiters are
                      // present
      tokens.push_back(tmp);
    start = line.find_first_not_of(sep, end);
    if (start == std::string::npos)
      break;
  }
  // Catch the last token if the string doesn't end with a delimiter
  if (start < line.length()) {
    std::string tmp = line.substr(start);
    if (!tmp.empty())
      tokens.push_back(tmp);
  }
  return (tokens);
}
} // namespace

ConfigParser::ConfigParser() : _servers(), _server_config(), _nb_server(0) {}

ConfigParser::~ConfigParser() {}

ConfigParser::ConfigParser(const ConfigParser &other)
    : _servers(other._servers), _server_config(other._server_config),
      _nb_server(other._nb_server) {}

ConfigParser &ConfigParser::operator=(const ConfigParser &rhs) {
  if (this != &rhs) {
    _servers = rhs._servers;
    _server_config = rhs._server_config;
    _nb_server = rhs._nb_server;
  }
  return (*this);
}

int ConfigParser::print(std::ostream &out) const {
  out << "------------- Config -------------" << std::endl;
  for (size_t i = 0; i < _servers.size(); i++) {
    out << "Server #" << i + 1 << std::endl;
    out << "Server name: " << _servers[i].getServerName() << std::endl;
    out << "Host: " << _servers[i].getHost() << std::endl;
    out << "Root: " << _servers[i].getRoot() << std::endl;
    out << "Index: " << _servers[i].getIndex() << std::endl;
    out << "Port: " << _servers[i].getPort() << std::endl;
    out << "Max BSize: " << _servers[i].getClientMaxBodySize() << std::endl;
    out << "Error pages: " << _servers[i].getErrorPages().size() << std::endl;
    std::map<short, std::string>::const_iterator it =
        _servers[i].getErrorPages().begin();
    while (it != _servers[i].getErrorPages().end()) {
      out << (*it).first << " - " << it->second << std::endl;
      ++it;
    }
    out << "Locations: " << _servers[i].getLocations().size() << std::endl;
    std::vector<Location>::const_iterator itl =
        _servers[i].getLocations().begin();
    while (itl != _servers[i].getLocations().end()) {
      out << "name location: " << itl->getPath() << std::endl;
      out << "methods: " << itl->getPrintMethods() << std::endl;
      out << "index: " << itl->getIndexLocation() << std::endl;
      if (itl->getCgiPath().empty()) {
        out << "root: " << itl->getRootLocation() << std::endl;
        if (!itl->getReturn().empty())
          out << "return: " << itl->getReturn() << std::endl;
        if (!itl->getAlias().empty())
          out << "alias: " << itl->getAlias() << std::endl;
      } else {
        out << "cgi root: " << itl->getRootLocation() << std::endl;
        out << "cgi_path: " << itl->getCgiPath().size() << std::endl;
        out << "cgi_ext: " << itl->getCgiExtension().size() << std::endl;
      }
      ++itl;
    }
    out << "-----------------------------" << std::endl;
  }
  return (0);
}

int ConfigParser::createCluster(const std::string &config_file) {
  std::string content;
  ConfigFile file(config_file);

  // Validate file type and accessibility
  if (file.getTypePath(file.getPath()) != 1)
    throw ConfigError("File is invalid");
  if (file.checkFile(file.getPath(), 4) == -1)
    throw ConfigError("File is not accessible");

  // Read file content
  content = file.readFile(config_file);
  if (content.empty())
    throw ConfigError("File is empty");

  // Pre-process content
  removeComments(content);
  removeWhiteSpace(content);

  // Split content into server blocks
  splitServers(content);
  if (_server_config.size() != _nb_server)
    throw ConfigError("Server count mismatch after parsing");

  // Parse each server block
  for (size_t i = 0; i < _nb_server; i++) {
    ServerConfig server;
    createServer(_server_config[i], server);
    _servers.push_back(server);
  }

  // Validate cluster configuration
  if (_nb_server > 1)
    checkServers();
  return (0);
}

void ConfigParser::removeComments(std::string &content) {
  size_t pos = content.find('#');
  while (pos != std::string::npos) {
    size_t pos_end = content.find('\n', pos);
    content.erase(pos, pos_end - pos);
    pos = content.find('#');
  }
}

void ConfigParser::removeWhiteSpace(std::string &content) {
  content = trim(content);
}

void ConfigParser::splitServers(std::string &content) {
  size_t start = 0;
  size_t end = 1;

  if (content.find("server", 0) == std::string::npos)
    throw ConfigError("Server did not find");
  while (start != end && start < content.length()) {
    start = findStartServer(start, content);
    end = findEndServer(start, content);
    if (start == end)
      throw ConfigError("Problem with scope");
    _server_config.push_back(content.substr(start, end - start + 1));
    _nb_server++;
    start = end + 1;
  }
}

size_t ConfigParser::findStartServer(size_t start, std::string &content) {
  size_t i;

  for (i = start; content[i]; i++) {
    if (content[i] == 's')
      break;
    if (!isspace(content[i]))
      throw ConfigError("Wrong character out of server scope{}");
  }
  if (!content[i])
    return (start);
  if (content.compare(i, 6, "server") != 0)
    throw ConfigError("Wrong character out of server scope{}");
  i += 6;
  while (content[i] && isspace(content[i]))
    i++;
  if (content[i] == '{')
    return (i);
  else
    throw ConfigError("Wrong character out of server scope{}");
}

size_t ConfigParser::findEndServer(size_t start, std::string &content) {
  size_t i;
  size_t scope = 0;

  for (i = start + 1; content[i]; i++) {
    if (content[i] == '{')
      scope++;
    if (content[i] == '}') {
      if (!scope)
        return (i);
      scope--;
    }
  }
  return (start);
}

void ConfigParser::createServer(std::string &config, ServerConfig &server) {
  _parseServerContent(config, server);
}

void ConfigParser::_parseServerContent(const std::string &config,
                                       ServerConfig &server) {
  std::vector<std::string> tokens;
  bool flag_autoindex = false;
  bool flag_max_size = false;

  tokens = splitParameters(config + ' ', std::string(" \n\t"));
  if (tokens.size() < 3)
    throw ConfigError("Failed server validation");

  for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i] == "listen" && (i + 1) < tokens.size()) {
      if (server.getPort())
        throw ConfigError("Port is duplicated");
      server.setPort(tokens[++i]);
    } else if (tokens[i] == "location" && (i + 1) < tokens.size()) {
      _parseLocation(tokens, i, server);
    } else if (tokens[i] == "host" && (i + 1) < tokens.size()) {
      if (server.getHost())
        throw ConfigError("Host is duplicated");
      server.setHost(tokens[++i]);
    } else if (tokens[i] == "root" && (i + 1) < tokens.size()) {
      if (!server.getRoot().empty())
        throw ConfigError("Root is duplicated");
      server.setRoot(tokens[++i]);
    } else if (tokens[i] == "error_page" && (i + 1) < tokens.size()) {
      std::vector<std::string> error_codes;
      while (++i < tokens.size()) {
        error_codes.push_back(tokens[i]);
        if (tokens[i].find(';') != std::string::npos)
          break;
        if (i + 1 >= tokens.size())
          throw ConfigError("Wrong character out of server scope{}");
      }
      server.setErrorPages(error_codes);
    } else if (tokens[i] == "client_max_body_size" && (i + 1) < tokens.size()) {
      if (flag_max_size)
        throw ConfigError("Client_max_body_size is duplicated");
      server.setClientMaxBodySize(tokens[++i]);
      flag_max_size = true;
    } else if (tokens[i] == "server_name" && (i + 1) < tokens.size()) {
      if (!server.getServerName().empty())
        throw ConfigError("Server_name is duplicated");
      server.setServerName(tokens[++i]);
    } else if (tokens[i] == "index" && (i + 1) < tokens.size()) {
      if (!server.getIndex().empty())
        throw ConfigError("Index is duplicated");
      server.setIndex(tokens[++i]);
    } else if (tokens[i] == "autoindex" && (i + 1) < tokens.size()) {
      if (flag_autoindex)
        throw ConfigError("Autoindex of server is duplicated");
      server.setAutoindex(tokens[++i]);
      flag_autoindex = true;
    } else if (tokens[i] != "}" && tokens[i] != "{") {
      throw ConfigError("Unsupported directive: " + tokens[i]);
    }
  }

  // Set defaults if not specified
  if (server.getRoot().empty())
    server.setRoot("/;");
  if (server.getHost() == 0)
    server.setHost("localhost;");
  if (server.getIndex().empty())
    server.setIndex("index.html;");

  // Final validations
  if (ConfigFile::isFileExistAndReadable(server.getRoot(), server.getIndex()))
    throw ConfigError("Index from config file not found or unreadable");
  if (server.checkLocaitons())
    throw ConfigError("Locaition is duplicated");
  if (!server.getPort())
    throw ConfigError("Port not found");

  if (!server.isValidErrorPages())
    throw ConfigError("Incorrect path for error page or number of error");
}

void ConfigParser::_parseLocation(const std::vector<std::string> &tokens,
                                  size_t &i, ServerConfig &server) {
  std::string path;
  std::vector<std::string> location_tokens;

  i++; // Move past "location"
  if (tokens[i] == "{" || tokens[i] == "}")
    throw ConfigError("Wrong character in server scope{}");
  path = tokens[i];

  if (tokens[++i] != "{")
    throw ConfigError("Wrong character in server scope{}");
  i++;

  while (i < tokens.size() && tokens[i] != "}")
    location_tokens.push_back(tokens[i++]);

  server.setLocation(path, location_tokens);

  if (i < tokens.size() && tokens[i] != "}")
    throw ConfigError("Wrong character in server scope{}");
}

void ConfigParser::checkServers() {
  std::vector<ServerConfig>::iterator it1;
  std::vector<ServerConfig>::iterator it2;

  for (it1 = _servers.begin(); it1 != _servers.end() - 1; it1++) {
    for (it2 = it1 + 1; it2 != _servers.end(); it2++) {
      if (it1->getPort() == it2->getPort() &&
          it1->getHost() == it2->getHost() &&
          it1->getServerName() == it2->getServerName())
        throw ConfigError("Failed server validation");
    }
  }
}

std::vector<ServerConfig> ConfigParser::getServers() const {
  return (_servers);
}
