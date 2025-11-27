#include "ServerConfigParser.hpp"

#include <arpa/inet.h>
#include <cctype>
#include <map>
#include <stdexcept>

#include "ConfigurationFile.hpp"
#include "ParserUtils.hpp"

namespace {
std::vector<std::string> splitParameters(const std::string &line,
                                         const std::string &delims) {
  std::vector<std::string> tokens;
  std::string::size_type start = 0;
  while (start < line.size()) {
    start = line.find_first_not_of(delims, start);
    if (start == std::string::npos)
      break;
    std::string::size_type end = line.find_first_of(delims, start);
    if (end == std::string::npos) {
      tokens.push_back(line.substr(start));
      break;
    }
    tokens.push_back(line.substr(start, end - start));
    start = end + 1;
  }
  return tokens;
}

std::string hostToString(const in_addr_t &host) {
  struct in_addr addr;
  addr.s_addr = host;
  char buffer[INET_ADDRSTRLEN] = {0};
  if (!inet_ntop(AF_INET, &addr, buffer, sizeof(buffer)))
    return "";
  return buffer;
}
} // namespace

ServerConfigParser::ServerConfigParser(void)
    : _servers(), _config_lines(), _num_of_servers(0) {}

ServerConfigParser::ServerConfigParser(const ServerConfigParser &other)
    : _servers(other._servers), _config_lines(other._config_lines),
      _num_of_servers(other._num_of_servers) {}

ServerConfigParser &
ServerConfigParser::operator=(const ServerConfigParser &other) {
  if (this != &other) {
    _servers = other._servers;
    _config_lines = other._config_lines;
    _num_of_servers = other._num_of_servers;
  }
  return *this;
}

ServerConfigParser::~ServerConfigParser() {}

int ServerConfigParser::createCluster(const std::string &config_path) {
  _servers.clear();
  _config_lines.clear();
  _num_of_servers = 0;

  if (ConfigurationFile::getTypePath(config_path) != 1)
    throw std::runtime_error("File is invalid");
  if (ConfigurationFile::checkFile(config_path, 4) == -1)
    throw std::runtime_error("File is not accessible");

  ConfigurationFile file(config_path);
  std::string content = file.getFileContent(config_path);
  if (content.empty())
    throw std::runtime_error("File is empty");

  removeComments(content);
  removeWhitespaces(content);
  splitServers(content);
  if (_config_lines.size() != _num_of_servers)
    throw std::runtime_error("Server count mismatch after parsing");

  for (size_t i = 0; i < _num_of_servers; ++i) {
    WebserverConfig server;
    createServer(_config_lines[i], server);
    _servers.push_back(server);
  }

  if (_num_of_servers > 1)
    checkServers();
  return 0;
}

void ServerConfigParser::removeComments(std::string &content) {
  size_t pos = content.find('#');
  while (pos != std::string::npos) {
    size_t end = content.find('\n', pos);
    if (end == std::string::npos) {
      content.erase(pos);
    } else {
      content.erase(pos, end - pos);
    }
    pos = content.find('#');
  }
}

void ServerConfigParser::removeWhitespaces(std::string &content) {
  content = trimWhitespace(content);
}

void ServerConfigParser::splitServers(std::string &content) {
  if (content.find("server") == std::string::npos)
    throw std::runtime_error("Server did not find");

  size_t search_pos = 0;
  while (search_pos < content.size()) {
    size_t start = locateServerStart(content, search_pos);
    if (start == std::string::npos)
      break;
    size_t end = locateServerEnd(content, start);
    if (end == std::string::npos)
      throw std::runtime_error("Problem with scope");
    _config_lines.push_back(content.substr(start, end - start + 1));
    ++_num_of_servers;
    search_pos = end + 1;
  }
}

size_t ServerConfigParser::locateServerStart(std::string &content, size_t pos) {
  size_t keyword_pos = content.find("server", pos);
  if (keyword_pos == std::string::npos) {
    for (size_t i = pos; i < content.size(); ++i) {
      if (!std::isspace(static_cast<unsigned char>(content[i])))
        throw std::runtime_error("Wrong character out of server scope{}");
    }
    return std::string::npos;
  }
  for (size_t i = pos; i < keyword_pos; ++i) {
    if (!std::isspace(static_cast<unsigned char>(content[i])))
      throw std::runtime_error("Wrong character out of server scope{}");
  }
  size_t brace_pos = keyword_pos + 6;
  while (brace_pos < content.size() &&
         std::isspace(static_cast<unsigned char>(content[brace_pos])))
    ++brace_pos;
  if (brace_pos >= content.size() || content[brace_pos] != '{')
    throw std::runtime_error("Wrong character out of server scope{}");
  return brace_pos;
}

size_t ServerConfigParser::locateServerEnd(std::string &content, size_t pos) {
  if (pos == std::string::npos || pos >= content.size() || content[pos] != '{')
    return std::string::npos;
  size_t scope = 0;
  for (size_t i = pos + 1; i < content.size(); ++i) {
    if (content[i] == '{') {
      ++scope;
    } else if (content[i] == '}') {
      if (scope == 0)
        return i;
      --scope;
    }
  }
  return std::string::npos;
}

void ServerConfigParser::createServer(std::string &server_config,
                                      WebserverConfig &server) {
  _parseServerContent(server_config, server);
}

void ServerConfigParser::_parseServerContent(const std::string &config,
                                             WebserverConfig &server) {
  std::vector<std::string> tokens =
      splitParameters(config + ' ', std::string(" \n\t\r"));
  if (tokens.size() < 3)
    throw std::runtime_error("Failed server validation");

  bool flag_autoindex = false;
  bool flag_max_body_size = false;
  std::vector<std::pair<std::string, std::vector<std::string> > > locations;
  std::vector<std::vector<std::string> > error_page_blocks;

  for (size_t i = 0; i < tokens.size(); ++i) {
    if (tokens[i] == "listen" && (i + 1) < tokens.size()) {
      if (server.getPort())
        throw std::runtime_error("Port is duplicated");
      server.setPort(tokens[++i]);
    } else if (tokens[i] == "location" && (i + 1) < tokens.size()) {
      std::string path;
      std::vector<std::string> location_tokens;
      _collectLocationBlock(tokens, i, path, location_tokens);
      locations.push_back(std::make_pair(path, location_tokens));
    } else if (tokens[i] == "host" && (i + 1) < tokens.size()) {
      if (server.getHost())
        throw std::runtime_error("Host is duplicated");
      server.setHost(tokens[++i]);
    } else if (tokens[i] == "root" && (i + 1) < tokens.size()) {
      if (!server.getRoot().empty())
        throw std::runtime_error("Root is duplicated");
      server.setRoot(tokens[++i]);
    } else if (tokens[i] == "error_page" && (i + 1) < tokens.size()) {
      std::vector<std::string> error_codes;
      while (++i < tokens.size()) {
        error_codes.push_back(tokens[i]);
        if (tokens[i].find(';') != std::string::npos)
          break;
        if (i + 1 >= tokens.size())
          throw std::runtime_error("Wrong character out of server scope{}");
      }
      error_page_blocks.push_back(error_codes);
    } else if (tokens[i] == "client_max_body_size" && (i + 1) < tokens.size()) {
      if (flag_max_body_size)
        throw std::runtime_error("Client_max_body_size is duplicated");
      server.setClientMaxBodySize(tokens[++i]);
      flag_max_body_size = true;
    } else if (tokens[i] == "server_name" && (i + 1) < tokens.size()) {
      if (!server.getServerName().empty())
        throw std::runtime_error("Server_name is duplicated");
      server.setServerName(tokens[++i]);
    } else if (tokens[i] == "index" && (i + 1) < tokens.size()) {
      if (!server.getIndex().empty())
        throw std::runtime_error("Index is duplicated");
      server.setIndex(tokens[++i]);
    } else if (tokens[i] == "autoindex" && (i + 1) < tokens.size()) {
      if (flag_autoindex)
        throw std::runtime_error("Autoindex of server is duplicated");
      server.setAutoindex(tokens[++i]);
      flag_autoindex = true;
    } else if (tokens[i] != "}" && tokens[i] != "{") {
      throw std::runtime_error("Unsupported directive: " + tokens[i]);
    }
  }

  if (server.getRoot().empty())
    server.setRoot("/;");
  if (server.getHost() == 0)
    server.setHost("localhost;");
  if (server.getIndex().empty())
    server.setIndex("index.html;");

  for (size_t i = 0; i < error_page_blocks.size(); ++i)
    server.setErrorPages(error_page_blocks[i]);
  for (size_t i = 0; i < locations.size(); ++i)
    _parseLocationTokens(locations[i].first, locations[i].second, server);

  if (ConfigurationFile::doesFileExistAndIsReadable(server.getRoot(),
                                                    server.getIndex()))
    throw std::runtime_error("Index from config file not found or unreadable");
  if (server.checkLocations())
    throw std::runtime_error("Locaition is duplicated");
  if (!server.getPort())
    throw std::runtime_error("Port not found");
  if (!server.isValidErrorPages())
    throw std::runtime_error(
        "Incorrect path for error page or number of error");
}

void ServerConfigParser::_collectLocationBlock(
    const std::vector<std::string> &tokens, size_t &index, std::string &path,
    std::vector<std::string> &location_tokens) {
  ++index;
  if (index >= tokens.size() || tokens[index] == "{" || tokens[index] == "}")
    throw std::runtime_error("Wrong character in server scope{}");
  path = tokens[index];

  if ((index + 1) >= tokens.size() || tokens[++index] != "{")
    throw std::runtime_error("Wrong character in server scope{}");
  ++index;

  while (index < tokens.size() && tokens[index] != "}")
    location_tokens.push_back(tokens[index++]);

  if (index >= tokens.size() || tokens[index] != "}")
    throw std::runtime_error("Wrong character in server scope{}");
}

void ServerConfigParser::_parseLocationTokens(
    const std::string &path, const std::vector<std::string> &location_tokens,
    WebserverConfig &server) {
  server.setLocationBlocks(path, location_tokens);
}

void ServerConfigParser::checkServers(void) {
  for (size_t i = 0; i < _servers.size(); ++i) {
    for (size_t j = i + 1; j < _servers.size(); ++j) {
      if (_servers[i].getPort() == _servers[j].getPort() &&
          _servers[i].getHost() == _servers[j].getHost() &&
          _servers[i].getServerName() == _servers[j].getServerName())
        throw std::runtime_error("Failed server validation");
    }
  }
}

std::vector<WebserverConfig> ServerConfigParser::getServers() const {
  return _servers;
}

int ServerConfigParser::print(std::ostream &out) const {
  out << "------------- Config -------------" << std::endl;
  for (size_t i = 0; i < _servers.size(); ++i) {
    const WebserverConfig &server = _servers[i];
    out << "Server #" << i + 1 << std::endl;
    out << "Server name: " << server.getServerName() << std::endl;
    out << "Host: " << hostToString(server.getHost()) << std::endl;
    out << "Root: " << server.getRoot() << std::endl;
    out << "Index: " << server.getIndex() << std::endl;
    out << "Port: " << server.getPort() << std::endl;
    out << "Max BSize: " << server.getMaxBodySize() << std::endl;
    out << "Error pages: " << server.getErrorPages().size() << std::endl;
    std::map<short, std::string>::const_iterator error_it =
        server.getErrorPages().begin();
    while (error_it != server.getErrorPages().end()) {
      out << error_it->first << " - " << error_it->second << std::endl;
      ++error_it;
    }
    out << "Locations: " << server.getLocationBlocks().size() << std::endl;
    std::vector<LocationBlock>::const_iterator loc_it =
        server.getLocationBlocks().begin();
    while (loc_it != server.getLocationBlocks().end()) {
      out << "name location: " << loc_it->getPath() << std::endl;
      out << "methods: " << loc_it->getPrintMethods() << std::endl;
      out << "index: " << loc_it->getIndex() << std::endl;
      if (loc_it->getCgiPaths().empty()) {
        out << "root: " << loc_it->getRoot() << std::endl;
        if (!loc_it->getReturn().empty())
          out << "return: " << loc_it->getReturn() << std::endl;
        if (!loc_it->getAlias().empty())
          out << "alias: " << loc_it->getAlias() << std::endl;
      } else {
        out << "cgi root: " << loc_it->getRoot() << std::endl;
        out << "cgi_path: " << loc_it->getCgiPaths().size() << std::endl;
        out << "cgi_ext: " << loc_it->getCgiExtensions().size() << std::endl;
      }
      ++loc_it;
    }
    out << "-----------------------------" << std::endl;
  }
  return 0;
}
