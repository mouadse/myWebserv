#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

void init_conf_file(int ac, char **av, std::string &filename) {
  // We will be having 3 cases: one no ac it means we will use the default
  // 2nd case is when we have ac == 2 and the last case is when we have ac > 2
  // if ac > 2 then we will print an error message and exit the program
  if (1 == ac) {
    filename = "nginx.conf";
  } else if (2 == ac) {
    filename = av[1];
  } else {
    std::cerr << "Usage: " << av[0] << " [config file path]" << std::endl;
    exit(EXIT_FAILURE);
  }
}

std::string read_config_file(const std::string &file_path) {
  struct stat st;
  // Check if the file exists or permissions are correct
  if (stat(file_path.c_str(), &st) != 0) {
    std::string err_msg = "Error: Unable to find the configuration file ('" +
                          file_path + "'). " + std::string(strerror(errno));
    throw std::runtime_error(err_msg);
  }
  return "";
}

int main(int argc, char *argv[]) {
  std::string config_file;
  init_conf_file(argc, argv, config_file);
  return 0;
}
