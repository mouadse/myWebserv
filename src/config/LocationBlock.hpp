#ifndef LOCATIONBLOCK_HPP
#define LOCATIONBLOCK_HPP
#include "ConfigurationFile.hpp"
#include "ParserUtils.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>

class LocationBlock {
private:
  std::string _root;
  std::string _path;
  bool _autoindex;
  std::string _index;
  std::string _return;
  std::string _alias;
  std::vector<short> _methods;
  std::vector<std::string> _cgi_extensions;
  std::vector<std::string> _cgi_paths;

  unsigned long _max_body_size;

public:
  std::map<std::string, std::string> _extension_to_cgi;
  LocationBlock(void);
  LocationBlock(const LocationBlock &other);
  LocationBlock &operator=(const LocationBlock &other);
  ~LocationBlock();
  // Setter methods for our private members
  void setRoot(const std::string &root);
  void setPath(const std::string &path);
  void setAutoindex(const std::string &autoindex);
  void setMethods(const std::vector<std::string> &methods);
  void setIndex(const std::string &index);
  void setReturn(const std::string &ret);
  void setAlias(const std::string &alias);
  void setCgiExtensions(const std::vector<std::string> &extensions);
  void setCgiPaths(const std::vector<std::string> &paths);
  void setMaxBodySize(const std::string &size);
  void setMaxBodySize(unsigned long size);

  // Getter methods for our private members
  const std::string &getRoot(void) const;
  const std::string &getPath(void) const;
  const bool &getAutoindex(void) const;
  const std::string &getIndex(void) const;
  const std::string &getReturn(void) const;
  const std::string &getAlias(void) const;
  const std::vector<short> &getMethods(void) const;
  const std::vector<std::string> &getCgiExtensions(void) const;
  const std::vector<std::string> &getCgiPaths(void) const;
  const std::map<std::string, std::string> &getExtensionToCgiMap(void) const;
  const unsigned long &getMaxBodySize(void) const;

  std::string getPrintMethods(void) const;
};

#endif // LOCATIONBLOCK_HPP
