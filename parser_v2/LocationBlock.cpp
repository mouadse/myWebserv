#include "LocationBlock.hpp"

// Default constructor
LocationBlock::LocationBlock(void)
{
    std::cout << "Default constructor called" << std::endl;
    return ;
}

// Copy constructor
LocationBlock::LocationBlock(const LocationBlock &other)
{
    std::cout << "Copy constructor called" << std::endl;
    (void) other;
    return ;
}

// Assignment operator overload
LocationBlock &LocationBlock::operator=(const LocationBlock &other)
{
    std::cout << "Assignment operator called" << std::endl;
    (void) other;
    return (*this);
}

// Destructor
LocationBlock::~LocationBlock(void)
{
    std::cout << "Destructor called" << std::endl;
    return ;
}

