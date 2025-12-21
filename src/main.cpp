/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:10 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/20 22:01:07 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config/ServerConfigParser.hpp"
#include "server/server.hpp"
#include "utils/Helpers.hpp"
#include "utils/SignalHandling.hpp"
#include <cerrno>
#include <cstring>
#include <iostream>

static std::vector<Server> setServer(int argc, char **argv) {
  std::string config_path = "./config/demo.conf";
  if (argc > 1)
    config_path = argv[1];

  Logger::log("Loading configuration from: " + config_path);

  ServerConfigParser parser;
  parser.createCluster(config_path);

  std::vector<WebserverConfig> configs = parser.getServers();

  Logger::log("Successfully parsed " + Helpers::toString(configs.size()) +
              " server(s) from configuration file.");

  if (configs.empty()) {
    Logger::error("No server blocks found in configuration file");
    throw std::runtime_error(
        "Config file parsed successfully but contains no server blocks.");
  }
  //   parser.print(std::cout);

  std::vector<Server> servers;
  uint16_t port;
  for (size_t i = 0; i < configs.size(); i++) {
    port = configs[i].getPort();
    Logger::debug("Creating server " + Helpers::toString(i + 1) +
                  " with port " + Helpers::toString(port));
    servers.push_back(Server(configs[i]));
  }

  Logger::log("All " + Helpers::toString(servers.size()) +
              " server(s) initialized successfully");
  return servers;
}

static void runServer(std::vector<Server> &servers) {
  EpollManager &ep = EpollManager::getInstance();
  epoll_event events[64];
  SignalHandling signals;

  Logger::log("Server starting event loop...");

  signals.install();
  ep.add(signals.getReadFd(), EPOLLIN);

  bool running = true;
  while (running) {
    int timeoutMs = -1;
    for (size_t s = 0; s < servers.size(); ++s) {
      int serverTimeout = servers[s].nextCgiTimeoutMs();
      if (serverTimeout >= 0 && (timeoutMs < 0 || serverTimeout < timeoutMs)) {
        timeoutMs = serverTimeout;
      }
    }

    int n = ep.wait(events, 64, timeoutMs);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      Logger::error("epoll_wait failed: " + std::string(strerror(errno)) +
                    " (errno: " + Helpers::toString(errno) + ")");
      break;
    }

    if (n > 0) {
      for (int i = 0; i < n; i++) {
        int fd = events[i].data.fd;

        if (fd == signals.getReadFd()) {
          signals.consume();
          Logger::warn("Stop requested (signal " +
                       Helpers::toString(signals.lastSignal()) + ")");
          running = false;
          break;
        }

        Helpers::FdInfo info = Helpers::findServerByFd(fd, servers);

        if (info.serverIndex < 0) {
          Logger::warn("Received event for unknown fd: " +
                       Helpers::toString(fd));
          continue;
        }

        Server &server = servers[info.serverIndex];

        if (info.type == 0) {
          server.acceptClient();
        } else if (info.type == 1) {
          if (events[i].events & (EPOLLHUP | EPOLLERR)) {
            server.closeClient(fd);
            continue;
          }
          if (events[i].events & EPOLLRDHUP) {
            server.readClient(fd);
            if (server.hasClient(fd))
              server.closeClient(fd);
            continue;
          }
          if (events[i].events & EPOLLIN) {
            server.readClient(fd);
          }
          if ((events[i].events & EPOLLOUT) && server.hasClient(fd)) {
            server.writeClient(fd);
          }
        } else if (info.type == 2) {
          server.handleCgiEvent(fd, events[i].events);
        }
      }
    }

    for (size_t s = 0; s < servers.size(); ++s)
      servers[s].checkCgiTimeouts();

    if (n == 0)
      continue;
  }

  signals.uninstall();
  SignalHandling::shutdownCluster(servers);
}

int main(int argc, char **argv) {
  Logger::getInstance().enableDebugMode(); // enabling debuging mode
  std::vector<Server> servers;

  try {
    servers = setServer(argc, argv);
    runServer(servers);

  } catch (const std::exception &e) {
    Logger::error(std::string("Fatal error: ") + e.what());
    try {
      SignalHandling::shutdownCluster(servers);
    } catch (...) {
    }
    return 1;
  }

  return 0;
}
