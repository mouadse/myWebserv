#include "ServerConfig.hpp"

// Default constructor
ServerConfig::ServerConfig(void)
{
    std::cout << "Default constructor called" << std::endl;
    return ;
}

// Copy constructor
ServerConfig::ServerConfig(const ServerConfig &other)
{
    std::cout << "Copy constructor called" << std::endl;
    (void) other;
    return ;
}

// Assignment operator overload
ServerConfig &ServerConfig::operator=(const ServerConfig &other)
{
    std::cout << "Assignment operator called" << std::endl;
    (void) other;
    return (*this);
}

// Destructor
ServerConfig::~ServerConfig(void)
{
    std::cout << "Destructor called" << std::endl;
    return ;
}

