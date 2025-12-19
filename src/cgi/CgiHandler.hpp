#ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP
# include <iostream>

class CgiHandler
{
    public:
        CgiHandler(void);
        CgiHandler(const CgiHandler& other);
        CgiHandler &operator=(const CgiHandler &other);
        ~CgiHandler();
};

#endif

