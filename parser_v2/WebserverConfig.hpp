#ifndef WEBSERVERCONFIG_HPP
#define WEBSERVERCONFIG_HPP

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <vector>

#include "ConfigurationFile.hpp"
#include "LocationBlock.hpp"
#include "ParserUtils.hpp"

class WebserverConfig {
private:
  uint16_t _port;
  in_addr_t _host;
  std::string _server_name;
  std::string _root;
  std::string _index;
  size_t _max_body_size;
  bool _autoindex;
  std::map<short, std::string> _error_pages;
  std::vector<LocationBlock> _location_blocks;
  struct sockaddr_in _server_address;
  int _listen_fd;

public:
  WebserverConfig(void);
  WebserverConfig(const WebserverConfig &other);
  WebserverConfig &operator=(const WebserverConfig &other);
  ~WebserverConfig();

  void initErrorPages(void);

  // Setters for our attributes
  void setServerName(std::string server_name);
  void setHost(std::string host);
  void setRoot(std::string root);
  void setFdx(int fd);
  void setPort(std::string value);
  void setClientMaxBodySize(std::string value);
  void setErrorPages(std::vector<std::string> error_pages);
  void setIndex(std::string index);

  void setLocationBlocks(std::string path,
                         const std::vector<std::string> &parameters);
  void setAutoindex(std::string autoindex);

  // Our validators for our attributes

  bool isValidHost(std::string host) const;
  bool isValidErrorPages();
  int isValidLocationBlock(LocationBlock &location_block) const;

  // Getters for our attributes
  const std::string &getServerName() const;
  const uint16_t &getPort() const;
  const in_addr_t &getHost() const;
  const size_t &getMaxBodySize() const;
  const std::vector<LocationBlock> &getLocationBlocks() const;
  const std::string &getRoot() const;
  const std::map<short, std::string> &getErrorPages() const;
  const std::string &getIndex() const;
  const bool &getAutoindex() const;
  const std::string getPathErrorPage(short key) const;
  std::vector<LocationBlock>::const_iterator
  getLocationBlockByName(const std::string &name) const;

  static void checkTokenValidity(std::string &token);
  bool checkLocations() const;

  void setupWebserver(void);
  int getFdX(void) const;
};

#endif
