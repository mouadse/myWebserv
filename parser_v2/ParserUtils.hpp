#ifndef PARSERUTILS_HPP
# define PARSERUTILS_HPP
# include <iostream>

class ParserUtils
{
    public:
        ParserUtils(void);
        ParserUtils(const ParserUtils& other);
        ParserUtils &operator=(const ParserUtils &other);
        ~ParserUtils();
};

#endif

