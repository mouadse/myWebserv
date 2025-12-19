/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:37:59 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/08 12:52:08 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP



#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "./utils/Logger.hpp"
#include "./utils/Helpers.hpp"
#include "../http/HTTPRequest.hpp"
#include "../http/HTTPResponse.hpp"

class Client
{
public:
    int fd;               // the client's socket
    HTTPRequest request;   // to handle the incoming request
    HttpResponse response;
    std::vector<char> writeBuffer; // data TO client   (server response)

    bool wantWrite;           // whether we should switch to EPOLLOUT
    Client(int fd);

};

#endif