#ifndef LOCATIONBLOCK_HPP
# define LOCATIONBLOCK_HPP
# include <iostream>

class LocationBlock
{
    public:
        LocationBlock(void);
        LocationBlock(const LocationBlock& other);
        LocationBlock &operator=(const LocationBlock &other);
        ~LocationBlock();
};

#endif

