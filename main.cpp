#include <iostream>
#include <string>

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

int main(int argc, char *argv[]) {
  // This is a simple demo

  return 0;
}
