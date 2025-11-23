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

