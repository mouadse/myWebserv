/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:18 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/20 21:54:53 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../utils/Helpers.hpp"
#include "../utils/Logger.hpp"
#include "EpollManager.hpp"
#include "client.hpp"
#include "config/WebserverConfig.hpp"
#include "http/RequestHandler.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

class Client;
class Server {
public:
  int server_fd;
  uint16_t _port;
  in_addr_t _host;
  WebserverConfig config;

  std::map<int, Client *> clients;
  std::map<int, Client *> cgiFds;

  Server();
  Server(const WebserverConfig &cfg);
  void acceptClient();
  void readClient(int fd);
  void writeClient(int fd);
  void closeClient(int fd);
  bool hasClient(int fd) const;
  bool hasCgiFd(int fd) const;
  void handleCgiEvent(int fd, uint32_t events);
  int nextCgiTimeoutMs() const;
  void checkCgiTimeouts();
};
