#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>

enum LogLevel {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3
};

class Logger {
private:
    Logger();
    Logger(const Logger&);
    Logger& operator=(const Logger&);
    ~Logger();
    
    static Logger* instance;
    
    std::ofstream logFile;
    LogLevel currentLevel;
    bool logToConsole;
    bool logToFile;
    
    std::string getTimestamp();
    std::string levelToString(LogLevel level);
    void writeLog(LogLevel level, const std::string& message);
    
public:
    static Logger& getInstance();
    static void destroyInstance();
    
    // Configuration - keep as instance methods
    void setLogLevel(LogLevel level);
    void setLogToConsole(bool enable);
    void setLogToFile(bool enable, const std::string& filename = "webserv.log");
    
    // Debug mode control - keep as instance methods
    void enableDebugMode();
    void disableDebugMode();
    bool isDebugMode() const;
    
    // Logging methods - MAKE THEM STATIC!
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    
    // Already static convenience methods
    static void log(LogLevel level, const std::string& message);
    static void log(const std::string& message);
};

#endif