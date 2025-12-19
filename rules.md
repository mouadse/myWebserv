# Webserv

## Overview

Implement a fully functional HTTP server in **C++98**, compatible with standard web browsers, following a subset of the HTTP specification.

Executable:

```bash
./webserv [configuration_file]
```

---

## Project Rules

* Must **never crash** or terminate unexpectedly.
* Must compile with **c++**, flags: `-Wall -Wextra -Werror -std=c++98`.
* **No external libraries** (including Boost).
* Must provide a **Makefile** with rules: `NAME`, `all`, `clean`, `fclean`, `re`.

---

## Authorized Functions

All functionality must be implemented in C++98. Authorized system calls include (non-exhaustive):

* Process / FD: `fork`, `execve`, `pipe`, `dup`, `dup2`, `close`, `fcntl`
* Sockets / Network: `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `getaddrinfo`, `freeaddrinfo`, `setsockopt`
* Multiplexing: `poll` **or equivalent** (`select`, `epoll`, `kqueue`)
* Filesystem: `open`, `read`, `write`, `stat`, `access`, `opendir`, `readdir`, `closedir`
* Signals / Errors: `signal`, `kill`, `waitpid`, `errno`, `strerror`, `gai_strerror`

---

## Core Requirements

### General

* Must use **one single poll (or equivalent)** for **all I/O** (clients + listening sockets).
* Server must be **non-blocking at all times**.
* **No read/write** on sockets or pipes without prior readiness from poll (grade = 0).
* Checking `errno` after `read/write` to adjust behavior is **forbidden**.
* Disk files do **not** require polling.
* Requests must **never hang indefinitely**.
* Must support **multiple listening ports**.
* Must serve a **fully static website**.

### HTTP

* Minimum supported methods:

  * `GET`
  * `POST`
  * `DELETE`
* Correct **HTTP status codes** required.
* Default **error pages** must exist if not provided.
* Compatible with standard browsers.
* File uploads must be supported.

### CGI

* Fork allowed **only for CGI**.
* At least **one CGI** must be supported (e.g. PHP, Python).
* CGI execution based on file extension.
* Full request data must be passed via environment variables.
* Chunked requests must be **unchunked** before CGI execution.
* CGI output:

  * If `Content-Length` missing → EOF marks end.
* CGI must run in the correct working directory.

---

## Configuration File

Inspired by **NGINX** `server` blocks.

Must support:

* Multiple `host:port` listeners.
* Default error pages.
* Max client body size.
* Per-route configuration:

  * Allowed HTTP methods
  * HTTP redirections
  * Root directory mapping
  * Directory listing on/off
  * Default index file
  * Upload enable + storage path
  * CGI execution rules

Virtual hosts are **optional**.

---

## macOS Specific

* `fcntl()` allowed **only** with:

  * `F_SETFL`
  * `O_NONBLOCK`
  * `FD_CLOEXEC`
* Required due to different `write()` behavior.

---

## Testing

* Read HTTP RFCs before implementation.
* HTTP/1.0 suggested as reference.
* Test with:

  * Web browsers
  * `telnet`
  * `curl`
  * `nginx` (for behavior comparison)
* Stress testing is mandatory.
* Resilience is critical: server must remain operational.

---

## Bonus (Optional)

* Cookies support
* Session management
* Multiple CGI types

Bonus is evaluated **only if mandatory part is perfect**.

---

## Submission

* Submit via Git repository.
* Only repository content is evaluated.
* Be ready to apply **small live modifications** during evaluation to prove understanding.

