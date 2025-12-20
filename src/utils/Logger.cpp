/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebelkadi <ebelkadi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/08 01:11:47 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/08 01:31:14 by ebelkadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"
#include <cstring>
#include <iomanip>

Logger *Logger::instance = 0;

Logger::Logger() : currentLevel(INFO), logToConsole(true), logToFile(false) {}

Logger::~Logger() {
  if (logFile.is_open()) {
    logFile.close();
  }
}

Logger::Logger(const Logger &) {}
Logger &Logger::operator=(const Logger &) { return *this; }

Logger &Logger::getInstance() {
  if (!instance) {
    instance = new Logger();
  }
  return *instance;
}

void Logger::destroyInstance() {
  if (instance) {
    delete instance;
    instance = 0;
  }
}

std::string Logger::getTimestamp() {
  time_t now = time(0);
  tm *timeinfo = localtime(&now);

  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
  return std::string(buffer);
}

std::string Logger::levelToString(LogLevel level) {
  switch (level) {
  case DEBUG:
    return "DEBUG";
  case INFO:
    return "INFO";
  case WARN:
    return "WARN";
  case ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

void Logger::writeLog(LogLevel level, const std::string &message) {
  if (level < currentLevel)
    return;

  std::stringstream logEntry;
  logEntry << "[" << getTimestamp() << "] " << levelToString(level) << ": "
           << message;

  if (logToConsole) {
    if (level == ERROR) {
      std::cerr << "\033[1;31m" << logEntry.str() << "\033[0m" << std::endl;
    } else if (level == WARN) {
      std::cerr << "\033[1;33m" << logEntry.str() << "\033[0m" << std::endl;
    } else if (level == INFO) {
      std::cout << "\033[1;32m" << logEntry.str() << "\033[0m" << std::endl;
    } else if (level == DEBUG) {
      std::cout << "\033[1;36m" << logEntry.str() << "\033[0m" << std::endl;
    } else {
      std::cout << logEntry.str() << std::endl;
    }
  }

  if (logToFile && logFile.is_open()) {
    logFile << logEntry.str() << std::endl;
    logFile.flush();
  }
}

// Debug mode control
void Logger::enableDebugMode() { currentLevel = DEBUG; }

void Logger::disableDebugMode() { currentLevel = INFO; }

bool Logger::isDebugMode() const { return (currentLevel == DEBUG); }

bool Logger::isDebugEnabled() { return getInstance().isDebugMode(); }

// Configuration
void Logger::setLogLevel(LogLevel level) { currentLevel = level; }

void Logger::setLogToConsole(bool enable) { logToConsole = enable; }

void Logger::setLogToFile(bool enable, const std::string &filename) {
  logToFile = enable;
  if (logToFile) {
    logFile.open(filename.c_str(), std::ios::app);
    if (!logFile.is_open()) {
      std::cerr << "ERROR: Cannot open log file: " << filename << std::endl;
      logToFile = false;
    }
  } else if (logFile.is_open()) {
    logFile.close();
  }
}

// Logging methods
void Logger::debug(const std::string &message) {
  getInstance().writeLog(DEBUG, message);
}

void Logger::info(const std::string &message) {
  getInstance().writeLog(INFO, message);
}

void Logger::warn(const std::string &message) {
  getInstance().writeLog(WARN, message);
}

void Logger::error(const std::string &message) {
  getInstance().writeLog(ERROR, message);
}

// Static convenience methods
void Logger::log(LogLevel level, const std::string &message) {
  getInstance().writeLog(level, message);
}

void Logger::log(const std::string &message) {
  getInstance().writeLog(INFO, message);
}