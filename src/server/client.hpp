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

#include <arpa/inet.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../http/HTTPRequest.hpp"
#include "../http/HTTPResponse.hpp"
#include "./utils/Helpers.hpp"
#include "./utils/Logger.hpp"

class Client {
public:
  int fd;              // the client's socket
  HTTPRequest request; // to handle the incoming request
  HttpResponse response;
  std::vector<char> writeBuffer; // data TO client   (server response)
  size_t writeOffset;   // current position in writeBuffer (avoids O(n) erase)
  bool wantWrite;       // whether we should switch to EPOLLOUT
  bool closeAfterWrite; // close connection after flushing buffer (Connection:
                        // close)
  Client(int fd);
};

#endif