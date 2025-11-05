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
  // Check if the provided path is a regular file
  if (!S_ISREG(st.st_mode)) {
    std::string err_msg =
        "Error: The path '" + file_path + "' is not a regular file.";
    throw std::runtime_error(err_msg);
  }
  std::ifstream file_stream(file_path.c_str());
  if (!file_stream.is_open()) {
    std::string err_msg = "Error: Unable to open the configuration file ('" +
                          file_path + "'). " + std::string(strerror(errno));
    throw std::runtime_error(err_msg);
  }
  std::string file_content;
  std::string line;
  while (std::getline(file_stream, line)) {
    // Optional: strip Windows CR if present to normalize line endings.
    if (!line.empty() && line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1);
    }
    file_content += line;
    file_content.push_back('\n'); // preserve your original behavior
  }
  if (file_stream.bad()) {
    std::string err_msg = "Error: An error occurred while reading the file ('" +
                          file_path + "'). " + std::string(strerror(errno));
    throw std::runtime_error(err_msg);
  }
  file_stream.close();
  return file_content;
}

int main(int argc, char *argv[]) {
  std::string config_file;
  init_conf_file(argc, argv, config_file);
  try {
    std::string config_content = read_config_file(config_file);
    std::cout << "Configuration file content:\n" << config_content << std::endl;
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
