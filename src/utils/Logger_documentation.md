# Logger Module
------------------

Central logging system with colored console output and file logging.

---

## High Level Flow Diagram:
--------------------------

Log Message
     │
     ▼
Logger::debug/info/warn/error()
     │
     ▼
Timestamp + Level Prefix
     │
     ▼
Console (colored)  File (plain text)
     │                    │
     └────────────────────┘
           Written

---

## How to Use This Logger:
-------------------------

| Function | Purpose |
|----------|---------|
| `Logger::debug("message")` | Technical details (epoll, buffers, FDs) |
| `Logger::info("message")` | Normal operations (connections, requests) |
| `Logger::warn("message")` | Non-critical issues (limits, warnings) |
| `Logger::error("message")` | Critical errors (failures, crashes) |
| `Logger::getInstance().enableDebugMode()` | Show all DEBUG messages |
| `Logger::getInstance().disableDebugMode()` | Hide DEBUG messages (default) |
| `Logger::getInstance().setLogToFile(true, "filename")` | Save logs to file |

---

### Example Usage:
-----------------
```cpp : 

#include "Logger.hpp"

// One-time setup (in main)
Logger::getInstance().enableDebugMode();
Logger::getInstance().setLogToFile(true, "webserv.log");

// Use anywhere
Logger::info("Server starting on port 8080");
Logger::debug("Epoll FD created: " + toString(fd));
Logger::error("bind() failed: " + std::string(strerror(errno)));
```