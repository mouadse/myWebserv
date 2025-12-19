/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollManager.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:06 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/08 12:56:07 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLLMANAGER_HPP
#define EPOLLMANAGER_HPP

#include <sys/epoll.h>
#include <stdexcept>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "./utils/Logger.hpp"
#include "./utils/Helpers.hpp"

class EpollManager 
{
    private:

    int epfd;

    // A private constructor to prevent direct instantiation and copying the epollmanager.
    EpollManager();
    EpollManager(const EpollManager &other);
    EpollManager& operator=(const EpollManager &other);
    ~EpollManager();

    // a static point to the single instance :
    static EpollManager* instance;

    public:

    // static method to get the single instance and to destroy it.
    static EpollManager& getInstance();
    static void destroyInstance();

    // Crud operation of the epoll instance :
    void add(int fd, uint32_t events);
    void mod(int fd, uint32_t events);
    void remove(int fd);
    int wait(epoll_event *events, int maxEvents);
    
};



# endif