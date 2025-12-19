#include "CgiHandler.hpp"

// Default constructor
CgiHandler::CgiHandler(void)
{
    std::cout << "Default constructor called" << std::endl;
    return ;
}

// Copy constructor
CgiHandler::CgiHandler(const CgiHandler &other)
{
    std::cout << "Copy constructor called" << std::endl;
    (void) other;
    return ;
}

// Assignment operator overload
CgiHandler &CgiHandler::operator=(const CgiHandler &other)
{
    std::cout << "Assignment operator called" << std::endl;
    (void) other;
    return (*this);
}

// Destructor
CgiHandler::~CgiHandler(void)
{
    std::cout << "Destructor called" << std::endl;
    return ;
}

