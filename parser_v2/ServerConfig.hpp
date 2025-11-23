#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP
# include <iostream>

class ServerConfig
{
    public:
        ServerConfig(void);
        ServerConfig(const ServerConfig& other);
        ServerConfig &operator=(const ServerConfig &other);
        ~ServerConfig();
};

#endif

