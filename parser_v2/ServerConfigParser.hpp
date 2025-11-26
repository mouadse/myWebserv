#ifndef SERVERCONFIGPARSER_HPP
# define SERVERCONFIGPARSER_HPP
# include <iostream>

class ServerConfigParser
{
    public:
        ServerConfigParser(void);
        ServerConfigParser(const ServerConfigParser& other);
        ServerConfigParser &operator=(const ServerConfigParser &other);
        ~ServerConfigParser();
};

#endif

