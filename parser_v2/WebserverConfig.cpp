#include "WebserverConfig.hpp"

#include <cerrno>
#include <stdexcept>
#include <unistd.h>

namespace {
std::string normalizeDirective(std::string value, const std::string &context) {
  value = trimWhitespace(value);
  enforceTrailingSemicolon(value, context);
  value = trimWhitespace(value);
  return value;
}

std::string joinPaths(const std::string &base, const std::string &relative) {
  if (relative.empty())
    return base;
  std::string rel = relative;
  if (!rel.empty() && rel[0] == '/')
    rel.erase(0, 1);
  if (base.empty())
    return rel;
  if (base[base.size() - 1] == '/')
    return base + rel;
  return base + "/" + rel;
}
} // namespace

WebserverConfig::WebserverConfig(void)
    : _port(0), _host(0), _server_name(""), _root(""), _index(""),
      _max_body_size(kDefaultMaxBodySize), _autoindex(false), _error_pages(),
      _location_blocks(), _server_address(), _listen_fd(-1) {
  std::memset(&_server_address, 0, sizeof(_server_address));
  initErrorPages();
}

WebserverConfig::WebserverConfig(const WebserverConfig &other)
    : _port(other._port), _host(other._host), _server_name(other._server_name),
      _root(other._root), _index(other._index),
      _max_body_size(other._max_body_size), _autoindex(other._autoindex),
      _error_pages(other._error_pages),
      _location_blocks(other._location_blocks),
      _server_address(other._server_address), _listen_fd(other._listen_fd) {}

WebserverConfig &WebserverConfig::operator=(const WebserverConfig &other) {
  if (this != &other) {
    _port = other._port;
    _host = other._host;
    _server_name = other._server_name;
    _root = other._root;
    _index = other._index;
    _max_body_size = other._max_body_size;
    _autoindex = other._autoindex;
    _error_pages = other._error_pages;
    _location_blocks = other._location_blocks;
    _server_address = other._server_address;
    _listen_fd = other._listen_fd;
  }
  return (*this);
}

WebserverConfig::~WebserverConfig() {}

void WebserverConfig::initErrorPages(void) {
  _error_pages[301] = "";
  _error_pages[302] = "";
  _error_pages[400] = "";
  _error_pages[401] = "";
  _error_pages[402] = "";
  _error_pages[403] = "";
  _error_pages[404] = "";
  _error_pages[405] = "";
  _error_pages[406] = "";
  _error_pages[500] = "";
  _error_pages[501] = "";
  _error_pages[502] = "";
  _error_pages[503] = "";
  _error_pages[505] = "";
}

void WebserverConfig::setServerName(std::string server_name) {
  _server_name = normalizeDirective(server_name, "server_name");
}

void WebserverConfig::setHost(std::string host) {
  host = normalizeDirective(host, "host");
  if (host == "localhost")
    host = "127.0.0.1";
  if (!isValidHost(host))
    throw std::runtime_error("Wrong syntax: host");
  _host = inet_addr(host.c_str());
}

void WebserverConfig::setRoot(std::string root) {
  root = normalizeDirective(root, "root");
  if (ConfigurationFile::getTypePath(root) == 2) {
    _root = root;
    return;
  }
  char cwd[1024];
  if (!getcwd(cwd, sizeof(cwd)))
    throw std::runtime_error("Failed to resolve working directory");
  std::string full_root = std::string(cwd);
  if (!full_root.empty() && full_root[full_root.size() - 1] != '/')
    full_root += "/";
  full_root += root;
  if (ConfigurationFile::getTypePath(full_root) != 2)
    throw std::runtime_error("Wrong syntax: root");
  _root = full_root;
}

void WebserverConfig::setFdx(int fd) { _listen_fd = fd; }

void WebserverConfig::setPort(std::string value) {
  value = normalizeDirective(value, "port");
  if (!isAllDigits(value))
    throw std::runtime_error("Wrong syntax: port");
  unsigned int port = static_cast<unsigned int>(stoiStrict(value));
  if (port < 1 || port > 65535)
    throw std::runtime_error("Wrong syntax: port");
  _port = static_cast<uint16_t>(port);
}

void WebserverConfig::setClientMaxBodySize(std::string value) {
  value = normalizeDirective(value, "client_max_body_size");
  if (!isAllDigits(value))
    throw std::runtime_error("Wrong syntax: client_max_body_size");
  size_t size = static_cast<size_t>(stoiStrict(value));
  if (size == 0)
    throw std::runtime_error("Wrong syntax: client_max_body_size");
  _max_body_size = size;
}

void WebserverConfig::setIndex(std::string index) {
  _index = normalizeDirective(index, "index");
}

void WebserverConfig::setAutoindex(std::string autoindex) {
  autoindex = normalizeDirective(autoindex, "autoindex");
  if (autoindex != "on" && autoindex != "off")
    throw std::runtime_error("Wrong syntax: autoindex");
  _autoindex = (autoindex == "on");
}

void WebserverConfig::setErrorPages(std::vector<std::string> error_pages) {
  if (error_pages.empty())
    return;
  if (error_pages.size() % 2 != 0)
    throw std::runtime_error("Error page initialization failed");
  for (size_t i = 0; i + 1 < error_pages.size(); i += 2) {
    const std::string &code = error_pages[i];
    if (!isAllDigits(code) || code.size() != 3)
      throw std::runtime_error("Error code is invalid");
    short status_code = static_cast<short>(stoiStrict(code));
    if (statusCodeToString(status_code) == "Undefined" || status_code < 400)
      throw std::runtime_error("Incorrect error code: " + code);
    std::string path = error_pages[i + 1];
    if (!path.empty() && path[path.size() - 1] == ';')
      path = normalizeDirective(path, "error_page");
    std::string candidate = path;
    if (ConfigurationFile::getTypePath(candidate) != 1)
      candidate = joinPaths(_root, path);
    if (ConfigurationFile::getTypePath(candidate) != 1)
      throw std::runtime_error("Incorrect path for error page file: " +
                               candidate);
    if (ConfigurationFile::checkFile(candidate, 0) == -1 ||
        ConfigurationFile::checkFile(candidate, 4) == -1)
      throw std::runtime_error("Error page file :" + candidate +
                               " is not accessible");
    std::map<short, std::string>::iterator it = _error_pages.find(status_code);
    if (it != _error_pages.end())
      it->second = path;
    else
      _error_pages.insert(std::make_pair(status_code, path));
  }
}

void WebserverConfig::setLocationBlocks(
    std::string path, const std::vector<std::string> &parameters) {
  LocationBlock new_location;
  bool has_methods = false;
  bool has_autoindex = false;
  bool has_max_size = false;

  new_location.setPath(path);
  for (size_t i = 0; i < parameters.size(); ++i) {
    if (parameters[i] == "root" && (i + 1) < parameters.size()) {
      if (!new_location.getRoot().empty())
        throw std::runtime_error("Root of location is duplicated");
      std::string value = parameters[++i];
      value = normalizeDirective(value, "location root");
      if (ConfigurationFile::getTypePath(value) == 2)
        new_location.setRoot(value);
      else
        new_location.setRoot(joinPaths(_root, value));
    } else if ((parameters[i] == "allow_methods" ||
                parameters[i] == "methods") &&
               (i + 1) < parameters.size()) {
      if (has_methods)
        throw std::runtime_error("Allow_methods of location is duplicated");
      std::vector<std::string> methods;
      while (++i < parameters.size()) {
        if (parameters[i].find(';') != std::string::npos) {
          std::string value = parameters[i];
          value = normalizeDirective(value, "allow_methods");
          methods.push_back(value);
          break;
        } else {
          methods.push_back(parameters[i]);
          if (i + 1 >= parameters.size())
            throw std::runtime_error("Token is invalid");
        }
      }
      new_location.setMethods(methods);
      has_methods = true;
    } else if (parameters[i] == "autoindex" && (i + 1) < parameters.size()) {
      if (path == "/cgi-bin")
        throw std::runtime_error("Parametr autoindex not allow for CGI");
      if (has_autoindex)
        throw std::runtime_error("Autoindex of location is duplicated");
      std::string value = parameters[++i];
      value = normalizeDirective(value, "location autoindex");
      new_location.setAutoindex(value);
      has_autoindex = true;
    } else if (parameters[i] == "index" && (i + 1) < parameters.size()) {
      if (!new_location.getIndex().empty())
        throw std::runtime_error("Index of location is duplicated");
      std::string value = parameters[++i];
      value = normalizeDirective(value, "location index");
      new_location.setIndex(value);
    } else if (parameters[i] == "return" && (i + 1) < parameters.size()) {
      if (path == "/cgi-bin")
        throw std::runtime_error("Parametr return not allow for CGI");
      if (!new_location.getReturn().empty())
        throw std::runtime_error("Return of location is duplicated");
      std::string value = parameters[++i];
      value = normalizeDirective(value, "location return");
      new_location.setReturn(value);
    } else if (parameters[i] == "alias" && (i + 1) < parameters.size()) {
      if (path == "/cgi-bin")
        throw std::runtime_error("Parametr alias not allow for CGI");
      if (!new_location.getAlias().empty())
        throw std::runtime_error("Alias of location is duplicated");
      std::string value = parameters[++i];
      value = normalizeDirective(value, "location alias");
      new_location.setAlias(value);
    } else if (parameters[i] == "cgi_ext" && (i + 1) < parameters.size()) {
      std::vector<std::string> extensions;
      while (++i < parameters.size()) {
        if (parameters[i].find(';') != std::string::npos) {
          std::string value = parameters[i];
          value = normalizeDirective(value, "cgi_ext");
          extensions.push_back(value);
          break;
        } else {
          extensions.push_back(parameters[i]);
          if (i + 1 >= parameters.size())
            throw std::runtime_error("Token is invalid");
        }
      }
      new_location.setCgiExtensions(extensions);
    } else if (parameters[i] == "cgi_path" && (i + 1) < parameters.size()) {
      std::vector<std::string> paths_list;
      while (++i < parameters.size()) {
        if (parameters[i].find(';') != std::string::npos) {
          std::string value = parameters[i];
          value = normalizeDirective(value, "cgi_path");
          if (value.find("/python") == std::string::npos &&
              value.find("/bash") == std::string::npos)
            throw std::runtime_error("cgi_path is invalid");
          paths_list.push_back(value);
          break;
        } else {
          if (parameters[i].find("/python") == std::string::npos &&
              parameters[i].find("/bash") == std::string::npos)
            throw std::runtime_error("cgi_path is invalid");
          paths_list.push_back(parameters[i]);
          if (i + 1 >= parameters.size())
            throw std::runtime_error("Token is invalid");
        }
      }
      new_location.setCgiPaths(paths_list);
    } else if (parameters[i] == "client_max_body_size" &&
               (i + 1) < parameters.size()) {
      if (has_max_size)
        throw std::runtime_error("Maxbody_size of location is duplicated");
      std::string value = parameters[++i];
      value = normalizeDirective(value, "location client_max_body_size");
      new_location.setMaxBodySize(value);
      has_max_size = true;
    } else if (i < parameters.size()) {
      throw std::runtime_error("Parametr in a location is invalid");
    }
  }

  if (new_location.getPath() != "/cgi-bin" && new_location.getIndex().empty())
    new_location.setIndex(_index);
  if (!has_max_size)
    new_location.setMaxBodySize(_max_body_size);

  int validation = isValidLocationBlock(new_location);
  if (validation == 1)
    throw std::runtime_error("Failed CGI validation");
  else if (validation == 2)
    throw std::runtime_error("Failed path in location validation");
  else if (validation == 3)
    throw std::runtime_error("Failed redirection file in location validation");
  else if (validation == 4)
    throw std::runtime_error("Failed alias file in location validation");
  else if (validation == 5)
    throw std::runtime_error("Failed index file in location validation");

  _location_blocks.push_back(new_location);
}

bool WebserverConfig::isValidHost(std::string host) const {
  struct sockaddr_in sockaddr;
  return (inet_pton(AF_INET, host.c_str(), &(sockaddr.sin_addr)) ? true
                                                                 : false);
}

bool WebserverConfig::isValidErrorPages() {
  std::map<short, std::string>::const_iterator it;
  for (it = _error_pages.begin(); it != _error_pages.end(); ++it) {
    if (it->first < 100 || it->first > 599)
      return false;
    if (it->second.empty())
      continue;
    std::string candidate = it->second;
    if (ConfigurationFile::getTypePath(candidate) != 1)
      candidate = joinPaths(getRoot(), it->second);
    if (ConfigurationFile::getTypePath(candidate) != 1 ||
        ConfigurationFile::checkFile(candidate, 0) < 0 ||
        ConfigurationFile::checkFile(candidate, 4) < 0)
      return false;
  }
  return true;
}

int WebserverConfig::isValidLocationBlock(LocationBlock &location_block) const {
  if (location_block.getPath() == "/cgi-bin") {
    if (location_block.getCgiPaths().empty() ||
        location_block.getCgiExtensions().empty() ||
        location_block.getIndex().empty())
      return 1;
    if (ConfigurationFile::checkFile(location_block.getIndex(), 4) < 0) {
      std::string candidate = joinPaths(
          joinPaths(location_block.getRoot(), location_block.getPath()),
          location_block.getIndex());
      if (ConfigurationFile::getTypePath(candidate) != 1) {
        char cwd[1024];
        if (!getcwd(cwd, sizeof(cwd)))
          return 1;
        location_block.setRoot(std::string(cwd));
        candidate = joinPaths(
            joinPaths(location_block.getRoot(), location_block.getPath()),
            location_block.getIndex());
      }
      if (candidate.empty() || ConfigurationFile::getTypePath(candidate) != 1 ||
          ConfigurationFile::checkFile(candidate, 4) < 0)
        return 1;
    }
    if (location_block.getCgiPaths().size() !=
        location_block.getCgiExtensions().size())
      return 1;
    for (std::vector<std::string>::const_iterator it =
             location_block.getCgiPaths().begin();
         it != location_block.getCgiPaths().end(); ++it) {
      if (ConfigurationFile::getTypePath(*it) < 0)
        return 1;
    }
    location_block._extension_to_cgi.clear();
    for (std::vector<std::string>::const_iterator it =
             location_block.getCgiExtensions().begin();
         it != location_block.getCgiExtensions().end(); ++it) {
      const std::string &ext = *it;
      if (ext != ".py" && ext != "*.py" && ext != ".sh" && ext != "*.sh")
        return 1;
      for (std::vector<std::string>::const_iterator path_it =
               location_block.getCgiPaths().begin();
           path_it != location_block.getCgiPaths().end(); ++path_it) {
        if ((ext == ".py" || ext == "*.py") &&
            path_it->find("python") != std::string::npos)
          location_block._extension_to_cgi.insert(
              std::make_pair(".py", *path_it));
        else if ((ext == ".sh" || ext == "*.sh") &&
                 path_it->find("bash") != std::string::npos)
          location_block._extension_to_cgi[".sh"] = *path_it;
      }
    }
    if (location_block.getCgiPaths().size() !=
        location_block.getExtensionToCgiMap().size())
      return 1;
  } else {
    if (location_block.getPath().empty() || location_block.getPath()[0] != '/')
      return 2;
    if (location_block.getRoot().empty())
      location_block.setRoot(_root);
    std::string location_root =
        joinPaths(location_block.getRoot(), location_block.getPath());
    if (ConfigurationFile::getTypePath(location_root) == 2) {
      std::string candidate_index =
          joinPaths(location_root, location_block.getIndex());
      if (ConfigurationFile::getTypePath(candidate_index) != 1 ||
          ConfigurationFile::checkFile(candidate_index, 4) < 0)
        return 5;
    }
    if (!location_block.getReturn().empty()) {
      if (ConfigurationFile::doesFileExistAndIsReadable(
              location_block.getRoot(), location_block.getReturn()))
        return 3;
    }
    if (!location_block.getAlias().empty()) {
      if (ConfigurationFile::doesFileExistAndIsReadable(
              location_block.getRoot(), location_block.getAlias()))
        return 4;
    }
  }
  return 0;
}

const std::string &WebserverConfig::getServerName() const {
  return _server_name;
}

const uint16_t &WebserverConfig::getPort() const { return _port; }

const in_addr_t &WebserverConfig::getHost() const { return _host; }

const size_t &WebserverConfig::getMaxBodySize() const { return _max_body_size; }

const std::vector<LocationBlock> &WebserverConfig::getLocationBlocks() const {
  return _location_blocks;
}

const std::string &WebserverConfig::getRoot() const { return _root; }

const std::map<short, std::string> &WebserverConfig::getErrorPages() const {
  return _error_pages;
}

const std::string &WebserverConfig::getIndex() const { return _index; }

const bool &WebserverConfig::getAutoindex() const { return _autoindex; }

const std::string WebserverConfig::getPathErrorPage(short key) const {
  std::map<short, std::string>::const_iterator it = _error_pages.find(key);
  if (it == _error_pages.end())
    throw std::runtime_error("Error_page does not exist");
  return it->second;
}

std::vector<LocationBlock>::const_iterator
WebserverConfig::getLocationBlockByName(const std::string &name) const {
  std::vector<LocationBlock>::const_iterator it;
  for (it = _location_blocks.begin(); it != _location_blocks.end(); ++it) {
    if (it->getPath() == name)
      return it;
  }
  throw std::runtime_error("Error: path to location not found");
}

void WebserverConfig::checkTokenValidity(std::string &token) {
  enforceTrailingSemicolon(token, "directive");
}

bool WebserverConfig::checkLocations() const {
  if (_location_blocks.size() < 2)
    return false;
  for (std::vector<LocationBlock>::const_iterator it = _location_blocks.begin();
       it != _location_blocks.end(); ++it) {
    for (std::vector<LocationBlock>::const_iterator jt = it + 1;
         jt != _location_blocks.end(); ++jt) {
      if (it->getPath() == jt->getPath())
        return true;
    }
  }
  return false;
}

void WebserverConfig::setupWebserver(void) {
  _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (_listen_fd == -1)
    throw std::runtime_error(std::string("socket error: ") +
                             std::strerror(errno));
  int option_value = 1;
  setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &option_value,
             sizeof(option_value));
  std::memset(&_server_address, 0, sizeof(_server_address));
  _server_address.sin_family = AF_INET;
  _server_address.sin_addr.s_addr = _host;
  _server_address.sin_port = htons(_port);
  if (bind(_listen_fd, reinterpret_cast<struct sockaddr *>(&_server_address),
           sizeof(_server_address)) == -1) {
    close(_listen_fd);
    throw std::runtime_error(std::string("bind error: ") +
                             std::strerror(errno));
  }
}

int WebserverConfig::getFdX(void) const { return _listen_fd; }
