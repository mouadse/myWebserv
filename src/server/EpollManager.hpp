/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollManager.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:06 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/20 21:49:51 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLLMANAGER_HPP
#define EPOLLMANAGER_HPP

#include "./utils/Helpers.hpp"
#include "./utils/Logger.hpp"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>

class EpollManager {
private:
  int epfd;

  EpollManager();
  EpollManager(const EpollManager &other);
  EpollManager &operator=(const EpollManager &other);
  ~EpollManager();

  static EpollManager *instance;

public:
  static EpollManager &getInstance();
  static void destroyInstance();

  void add(int fd, uint32_t events);
  void mod(int fd, uint32_t events);
  void remove(int fd);
  int wait(epoll_event *events, int maxEvents, int timeoutMs = -1);
};

#endif
