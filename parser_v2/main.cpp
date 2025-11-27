#include <exception>
#include <iostream>
#include <string>

#include "ServerConfigParser.hpp"

int main(int argc, char **argv) {
  std::string config_path = "example.conf";
  if (argc > 1)
    config_path = argv[1];

  try {
    ServerConfigParser parser;
    parser.createCluster(config_path);
    std::vector<WebserverConfig> servers = parser.getServers();
    std::cout << "Successfully parsed " << servers.size()
              << " server(s) from configuration file." << std::endl;
    // parser.print(std::cout);
    // Setting up the first server as an example
    if (!servers.empty())
      servers[0].setupWebserver();
    std::cout << "Server name is " << servers[0].getServerName() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return (1);
  }
  return (0);
}
