/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:37:51 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/08 14:14:37 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "client.hpp"

Client::Client(int fd) {
  if (Logger::isDebugEnabled())
    Logger::debug("Client constructor called for FD: " + Helpers::toString(fd));

  // 1. Validate FD
  if (fd < 0) {
    Logger::error("Invalid client file descriptor: " + Helpers::toString(fd));
    throw std::runtime_error("Invalid client file descriptor");
  }

  this->fd = fd;
  if (Logger::isDebugEnabled())
    Logger::debug("Client FD set to: " + Helpers::toString(this->fd));

  // 2. Initialize state safely
  this->wantWrite = false;
  this->writeOffset = 0;
  this->closeAfterWrite = false;
  this->fileResponse = false;
  this->streaming = false;
  this->streamFd = -1;
  this->streamRemaining = 0;
  this->streamBufferOffset = 0;
  this->streamBufferSize = 0;
  this->streamBuffer.resize(STREAM_CHUNK_SIZE);
  if (Logger::isDebugEnabled())
    Logger::debug("Client state initialized");

  // this->writeBuffer.clear();

  // 3. Pre-reserve buffer sizes (security: predictable memory use)
  // this->writeBuffer.  // enough for your HTTP response

  if (Logger::isDebugEnabled())
    Logger::debug("Client object created for FD: " + Helpers::toString(fd));
}

// client for the test

// int main()
// {

//     int sock = socket(AF_INET, SOCK_STREAM,0); // 0 means choose the protocol
//     automaticaly sockaddr_in addr = {};

//     addr.sin_family = AF_INET;
//     addr.sin_port = (8080);
//     inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);

//     connect(sock, (sockaddr*) &addr, sizeof(addr));

//     write(sock, "GET / HTTP/1.1\r\n\r\n", 18);

//     char buffer[1024];
//     int n = read(sock, buffer, 1024);
//     buffer[n] = '\0';

//     std::cout << buffer << std::endl;
//     close(sock);

//     return 0;
// }
