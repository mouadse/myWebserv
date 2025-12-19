/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollManager.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:38:03 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/17 21:58:51 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "EpollManager.hpp"

// Initialize the static member :
EpollManager *EpollManager::instance = 0;

// private constructor :
EpollManager::EpollManager() {
  Logger::debug("EpollManager constructor called");

  epfd = epoll_create1(0);
  Logger::debug("epoll_create1() returned FD: " + Helpers::toString(epfd));

  if (epfd < 0) {
    Logger::error("epoll_create1() failed: " + std::string(strerror(errno)));
    throw std::runtime_error("epoll_create1() failed");
  }

  Logger::info("Epoll instance created successfully (FD: " +
               Helpers::toString(epfd) + ")");
}

EpollManager::EpollManager(const EpollManager &other) {
  (void)other;
  Logger::warn("EpollManager copy constructor called (should not happen)");
}

EpollManager &EpollManager::operator=(const EpollManager &other) {
  (void)other;
  Logger::warn("EpollManager assignment operator called (should not happen)");
  return *this;
}

EpollManager::~EpollManager() {
  Logger::debug("EpollManager destructor called");

  if (epfd >= 0) {
    Logger::debug("Closing epoll FD: " + Helpers::toString(epfd));
    close(epfd);
    Logger::info("Epoll FD closed: " + Helpers::toString(epfd));
  } else {
    Logger::debug("Epoll FD already closed or invalid: " +
                  Helpers::toString(epfd));
  }
}

// get single instance
EpollManager &EpollManager::getInstance() {
  Logger::debug("EpollManager::getInstance() called");

  if (instance == 0) {
    Logger::info("Creating new EpollManager instance");
    instance = new EpollManager();
  } else {
    Logger::debug("Returning existing EpollManager instance");
  }

  return *instance;
}

// in case i want to clean up the instance :
void EpollManager::destroyInstance() {
  Logger::debug("EpollManager::destroyInstance() called");

  if (instance != 0) {
    Logger::info("Destroying EpollManager instance");
    delete instance;
    instance = 0;
    Logger::debug("EpollManager instance set to NULL");
  } else {
    Logger::warn("destroyInstance() called but instance is already NULL");
  }
}

// Crud functions :

void EpollManager::add(int fd, uint32_t events) {
  Logger::debug("EpollManager::add() called - FD: " + Helpers::toString(fd) +
                ", events: " + Helpers::toString(events));

  if ((epfd < 0) || (fd < 0)) {
    Logger::error("Invalid FD in add() - epfd: " + Helpers::toString(epfd) +
                  ", fd: " + Helpers::toString(fd));
    throw std::runtime_error(
        "There is a problem in the EpollManager->add function");
  }

  epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  int result = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

  if (result < 0) {
    Logger::error(
        "epoll_ctl(EPOLL_CTL_ADD) failed for FD: " + Helpers::toString(fd) +
        " - Error: " + std::string(strerror(errno)));
    throw std::runtime_error("epoll_ctl ADD failed");
  }

  // Log event flags
  std::string eventStr;
  if (events & EPOLLIN)
    eventStr += "EPOLLIN ";
  if (events & EPOLLOUT)
    eventStr += "EPOLLOUT ";
  if (events & EPOLLERR)
    eventStr += "EPOLLERR ";
  if (events & EPOLLHUP)
    eventStr += "EPOLLHUP ";
  if (events & EPOLLET)
    eventStr += "EPOLLET ";

  Logger::info("Added FD: " + Helpers::toString(fd) + " to epoll (FD: " +
               Helpers::toString(epfd) + ") with events: " + eventStr);
}

void EpollManager::mod(int fd, uint32_t events) {
  Logger::debug("EpollManager::mod() called - FD: " + Helpers::toString(fd) +
                ", events: " + Helpers::toString(events));

  if ((epfd < 0) || (fd < 0)) {
    Logger::error("Invalid FD in mod() - epfd: " + Helpers::toString(epfd) +
                  ", fd: " + Helpers::toString(fd));
    throw std::runtime_error(
        "There is a problem in the EpollManager->mod function");
  }

  epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  int result = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);

  if (result < 0) {
    Logger::error(
        "epoll_ctl(EPOLL_CTL_MOD) failed for FD: " + Helpers::toString(fd) +
        " - Error: " + std::string(strerror(errno)));
    throw std::runtime_error("epoll_ctl MOD failed");
  }

  // Log event flags
  std::string eventStr;
  if (events & EPOLLIN)
    eventStr += "EPOLLIN ";
  if (events & EPOLLOUT)
    eventStr += "EPOLLOUT ";
  if (events & EPOLLERR)
    eventStr += "EPOLLERR ";
  if (events & EPOLLHUP)
    eventStr += "EPOLLHUP ";
  if (events & EPOLLET)
    eventStr += "EPOLLET ";

  Logger::info("Modified FD: " + Helpers::toString(fd) +
               " in epoll to events: " + eventStr);
}

void EpollManager::remove(int fd) {
  Logger::debug("EpollManager::remove() called - FD: " + Helpers::toString(fd));

  if ((epfd < 0) || (fd < 0)) {
    Logger::error("Invalid FD in remove() - epfd: " + Helpers::toString(epfd) +
                  ", fd: " + Helpers::toString(fd));
    throw std::runtime_error(
        "There is a problem in the EpollManager->remove function");
  }

  int result = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);

  if (result < 0) {
    Logger::error(
        "epoll_ctl(EPOLL_CTL_DEL) failed for FD: " + Helpers::toString(fd) +
        " - Error: " + std::string(strerror(errno)));
    throw std::runtime_error("epoll_ctl DEL failed");
  }

  Logger::info("Removed FD: " + Helpers::toString(fd) +
               " from epoll (FD: " + Helpers::toString(epfd) + ")");
}

int EpollManager::wait(epoll_event *events, int maxEvents) {
  Logger::debug("EpollManager::wait() called - maxEvents: " +
                Helpers::toString(maxEvents));

  if (epfd < 0) {
    Logger::error("Epoll FD invalid in wait(): " + Helpers::toString(epfd));
    return -1;
  }

  int ready = epoll_wait(epfd, events, maxEvents, -1);
  Logger::debug("epoll_wait() returned " + Helpers::toString(ready) +
                " ready events");

  if (ready < 0) {
    if (errno == EINTR) {
      Logger::debug("epoll_wait() interrupted by signal (EINTR), continuing");
      return 0;
    }
    Logger::error("epoll_wait() failed: " + std::string(strerror(errno)));
    return ready;
  }

  if (ready > 0) {
    Logger::debug("Processing " + Helpers::toString(ready) + " epoll events");

    // Log each ready event in debug mode
    for (int i = 0; i < ready; i++) {
      std::string eventStr;
      if (events[i].events & EPOLLIN)
        eventStr += "EPOLLIN ";
      if (events[i].events & EPOLLOUT)
        eventStr += "EPOLLOUT ";
      if (events[i].events & EPOLLERR)
        eventStr += "EPOLLERR ";
      if (events[i].events & EPOLLHUP)
        eventStr += "EPOLLHUP ";

      Logger::debug("Event " + Helpers::toString(i) + ": FD " +
                    Helpers::toString(events[i].data.fd) + " - " + eventStr);
    }
  }

  return ready;
}
