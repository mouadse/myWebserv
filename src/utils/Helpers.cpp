/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Helpers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebelkadi <ebelkadi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/08 01:49:12 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/08 03:06:53 by ebelkadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Helpers.hpp"
#include "../server/server.hpp"
#include <vector>

Helpers::Helpers() {}

Helpers::Helpers(const Helpers &other) {
    (void)other;
}

Helpers &Helpers::operator=(const Helpers &other) {
    if (this != &other) {
        // no state to copy currently
    }
    return *this;
}

Helpers::~Helpers() {}



Helpers::FdInfo Helpers::findServerByFd(int fd, const std::vector<Server> &servers) {
    FdInfo result;
    
    // First check if it's a server socket
    for (size_t j = 0; j < servers.size(); ++j) {
        if (fd == servers[j].server_fd) {
            result.serverIndex = j;
            result.type = 0; // Server socket
            return result;
        }
    }
    
    // Then check if it's a client socket
    for (size_t i = 0; i < servers.size(); ++i) {
        if (servers[i].hasClient(fd)) {
            result.serverIndex = i;
            result.type = 1; // Client socket
            return result;
        }
    }
    
    result.serverIndex = -1;
    result.type = -1; // Not found
    return result;
}

