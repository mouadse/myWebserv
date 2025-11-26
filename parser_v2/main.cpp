#include <iostream>
#include <string>
#include <exception>

#include "ServerConfigParser.hpp" // Assuming ServerConfigParser.hpp is the correct header

int main(int argc, char **argv) {
  std::string config_path = "example.conf";
  if (argc > 1)
    config_path = argv[1];

  try {
    ServerConfigParser parser; // Changed from ConfigParser to ServerConfigParser
    // Assuming createCluster and print methods exist in ServerConfigParser
    parser.createCluster(config_path);
    parser.print(std::cout);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return (1);
  }
  return (0);
}
