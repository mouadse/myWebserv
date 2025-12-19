
#pragma once

#include "server/server.hpp"
#include <signal.h>

class SignalHandling {
public:
  SignalHandling();
  ~SignalHandling();

  void install();
  void uninstall();

  int getReadFd() const;
  bool stopRequested() const;
  int lastSignal() const;

  void consume();

  static void shutdownCluster(std::vector<Server> &servers);

private:
  SignalHandling(const SignalHandling &other);
  SignalHandling &operator=(const SignalHandling &other);

  typedef void (*SignalFn)(int);

  int _pipeRead;
  int _pipeWrite;
  bool _installed;

  SignalFn _oldInt;
  SignalFn _oldTerm;
  SignalFn _oldQuit;
  SignalFn _oldPipe;

  static int s_writeFd;
  static volatile sig_atomic_t s_stopRequested;
  static volatile sig_atomic_t s_lastSignal;
  static SignalHandling *s_instance;

  static void handleStopSignal(int signo);
  static void safeCloseFd(int &fd);
  static void setNonBlockingCloexecOrThrow(int fd);
};
