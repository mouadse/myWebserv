#include "ServerConfigParser.hpp"

// Default constructor
ServerConfigParser::ServerConfigParser(void)
{
    std::cout << "Default constructor called" << std::endl;
    return ;
}

// Copy constructor
ServerConfigParser::ServerConfigParser(const ServerConfigParser &other)
{
    std::cout << "Copy constructor called" << std::endl;
    (void) other;
    return ;
}

// Assignment operator overload
ServerConfigParser &ServerConfigParser::operator=(const ServerConfigParser &other)
{
    std::cout << "Assignment operator called" << std::endl;
    (void) other;
    return (*this);
}

// Destructor
ServerConfigParser::~ServerConfigParser(void)
{
    std::cout << "Destructor called" << std::endl;
    return ;
}

