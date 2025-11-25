#include "LocationBlock.hpp"

LocationBlock::LocationBlock()
    : _root(""), _path(""), _autoindex(false), _index(""), _return(""),
      _alias(""), _methods(5, 0), _cgi_extensions(), _cgi_paths(),
      _max_body_size(kDefaultMaxBodySize), _extension_to_cgi() {}

LocationBlock::LocationBlock(const LocationBlock &other) {
  _root = other._root;
  _path = other._path;
  _autoindex = other._autoindex;
  _index = other._index;
  _return = other._return;
  _alias = other._alias;
  _methods = other._methods;
  _cgi_extensions = other._cgi_extensions;
  _cgi_paths = other._cgi_paths;
  _max_body_size = other._max_body_size;
  _extension_to_cgi = other._extension_to_cgi;
}

LocationBlock &LocationBlock::operator=(const LocationBlock &other) {
  if (this != &other) {
    _root = other._root;
    _path = other._path;
    _autoindex = other._autoindex;
    _index = other._index;
    _return = other._return;
    _alias = other._alias;
    _methods = other._methods;
    _cgi_extensions = other._cgi_extensions;
    _cgi_paths = other._cgi_paths;
    _max_body_size = other._max_body_size;
    _extension_to_cgi = other._extension_to_cgi;
  }
  return (*this);
}

LocationBlock::~LocationBlock(void) {}

void LocationBlock::setRoot(const std::string &root) {
  if (ConfigurationFile::getTypePath(root) != 2) {
    throw std::runtime_error("Invalid root path: " + root);
  }
  _root = root;
}
void LocationBlock::setPath(const std::string &path) { _path = path; }

static std::string stripTrailingSemicolonIfPresent(std::string &token,
                                                   const std::string &context) {
  if (!token.empty() && token[token.size() - 1] == ';') {
    enforceTrailingSemicolon(token, context);
  }
  return token;
}

void LocationBlock::setMethods(const std::vector<std::string> &methods) {
  _methods.assign(5, 0); // Reset methods
  for (size_t i = 0; i < methods.size(); ++i) {
    // ToDo to be inspected later
    std::string method = stripTrailingSemicolonIfPresent(
        const_cast<std::string &>(methods[i]), "location block methods");
    if (method == "GET") {
      _methods[0] = 1;
    } else if (method == "POST") {
      _methods[1] = 1;
    } else if (method == "DELETE") {
      _methods[2] = 1;
    } else if (method == "PUT") {
      _methods[3] = 1;
    } else if (method == "HEAD") {
      _methods[4] = 1;
    } else {
      throw std::runtime_error("Allow method not supported " + method);
    }
  }
}

void LocationBlock::setAutoindex(const std::string &autoindex) {
  std::string value = stripTrailingSemicolonIfPresent(
      const_cast<std::string &>(autoindex), "location block autoindex");
  if (value == "on") {
    _autoindex = true;
  } else if (value == "off") {
    _autoindex = false;
  } else {
    throw std::runtime_error("Autoindex value not supported: " + value);
  }
}

void LocationBlock::setIndex(const std::string &index) {
  std::string value = stripTrailingSemicolonIfPresent(
      const_cast<std::string &>(index), "location block index");
  _index = value;
}

void LocationBlock::setReturn(const std::string &ret) {
  std::string value = stripTrailingSemicolonIfPresent(
      const_cast<std::string &>(ret), "location block return");
  _return = value;
}

void LocationBlock::setAlias(const std::string &alias) {
  std::string value = stripTrailingSemicolonIfPresent(
      const_cast<std::string &>(alias), "location block alias");
  _alias = value;
}

void LocationBlock::setCgiPaths(const std::vector<std::string> &paths) {
  _cgi_paths = paths;
}

void LocationBlock::setCgiExtensions(
    const std::vector<std::string> &extensions) {
  _cgi_extensions = extensions;
}

void LocationBlock::setMaxBodySize(const std::string &size_str) {
  std::string value = stripTrailingSemicolonIfPresent(
      const_cast<std::string &>(size_str), "location block max body size");
  unsigned long size = 0;
  if (!isAllDigits(size_str)) {
    throw std::runtime_error("Max body size must be a positive integer: " +
                             size_str);
  }
  size = static_cast<unsigned long>(stoiStrict(size_str));
  if (!size) {
    throw std::runtime_error("Max body size must be greater than zero: " +
                             size_str);
  }
  _max_body_size = size;
}

void LocationBlock::setMaxBodySize(unsigned long size) {
  _max_body_size = size;
}

// Getters for our class members

const std::string &LocationBlock::getRoot() const { return _root; }
const std::string &LocationBlock::getPath() const { return _path; }
const std::string &LocationBlock::getIndex() const { return _index; }
const bool &LocationBlock::getAutoindex() const { return _autoindex; }
const std::string &LocationBlock::getReturn() const { return _return; }
const std::string &LocationBlock::getAlias() const { return _alias; }
const std::vector<short> &LocationBlock::getMethods() const { return _methods; }
const std::vector<std::string> &LocationBlock::getCgiExtensions() const {
  return _cgi_extensions;
}
const std::vector<std::string> &LocationBlock::getCgiPaths() const {
  return _cgi_paths;
}
const unsigned long &LocationBlock::getMaxBodySize() const {
  return _max_body_size;
}
const std::map<std::string, std::string> &
LocationBlock::getExtensionToCgiMap() const {
  return _extension_to_cgi;
}

std::string LocationBlock::getPrintMethods(void) const {
  std::string res;
  if (_methods[4])
    res.insert(0, "HEAD");
  if (_methods[3]) {
    if (!res.empty())
      res.insert(0, ", ");
    res.insert(0, "PUT");
  }
  if (_methods[2]) {
    if (!res.empty())
      res.insert(0, ", ");
    res.insert(0, "DELETE");
  }
  if (_methods[1]) {
    if (!res.empty())
      res.insert(0, ", ");
    res.insert(0, "POST");
  }
  if (_methods[0]) {
    if (!res.empty())
      res.insert(0, ", ");
    res.insert(0, "GET");
  }
  return (res);
}