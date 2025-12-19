# HTTPRequest Parsing Module:
----------------------------

This module parses incoming HTTP requests into structured components. It handles request line, headers, and body, including chunked transfer encoding. Data can be fed incrementally as it arrives from the socket.

---

## High Level Flow Diagram:
--------------------------

Incoming Socket Data
         │
         ▼
      Buffer
         │  (parsing by chunks)
         ▼
  Parsing State 
 ┌─────────────┐
 │ START_LINE  │  → parse method, target, version
 │ HEADERS     │  → parse key-value headers
 │ BODY        │  → parse normal or chunked body
 │ DONE        │  → request fully parsed
 │ ERROR       │  → invalid request detected
 └─────────────┘
         │
         ▼
Structured HTTPRequest Object
(method, target, version, headers(key:value pairs), body, query params)

---


## How to Use This Parser:
-------------------------

Feed incoming data from sockets to the parser and access parsed components:

| Function                            | Purpose                                  |
| ----------------------------------- | ---------------------------------------- |
| `addData()`                         | Feed incoming data to the parser         |
| `isComplete()`                      | Check if the request is fully parsed     |
| `hasError()`                        | Check if a parsing error occurred        |
| `getMethod()`                       | Return HTTP method (GET, POST,Delete.)   |
| `getTarget()`                       | Return request URL path                  |
| `getVersion()`                      | Return HTTP version (e.g., HTTP/1.1)     |
| `getHeader(key)`                    | access specific header value             |
| `getBody()` / `getBodyAsString()`   | Return body (as a vector<char> or string)|
| `getQueryParam(key)`                | Access URL query parameters              |
| `reset()`                           | Reset object to parse a new request      |
| `getErrorMessage()`                 | return error message if parsing failed   |
| `printRequest()`                    | Print the whole request

---

### Example Usage:
-----------------
#include "HTTPRequest.hpp"

HTTPRequest request;

// Feed partial data from socket after receiving/reading

request.addData(buffer_chunk1);
request.addData(buffer_chunk2);

if (request.isComplete()) {
    std::cout << "Method: " << request.getMethod() << "\n";
    std::cout << "Target: " << request.getTarget() << "\n";
    std::cout << "Body: " << request.getBodyAsString() << "\n";
} else if (request.hasError()) {
    std::cerr << "Error parsing request: " << request.getErrorMessage() << "\n";
}
