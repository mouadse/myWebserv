# Config Parser Tests

This suite stress-tests the parser with intentionally tricky configuration blocks plus a couple of healthy ones. The assets live under `tests/configs/` and the `parser_tests` binary drives the scenarios.

## Running the tests

```bash
# build & execute the full suite
make test

# run only tests whose name contains "cgi"
make test TEST_FILTER=cgi
```

## Config edge cases

### Valid fixtures

| File | Intent |
|------|--------|
| `valid_basic.conf` | Minimal healthy server that mixes inline comments, whitespace noise, and per-location overrides. |
| `valid_multiserver.conf` | Two valid servers with CGI, error page overrides, and differing limits to ensure cluster validation plus inheritance path. |
| `valid_defaults.conf` | Exercises default host/index/body-size inheritance plus per-location overrides. |
| `valid_cgi_extended.conf` | CGI-heavy server that verifies path/extension pairing, redirects, and small-body limits. |
| `valid_alias_and_return.conf` | Alias/return pairing, wildcard CGI mapping, and alternate `methods` directive usage. |

### Invalid fixtures

| File | Intent |
|------|--------|
| `invalid_missing_semicolon.conf` | Drops the `listen` semicolon to hit the tokenizer/validation path. |
| `invalid_duplicate_directives.conf` | Repeats `client_max_body_size` to ensure duplicate detection. |
| `invalid_duplicate_server_blocks.conf` | Two servers share the same host/port/name triplet to trigger cluster-level collision checks. |
| `invalid_cgi_block.conf` | CGI location lacks executables, so the CGI validator must fail. |
| `invalid_location_root.conf` | Location points to a directory that does not exist. |
| `invalid_error_page_code.conf` | Uses an out-of-range status code. |
| `invalid_location_path.conf` | Location path misses the leading slash. |
| `invalid_duplicate_locations.conf` | Declares two locations with the same path inside a single server. |
| `invalid_port_syntax.conf` | Non-numeric listen value to assert strict port parsing. |
| `invalid_host_syntax.conf` | Host string that cannot be parsed by `inet_pton`. |
| `invalid_missing_port.conf` | Omits the `listen` directive entirely. |
| `invalid_unknown_directive.conf` | Injects an unsupported directive to ensure it is rejected. |
| `invalid_duplicate_methods.conf` | Repeats `allow_methods` inside a single location. |
| `invalid_unsupported_method.conf` | Attempts to allow an unsupported HTTP verb. |
| `invalid_cgi_autoindex.conf` | Uses a forbidden directive (`autoindex`) inside a CGI block. |
| `invalid_cgi_mismatch.conf` | Mismatched `cgi_ext`/`cgi_path` lists trigger CGI validation failure. |
| `invalid_scope_trailing_text.conf` | Trailing text after a server block should break scope detection. |
| `invalid_error_page_missing_file.conf` | Points an `error_page` to a file that does not exist. |
| `invalid_return_missing_file.conf` | `return` target points to a missing file, exercising redirect file validation. |
| `invalid_alias_missing_file.conf` | `alias` targets a missing file to confirm alias existence checks. |
| `invalid_error_page_odd_count.conf` | `error_page` declared with an odd number of tokens should be rejected. |
| `invalid_cgi_bad_path.conf` | `cgi_path` uses a non-python/bash interpreter and must be refused. |
| `invalid_cgi_bad_extension.conf` | Unsupported CGI extension (`.php`) should fail validation. |
| `invalid_location_missing_index.conf` | Location inherits a missing index file from a real directory, tripping index validation. |
| `invalid_duplicate_server_defaults.conf` | Two servers collide on defaults (host/server_name) without explicit duplication. |
