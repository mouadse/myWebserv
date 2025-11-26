#ifndef SERVERCONFIGPARSER_HPP
#define SERVERCONFIGPARSER_HPP
#include <iostream>
#include <string>
#include <vector>

#include "WebserverConfig.hpp"

class ServerConfigParser {
private:
  std::vector<WebserverConfig> _servers;
  std::vector<std::string> _config_lines;
  size_t _num_of_servers;

  void _parseServerContent(const std::string &config, WebserverConfig &server);
  void _parseLocationBlock(const std::vector<std::string> &lines, size_t &index,
                           WebserverConfig &server);

public:
  ServerConfigParser(void);
  ServerConfigParser(const ServerConfigParser &other);
  ServerConfigParser &operator=(const ServerConfigParser &other);
  ~ServerConfigParser();

  int createCluster(const std::string &config_path);
  void splitServers(std::string &content);
  void removeComments(std::string &content);
  void removeWhitespaces(std::string &content);
  size_t locateServerStart(std::string &content, size_t pos);
  size_t locateServerEnd(std::string &content, size_t pos);
  void createServer(std::string &server_config, WebserverConfig &server);
  void checkServers(void);
  std::vector<WebserverConfig> getServers() const;
  int print(std::ostream &out) const;
};

#endif
