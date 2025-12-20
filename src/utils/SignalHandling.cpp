
#include "SignalHandling.hpp"
#include "utils/Logger.hpp"

#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>

int SignalHandling::s_writeFd = -1;
volatile sig_atomic_t SignalHandling::s_stopRequested = 0;
volatile sig_atomic_t SignalHandling::s_lastSignal = 0;
SignalHandling *SignalHandling::s_instance = NULL;

SignalHandling::SignalHandling()
    : _pipeRead(-1), _pipeWrite(-1), _installed(false), _oldInt(SIG_DFL),
      _oldTerm(SIG_DFL), _oldQuit(SIG_DFL), _oldPipe(SIG_DFL) {}

SignalHandling::~SignalHandling() { uninstall(); }

void SignalHandling::safeCloseFd(int &fd) {
  if (fd >= 0) {
    ::close(fd);
    fd = -1;
  }
}

void SignalHandling::setNonBlockingCloexecOrThrow(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    throw std::runtime_error("fcntl(F_GETFL) failed");
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    throw std::runtime_error("fcntl(F_SETFL, O_NONBLOCK) failed");

  int fdflags = fcntl(fd, F_GETFD, 0);
  if (fdflags < 0)
    throw std::runtime_error("fcntl(F_GETFD) failed");
  if (fcntl(fd, F_SETFD, fdflags | FD_CLOEXEC) < 0)
    throw std::runtime_error("fcntl(F_SETFD, FD_CLOEXEC) failed");
}

void SignalHandling::handleStopSignal(int signo) {
  s_stopRequested = 1;
  s_lastSignal = signo;

  if (s_writeFd >= 0) {
    unsigned char byte = 1;
    (void)::write(s_writeFd, &byte, 1);
  }
}

void SignalHandling::install() {
  if (_installed)
    return;

  if (s_instance != NULL && s_instance != this)
    throw std::runtime_error(
        "Only one SignalHandling instance may be installed at a time");

  int fds[2];
  if (::pipe(fds) < 0)
    throw std::runtime_error("pipe() failed");

  _pipeRead = fds[0];
  _pipeWrite = fds[1];

  try {
    setNonBlockingCloexecOrThrow(_pipeRead);
    setNonBlockingCloexecOrThrow(_pipeWrite);
  } catch (...) {
    safeCloseFd(_pipeRead);
    safeCloseFd(_pipeWrite);
    throw;
  }

  s_writeFd = _pipeWrite;
  s_stopRequested = 0;
  s_lastSignal = 0;

  _oldInt = ::signal(SIGINT, &SignalHandling::handleStopSignal);
  _oldTerm = ::signal(SIGTERM, &SignalHandling::handleStopSignal);
  _oldQuit = ::signal(SIGQUIT, &SignalHandling::handleStopSignal);
  _oldPipe = ::signal(SIGPIPE, SIG_IGN);

  _installed = true;
  s_instance = this;
  Logger::info("SignalHandling installed (SIGINT/SIGTERM/SIGQUIT stop, SIGPIPE "
               "ignored");
}

void SignalHandling::uninstall() {
  if (!_installed)
    return;

  ::signal(SIGINT, _oldInt);
  ::signal(SIGTERM, _oldTerm);
  ::signal(SIGQUIT, _oldQuit);
  ::signal(SIGPIPE, _oldPipe);

  s_writeFd = -1;
  safeCloseFd(_pipeRead);
  safeCloseFd(_pipeWrite);

  _installed = false;
  s_instance = NULL;
  Logger::info("SignalHandling uninstalled");
}

int SignalHandling::getReadFd() const { return _pipeRead; }

bool SignalHandling::stopRequested() const { return s_stopRequested != 0; }

int SignalHandling::lastSignal() const {
  return static_cast<int>(s_lastSignal);
}

void SignalHandling::consume() {
  if (_pipeRead < 0)
    return;

  char buf[128];
  while (true) {
    ssize_t n = ::read(_pipeRead, buf, sizeof(buf));
    if (n > 0)
      continue;
    break;
  }
}

static void closeClientNoThrow(Server &server, int fd) {
  try {
    EpollManager::getInstance().remove(fd);
  } catch (...) {
  }

  if (fd >= 0)
    ::close(fd);

  if (server.clients.count(fd)) {
    delete server.clients[fd];
    server.clients.erase(fd);
  }
}

static void closeServerSocketNoThrow(Server &server) {
  int fd = server.server_fd;
  if (fd < 0)
    return;

  try {
    EpollManager::getInstance().remove(fd);
  } catch (...) {
  }

  ::close(fd);
  server.server_fd = -1;
}

void SignalHandling::shutdownCluster(std::vector<Server> &servers) {
  Logger::warn("Graceful shutdown starting...");

  for (size_t i = 0; i < servers.size(); ++i) {
    Server &server = servers[i];

    std::vector<int> clientFds;
    clientFds.reserve(server.clients.size());
    for (std::map<int, Client *>::iterator it = server.clients.begin();
         it != server.clients.end(); ++it)
      clientFds.push_back(it->first);

    for (size_t j = 0; j < clientFds.size(); ++j)
      closeClientNoThrow(server, clientFds[j]);

    closeServerSocketNoThrow(server);
  }

  EpollManager::destroyInstance();
  Logger::warn("Graceful shutdown complete");
}
