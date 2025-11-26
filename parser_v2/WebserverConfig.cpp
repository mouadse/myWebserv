#include "WebserverConfig.hpp"

// Default constructor
WebserverConfig::WebserverConfig(void) {
  std::cout << "Default constructor called" << std::endl;
  return;
}

// Copy constructor
WebserverConfig::WebserverConfig(const WebserverConfig &other) {
  std::cout << "Copy constructor called" << std::endl;
  (void)other;
  return;
}

// Assignment operator overload
WebserverConfig &WebserverConfig::operator=(const WebserverConfig &other) {
  std::cout << "Assignment operator called" << std::endl;
  (void)other;
  return (*this);
}

// Destructor
WebserverConfig::~WebserverConfig(void) {
  std::cout << "Destructor called" << std::endl;
  return;
}
