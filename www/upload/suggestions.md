# Suggestions and Improvement Plan

This document turns the findings in `report.md` and `codex_report.md` into a concrete, verbose, step-by-step plan for improving a webserver codebase. It is written to be copied into LLM prompts and used as a checklist for incremental refactors. The plan assumes there is a "slow" baseline and a "fast" reference implementation. If you only have one codebase, treat "fast" items as target changes to implement.

## 1. Goals and scope

### Primary goals
- Improve throughput and reduce latency on the hot paths (accept, read, parse, respond, write).
- Reduce memory churn (allocations and copies) in request/response handling.
- Maintain or improve correctness, especially around partial writes and binary payloads.

### Secondary goals
- Keep code quality reasonable (remove dead code, centralize helpers).
- Preserve or improve security checks (path traversal, upload sanitization).
- Preserve or restore graceful shutdown behavior if needed.

### Non-goals (unless explicitly requested)
- Major redesign of the HTTP API or configuration system.
- Adding TLS or HTTP/2.
- Rewriting the server to a different I/O model (io_uring, threads, etc.).

## 2. Baseline and measurement plan

Before changing anything, establish a reproducible baseline. Many of the fast/slow differences in the reports are large enough that measurement should be consistent even on a laptop, but make the measurement official so improvements can be verified.

### 2.1 Build profiles
- Define two build profiles:
  - Debug: `-g -O0` with ASan or UBsan if available.
  - Release: `-O2` (or `-O3` only if validated) and no sanitizers.
- Confirm that the slow baseline uses debug settings and that the fast reference uses release settings. The reports note this as a major source of speed difference.
- Document in the Makefile or build script exactly which flags are used and how to switch.

### 2.2 Benchmark harness
- Use a consistent tool such as `wrk` or `ab` with pinned parameters.
- Measure at least:
  - Requests/sec (overall throughput).
  - Average latency and at least p95 or p99 latency.
  - CPU utilization and memory footprint.
- Repeat each benchmark 3-5 times and take a median.
- Capture baseline results in a `BENCHMARKS.md` or similar log for comparison.

### 2.3 Baseline test suite
- Ensure existing unit tests and integration tests pass before changes.
- If no tests exist, create a minimal smoke test covering:
  - Basic GET/HEAD/POST.
  - Large file download.
  - Upload or multipart POST.
  - Connection close behavior.
  - Non-ASCII binary content delivery.

## 3. High-level roadmap

The improvements are organized into phases. Each phase can be done independently and validated with benchmarks.

### Phase 0: Build and profiling alignment (must do first)
**Goal:** Ensure performance measurements are meaningful by using optimized builds for benchmarks.

Steps:
- Update the build system so that release builds use `-O2` (or `-O3` if verified) without sanitizers.
- Ensure debug builds keep sanitizers for safety in development.
- Add a `DEBUG=1` or `RELEASE=1` toggle to Makefile or scripts.

Acceptance criteria:
- Release build is created with optimization flags enabled.
- Debug build retains sanitizers.
- Running benchmarks on release build shows meaningful throughput improvement versus debug.

Risks:
- Minimal; this is configuration only. Ensure CI or developer workflow still uses debug builds.

### Phase 1: Socket setup and epoll behavior (high impact)
**Goal:** Reduce wakeups and improve handling of connection bursts.

Changes:
- Enable `TCP_NODELAY` on server sockets and client sockets to disable Nagle's algorithm.
- Enable `SO_REUSEPORT` on server sockets if supported, to allow kernel-level load balancing.
- Use `EPOLLET` (edge-triggered) for client sockets.
- Add `EPOLLRDHUP` to detect peer shutdown without extra recv calls.
- Implement multi-accept loop that drains all pending connections on each accept event.
- Ensure read loops and write loops drain until `EAGAIN`/`EWOULDBLOCK` when using EPOLLET.

Code areas (typical):
- `src/server/server.cpp`: socket setup, accept loop, epoll registration.
- `src/main.cpp`: event loop handling for EPOLLHUP/ERR and EPOLLRDHUP.

Acceptance criteria:
- Accept loop handles multiple pending connections in a single epoll wakeup.
- Read loop drains until `EAGAIN` without blocking.
- Write loop drains until `EAGAIN` without blocking.
- Requests/sec and latency improve under concurrent load.

Risks and mitigations:
- EPOLLET requires careful draining to avoid missing events. Add tests for burst connections and pipelined requests.
- Ensure errors are handled correctly and sockets are closed on fatal errors.

### Phase 2: Write path optimization (very high impact)
**Goal:** Avoid O(n) buffer shifts and improve partial-write handling.

Changes:
- Replace buffer front erasure with a `writeOffset` index.
- Track `writeOffset` and send in a loop until `EAGAIN`.
- Reset the write buffer only when fully flushed.
- Add a `closeAfterWrite` flag if needed to support `Connection: close` semantics.

Code areas:
- `src/server/client.hpp`: add `writeOffset`, `closeAfterWrite`.
- `src/server/server.cpp`: update write loop.

Acceptance criteria:
- No `erase()` on the write buffer in the hot path.
- Partial writes are handled correctly (send returns < requested bytes).
- Connection closes correctly after pending data is flushed when requested by HTTP headers.

Risks:
- Bugs in offset tracking can lead to duplicated or missing bytes. Add tests for partial writes and large responses.

### Phase 3: Read path and HTTP pipelining (high impact)
**Goal:** Process all available data per epoll wakeup and support multiple requests in the buffer.

Changes:
- Use a larger stack buffer for `recv` (e.g., 16KB) to reduce syscall count.
- Append received bytes to the request buffer without extra temporary vectors.
- Parse and handle multiple complete requests in the buffer (pipelining).
- Immediately enqueue responses into the write buffer.

Code areas:
- `src/server/server.cpp`: `readClient` implementation.
- `src/http/HTTPRequest.cpp`: add `addData(const char*, size_t)` overload.

Acceptance criteria:
- A single read call can yield multiple processed requests.
- Response data is appended to the write buffer without overwriting previous data.
- Benchmarks show reduced latency under pipelined requests.

Risks:
- Parser must correctly manage buffer offsets and not re-parse old data.
- Ensure request reset is done only after a full request is consumed.

### Phase 4: Response generation and binary safety (high impact)
**Goal:** Reduce allocations and support binary response bodies.

Changes:
- Store response body as `std::vector<char>` to handle binary data safely.
- Replace `std::ostringstream` response construction with a pre-sized buffer build using `memcpy`.
- Append response headers and body directly into the client write buffer (single resize).

Code areas:
- `src/http/HTTPResponse.cpp`: implement `toBuffer(std::vector<char>&)`.
- `src/http/HTTPResponse.hpp`: update body type and interface.

Acceptance criteria:
- Response generation uses a single buffer resize for header + body.
- Binary files (images, video) are delivered correctly without truncation.
- Profiling shows fewer allocations on the response path.

Risks:
- Incorrect size calculation can corrupt responses. Add tests for headers and body length correctness.

### Phase 5: Request parsing micro-optimizations (medium impact)
**Goal:** Reduce parsing overhead without sacrificing correctness.

Changes:
- Use `std::search` for CRLF detection rather than manual byte-by-byte loops.
- Use `memcpy` for body data copying into the request structure.

Code areas:
- `src/http/HTTPRequest.cpp`: parsing loops and buffer copying.

Acceptance criteria:
- Parser still correctly handles header boundaries and body sizes.
- No regression in parsing correctness with edge cases (empty lines, multiple headers).

Risks:
- Subtle bugs with iterator ranges; add tests for header parsing and body boundaries.

### Phase 6: Static file serving and range behavior (medium impact)
**Goal:** Reduce time-to-first-byte for large files and avoid unnecessary memory usage.

Changes:
- Add Range request support with proper `206 Partial Content` responses.
- For large files, consider sending an initial chunk instead of loading the full file into memory.
- Ensure alias resolution and path normalization is correct and secure.

Code areas:
- `src/http/Methods/Get.cpp`: range handling, file reads.
- `src/config/WebserverConfig.cpp`: alias validation behavior.

Acceptance criteria:
- Valid Range requests return correct `206` responses and Content-Range headers.
- Large file responses do not allocate the entire file in memory.
- Alias paths are validated against directory traversal.

Risks:
- The fast reference returns `206` even without Range for large files; this is non-standard. Decide whether to keep standard `200` behavior or accept the optimization. If you keep `206`, document the behavior and ensure clients accept it.

### Phase 7: Upload handling and multipart parsing (medium impact)
**Goal:** Improve upload performance without sacrificing security.

Changes:
- Simplify multipart parsing if performance is critical, but keep filename sanitization and safety checks.
- Use `std::search` or similar for boundary detection for efficiency.
- Write to disk incrementally instead of buffering entire uploads in memory.

Code areas:
- `src/http/Methods/Post.cpp`: multipart parser, file write flow.

Acceptance criteria:
- Multipart uploads succeed for valid requests.
- Filenames are sanitized to avoid directory traversal.
- Large uploads do not require full in-memory buffers.

Risks:
- Fast reference simplified parsing and may be less robust. Ensure you do not regress security (filename checks, boundary validation).

### Phase 8: Logging hot-path guard (low-medium impact)
**Goal:** Avoid expensive log message construction when logging is disabled.

Changes:
- Implement `Logger::isDebugEnabled()` and use it at call sites to guard expensive logs.
- Only construct strings when debug logging is active.

Code areas:
- `src/utils/Logger.hpp` and `src/utils/Logger.cpp`.
- Call sites that build complex strings or convert values.

Acceptance criteria:
- Debug logging disabled path does not allocate or format strings.
- Behavior unchanged when debug logging is enabled.

Risks:
- Low; but avoid scattering many guards if logging is infrequent in hot paths.

### Phase 9: Event loop structure and graceful shutdown (optional)
**Goal:** Simplify event loop without losing needed signal handling.

Changes:
- If you remove signal handling (like signalfd), add a clear alternative or document the lack of graceful shutdown.
- Ensure EPOLLHUP/EPOLLERR are handled early in the event loop.

Code areas:
- `src/main.cpp`: event loop and signal handling.

Acceptance criteria:
- Event loop handles HUP/ERR first and closes connections correctly.
- If graceful shutdown is required, reintroduce a minimal approach.

Risks:
- Removing signal handling may be acceptable in some deployments but not in others. Decide based on requirements.

## 4. Detailed task list (copy/paste checklist)

Use this as an LLM-friendly checklist. Each item can be a standalone task.

1. Build config
- Ensure release builds use `-O2` and disable ASan.
- Ensure debug builds keep `-g` and sanitizers.
- Add build toggles or targets.

2. Socket and epoll
- Add `TCP_NODELAY` and `SO_REUSEPORT` to server socket setup.
- Add `TCP_NODELAY` on each accepted client socket.
- Register client fds with `EPOLLIN | EPOLLRDHUP | EPOLLET`.
- Implement accept loop that drains until `EAGAIN`.
- Ensure read loop drains until `EAGAIN`.
- Ensure write loop drains until `EAGAIN`.

3. Write path
- Add `writeOffset` to client state.
- Replace write buffer erases with offset updates.
- Send in a loop until `EAGAIN`.
- Reset buffer when fully sent.
- Add `closeAfterWrite` for `Connection: close`.

4. Read path and pipelining
- Use a stack buffer of ~16KB in `readClient`.
- Implement `HTTPRequest::addData(const char*, size_t)` to append directly.
- Parse and process all complete requests in the buffer in a loop.
- Append each response to the write buffer; do not overwrite.

5. Response generation
- Store response body as `std::vector<char>`.
- Add `HttpResponse::toBuffer(std::vector<char>&)` with pre-sized allocation.
- Replace `ostringstream` with `memcpy`-style buffer assembly.

6. Parser micro-opts
- Use `std::search` for CRLF detection.
- Replace `insert` with `resize` + `memcpy` for body copies.

7. Static files
- Add Range support and correct headers.
- Avoid full-file reads for large files; send first chunk if needed.
- Ensure alias resolution and path checks are secure.

8. Uploads
- Optimize multipart parsing with `std::search` or boundary scanning.
- Preserve filename sanitization and boundary checks.
- Stream upload to disk to avoid large buffers.

9. Logging
- Add `Logger::isDebugEnabled()` and guard expensive logs.

10. Event loop
- Handle EPOLLHUP/EPOLLERR first.
- Decide whether to keep or remove signalfd; document behavior.

## 5. Acceptance criteria and validation matrix

Use this matrix to validate changes after each phase.

Functional tests:
- GET small file returns 200 and correct body.
- GET large file returns full content; if Range is used, returns correct 206 and Content-Range.
- HEAD returns headers only.
- POST upload accepts multipart and stores file correctly.
- Connection closes correctly when client sends `Connection: close`.
- Binary files are served correctly without truncation.

Performance tests:
- Throughput target: at least 2-4x improvement vs baseline.
- Average latency reduced by at least 2x.
- p95 latency improves under concurrent load.

Correctness and robustness tests:
- Pipelined requests: send multiple requests on one connection and confirm all responses in order.
- Partial write simulation: throttle client to force partial sends.
- Burst connections: open many connections at once to test accept loop.

Security tests:
- Path traversal in GET/DELETE blocked.
- Upload filename sanitization prevents traversal.
- Alias paths are validated as directories.

## 6. Known trade-offs and decisions

These are the known behavior changes and choices you must make consciously:

- EPOLLET is faster but more fragile; ensure loops drain to EAGAIN.
- Removing signalfd simplifies code but removes graceful shutdown.
- Simplified multipart parsing can accept unsafe filenames if not sanitized.
- Returning 206 without Range is non-standard; decide if you accept the optimization.
- Removing path validation improves speed but may weaken security; avoid this unless explicitly desired.

## 7. Suggested sequence for implementation

This is a clean ordering that usually minimizes regressions:

1. Build flags (Phase 0) and baseline benchmarks.
2. Socket + epoll updates (Phase 1).
3. Write path offsets and looped send (Phase 2).
4. Read loop, pipelining, `addData` overload (Phase 3).
5. Response generation with `vector<char>` and `toBuffer` (Phase 4).
6. Parsing micro-optimizations (Phase 5).
7. Static file Range and chunk optimization (Phase 6).
8. Upload parser optimization with security checks intact (Phase 7).
9. Logging guard (Phase 8).
10. Event loop cleanup and signal handling decision (Phase 9).

Each step should be benchmarked and tested before moving on.

## 8. Optional extras (if you want to go further)

These are outside the core recommendations but can help:
- Add connection metrics and per-FD state logging (guarded by debug flags).
- Add backpressure handling when write buffers grow beyond a threshold.
- Implement response caching for static assets.
- Consider sendfile() for large static files if available.
- Add per-request timing instrumentation for profiling (debug builds only).

## 9. Summary

The largest wins come from:
- Using release build optimizations (Phase 0).
- Switching to edge-triggered epoll with proper drain loops (Phase 1).
- Eliminating O(n) buffer erases in the write path (Phase 2).
- Building responses with pre-sized buffers and binary-safe bodies (Phase 4).

Following the phased plan above should bring the slow codebase close to the fast codebase while keeping correctness and security under control.
