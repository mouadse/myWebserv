/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebelkadi <ebelkadi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:18 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/11/29 19:36:57 by ebelkadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once 

#include "../utils/Helpers.hpp"
#include "../utils/Logger.hpp"
#include "EpollManager.hpp"
#include "config/WebserverConfig.hpp"
#include "http/RequestHandler.hpp"
#include "client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <cstring>
#include <map>
#include <cstdio>
#include <cstdlib> 
#include <iostream>

class Client;
class Server 
{
    public :
        int server_fd;
        uint16_t _port;
        in_addr_t _host;
        WebserverConfig config; // This is added by webserv
        
        // EpollManager ep;
        std::map<int, Client*> clients;
        

        Server(); // for the default constructor the port is 8080 and host INADDR_ANY
        Server(const WebserverConfig &cfg);
        void acceptClient();
        void readClient(int fd); // change this to a vector ? 
        void writeClient(int fd);
        void closeClient(int fd);
        bool hasClient(int fd) const ;

};