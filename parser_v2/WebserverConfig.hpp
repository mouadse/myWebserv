#ifndef WEBSERVERCONFIG_HPP
#define WEBSERVERCONFIG_HPP

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
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
  unsigned long _max_body_size;
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
  void setIdex(std::string index);
};

#endif
