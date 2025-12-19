/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:15 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/17 21:57:39 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "client.hpp"


Server::Server() 
{
    Logger::log("Creating default server on port 8080");
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) 
    {
        Logger::error("socket() failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->socket function");
    }
    Logger::debug("socket() created FD: " + Helpers::toString(server_fd));
    
    /*  
    When a server closes a connection, the port it was using enters a TIME_WAIT
    state for a brief period to ensure all packets on the network have cleared out. 
    By default, attempting to bind() to that same port immediately results in an "Address
    already in use" error. Enabling SO_REUSEADDR explicitly tells the operating system that 
    it is safe to reuse the address even if it's technically still in that waiting state.
    */
    int yes = 1;
    int setSock = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (setSock < 0)
    {
        Logger::error("setsockopt(SO_REUSEADDR) failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->SetSocket function");
    }
    Logger::debug("SO_REUSEADDR enabled for FD: " + Helpers::toString(server_fd));

    sockaddr_in addr = {};
    addr.sin_port = htons(8080);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        Logger::error("bind() to 0.0.0.0:8080 failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->Bind function");
    }
    Logger::info("Server bound to 0.0.0.0:8080 (FD: " + Helpers::toString(server_fd) + ")");

    if (listen(server_fd, 128) < 0)
    {
        Logger::error("listen() failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->Listen function"); 
    }
    Logger::debug("listen() called with backlog 128");

    // we have two categories of flags associated with file handling : 
    // File descriptor flags : associated with single specific FD eithing a process's FD table
    // File status  flags : associated to the open file description 
    // Here the fctl is setting the file status flags  behaviour to be none blocking.
    if(fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0 )
    {
        Logger::error("fcntl(O_NONBLOCK) failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->Fcntl1 function"); 
    }
    Logger::debug("FD: " + Helpers::toString(server_fd) + " set to non-blocking mode");

    EpollManager::getInstance().add(server_fd, EPOLLIN);
    Logger::debug("Server FD: " + Helpers::toString(server_fd) + " added to epoll with EPOLLIN");
    Logger::info("Default server initialized successfully");
}


Server::Server(const WebserverConfig &cfg)
    : _port(cfg.getPort()), _host(cfg.getHost()), config(cfg)
{
    
    Logger::debug("Creating server with port: " + Helpers::toString(_port) + ", host: " + Helpers::toString(_host));
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) 
    {
        Logger::error("socket() failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->socket function");
    }
    Logger::debug("socket() created FD: " + Helpers::toString(server_fd));
    
    int yes = 1;
    int setSock = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (setSock < 0)
    {
        Logger::error("setsockopt(SO_REUSEADDR) failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->SetSocket function");
    }
    Logger::debug("SO_REUSEADDR enabled for FD: " + Helpers::toString(server_fd));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = _host;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        Logger::error("bind() failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->Bind function");
    }
    if (listen(server_fd, 128) < 0)
    {
        Logger::error("listen() failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->Listen function"); 
    }
    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        Logger::error("fcntl(O_NONBLOCK) failed: " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->Fcntl1 function"); 
    }
    Logger::debug("FD: " + Helpers::toString(server_fd) + " set to non-blocking mode");

    EpollManager::getInstance().add(server_fd, EPOLLIN);

    Logger::info("[Server] Listening on "
              + cfg.getHostString() + ":" 
              + Helpers::toString(_port) 
              + "\n");
}

void Server::acceptClient()
{
    Logger::debug("acceptClient() called on server FD: " + Helpers::toString(server_fd));
    
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int client_fd = accept(server_fd, (sockaddr*)&clientAddr, &clientLen);

    if (client_fd < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            Logger::debug("accept() interrupted/would block on server FD: " + Helpers::toString(server_fd));
            return;
        }
        Logger::error("accept() failed: " + std::string(strerror(errno)));
        throw std::runtime_error("Error during accept");
    }
    Logger::debug("accept() returned client FD: " + Helpers::toString(client_fd));

    // Make non-blocking
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
        Logger::error("fcntl(O_NONBLOCK) failed for client FD: " + Helpers::toString(client_fd) + ": " + std::string(strerror(errno)));
        throw std::runtime_error("There is a problem in the Server->Fcntl1 function");
    }
    Logger::debug("Client FD: " + Helpers::toString(client_fd) + " set to non-blocking");

    clients[client_fd] = new Client(client_fd);
    Logger::debug("Client object created for FD: " + Helpers::toString(client_fd));
    
    EpollManager::getInstance().add(client_fd, EPOLLIN);
    Logger::debug("Client FD: " + Helpers::toString(client_fd) + " added to epoll with EPOLLIN");

    // ---- Convert client IP + port to string ----
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, INET_ADDRSTRLEN);

    int port = ntohs(clientAddr.sin_port);

    Logger::info("Client connected: " +
                std::string(ip) +
                ":" + Helpers::toString(port) +
                " (FD: " + Helpers::toString(client_fd) + ")");
    
    Logger::debug("Total clients connected: " + Helpers::toString(clients.size()));
}

void Server::readClient(int fd)
{
    Client* c = clients[fd];
    std::vector<char> buf(16096);

    while (true)
    {
        int n = read(fd, &buf[0], buf.size());

        if (n > 0)
        {
            buf.resize(n);
            c->request.addData(buf);
            buf.resize(16096);
            Logger::debug("Added " + Helpers::toString(n) + " bytes to request buffer");

            if (c->request.hasError())
            {
                Logger::warn("Request has error, closing client FD: " + Helpers::toString(fd));
                break;
            }

            if (!c->request.isComplete()) {
                Logger::debug("Request not complete, continuing to read...");
                continue;
            }
            Logger::debug("Request complete on FD: " + Helpers::toString(fd));
            break;
        }
        if (n == 0)
        {
            Logger::info("Client FD: " + Helpers::toString(fd) + " disconnected (read 0 bytes)");
            closeClient(fd);
            return;
        }
        return;
    }
    // after receiving the function we call the function handler to process it : 
    RequestHandler handler(config);
    handler.process(c->request, c->response);
    
    // after proccessing, the handler create the response. ( we might have a problem in the case of the cgi)
    std::string res = c->response.toString();
    c->writeBuffer.assign(res.begin(), res.end());
    c->wantWrite = true;
    
    // modify the state of the fd so that we can write on it 
    EpollManager::getInstance().mod(fd, EPOLLOUT);
    Logger::debug("Modified FD: " + Helpers::toString(fd) + " to EPOLLOUT for writing");

    c->request.reset();
}

void Server::writeClient(int fd)
{
    Logger::debug("writeClient() called for FD: " + Helpers::toString(fd));
    
    Client *c = clients[fd];
    Logger::debug("Write buffer size: " + Helpers::toString(c->writeBuffer.size()) + " bytes");

    int n = write(fd, c->writeBuffer.data(), c->writeBuffer.size());
    Logger::debug("write() sent " + Helpers::toString(n) + " bytes on FD: " + Helpers::toString(fd));
    
    if (n < 0) 
    {
        Logger::error("write() failed on FD: " + Helpers::toString(fd) + ": " + std::string(strerror(errno)));
        perror("write failed");
        // handle error / maybe close the client
    } else if (n > 0) 
    {
        // remove the bytes that were sent
        c->writeBuffer.erase(c->writeBuffer.begin(), c->writeBuffer.begin() + n);
        Logger::debug("Write buffer remaining: " + Helpers::toString(c->writeBuffer.size()) + " bytes");
    }

    c->wantWrite = false;
    EpollManager::getInstance().mod(fd, EPOLLIN);
    Logger::debug("Modified FD: " + Helpers::toString(fd) + " back to EPOLLIN for reading");
    
    if (c->writeBuffer.empty()) {
        Logger::info("Response fully sent on FD: " + Helpers::toString(fd));
    } else {
        Logger::debug("Partial write on FD: " + Helpers::toString(fd) + ", " + Helpers::toString(c->writeBuffer.size()) + " bytes remaining");
    }
}

void Server::closeClient(int fd)
{
    Logger::debug("closeClient() called for FD: " + Helpers::toString(fd));
    
    try {
        EpollManager::getInstance().remove(fd);
        Logger::debug("FD: " + Helpers::toString(fd) + " removed from epoll");
    } catch (const std::exception &e) {
        Logger::warn("Failed to remove FD from epoll during closeClient(): " + Helpers::toString(fd) + " (" + e.what() + ")");
    }

    close(fd);
    Logger::debug("close() called on FD: " + Helpers::toString(fd));

    if (clients.count(fd)) {
        delete clients[fd];
        clients.erase(fd);
        Logger::info("Client FD: " + Helpers::toString(fd) + " closed and cleaned up");
        Logger::debug("Remaining clients: " + Helpers::toString(clients.size()));
    } else {
        Logger::warn("closeClient() called for non-existent FD: " + Helpers::toString(fd));
    }
}

bool Server::hasClient(int fd) const {
    bool exists = clients.find(fd) != clients.end();
    Logger::debug("hasClient(FD: " + Helpers::toString(fd) + ") returns: " + (exists ? "true" : "false"));
    return exists;
}
