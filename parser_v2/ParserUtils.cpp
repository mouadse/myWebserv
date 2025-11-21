#include "ParserUtils.hpp"

// Default constructor
ParserUtils::ParserUtils(void)
{
    std::cout << "Default constructor called" << std::endl;
    return ;
}

// Copy constructor
ParserUtils::ParserUtils(const ParserUtils &other)
{
    std::cout << "Copy constructor called" << std::endl;
    (void) other;
    return ;
}

// Assignment operator overload
ParserUtils &ParserUtils::operator=(const ParserUtils &other)
{
    std::cout << "Assignment operator called" << std::endl;
    (void) other;
    return (*this);
}

// Destructor
ParserUtils::~ParserUtils(void)
{
    std::cout << "Destructor called" << std::endl;
    return ;
}

