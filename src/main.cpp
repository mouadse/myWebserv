/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:10 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/13 20:45:03 by ebelkadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config/ServerConfigParser.hpp"
#include "server/server.hpp"
#include "utils/Helpers.hpp"
#include "utils/SignalHandling.hpp"
#include <iostream>


static std::vector<Server> setServer(int argc,char **argv) 
{
  std::string config_path = "./config/example.conf"; // Default config path
  if (argc > 1)
    config_path = argv[1];

  Logger::log("Loading configuration from: " + config_path);

  ServerConfigParser parser;
  parser.createCluster(config_path);

  std::vector<WebserverConfig> configs = parser.getServers();

  Logger::log("Successfully parsed " + Helpers::toString(configs.size()) + " server(s) from configuration file.");

  if (configs.empty()) {
    Logger::error("No server blocks found in configuration file");
    throw std::runtime_error(
        "Config file parsed successfully but contains no server blocks.");
  }

//   parser.print(std::cout); // Uncomment for detailed config output

  // Create server instances from configurations
  std::vector<Server> servers;
  uint16_t port;
  for (size_t i = 0; i < configs.size(); i++) 
  {
    port = configs[i].getPort();
    Logger::debug("Creating server " + Helpers::toString(i + 1) + 
                  " with port " + Helpers::toString(port));
    servers.push_back(Server(configs[i]));
  }
  
  Logger::log("All " + Helpers::toString(servers.size()) + " server(s) initialized successfully");
  return servers;

}

static void runServer(std::vector<Server> &servers) 
{
    EpollManager &ep = EpollManager::getInstance();
    epoll_event events[64];
    SignalHandling signals;

    Logger::log("Server starting event loop...");

    signals.install();
    ep.add(signals.getReadFd(), EPOLLIN);

    bool running = true;
    while (running) {
        int n = ep.wait(events, 64);
        if (n <= 0)
            continue;
        
        for (int i = 0; i < n; i++) 
        {
            int fd = events[i].data.fd;

            if (fd == signals.getReadFd()) {
                signals.consume();
                Logger::warn("Stop requested (signal " + Helpers::toString(signals.lastSignal()) + ")");
                running = false;
                break;
            }

            Helpers::FdInfo info = Helpers::findServerByFd(fd, servers);
            
            if (info.serverIndex < 0) {
                Logger::warn("Received event for unknown fd: " + Helpers::toString(fd));
                continue;
            }
            
            Server &server = servers[info.serverIndex];
            
            if (info.type == 0) {
                // Server socket - accept new connection
                server.acceptClient();
            }
            else if (info.type == 1) {
                // Client socket - handle I/O
                if (events[i].events & EPOLLIN) {
                    server.readClient(fd); // i might change the name of this function to handle request cuz that what id does
                    
                } 
                else if (events[i].events & EPOLLOUT) {
                    server.writeClient(fd);
            }
        }
        }
    }

    signals.uninstall();
    SignalHandling::shutdownCluster(servers);
}


int main(int argc, char **argv) {
    // Logger::getInstance().enableDebugMode(); // enabling debuging mode
    std::vector<Server> servers;

    
    try {
        // Parse config files and create server(s) :
        servers = setServer(argc, argv);
        // Run the event loop
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
