#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// Reuse the production parsing logic while bypassing the CLI entry point.
#define main webserv_program_main
#include "../main.cpp"
#undef main

enum ValidatorTarget { BRACE_VALIDATOR, REQUIRED_CONTEXT_VALIDATOR };

struct ValidationCase {
  const char *name;
  const char *config;
  const char *expected_error; // NULL means success is expected
};

static bool run_case(const ValidationCase &tc, ValidatorTarget target) {
  bool passed = true;
  std::string failure_message;
  std::vector<std::string> tokens;

  try {
    std::string content(tc.config);
    tokens = tokenize_config_file(content);
  } catch (const std::exception &e) {
    passed = false;
    failure_message = std::string("tokenization failed: ") + e.what();
  }

  if (passed) {
    try {
      if (target == BRACE_VALIDATOR) {
        validateBraces(tokens);
      } else {
        validateRequiredContexts(tokens);
      }
      if (tc.expected_error != 0) {
        passed = false;
        failure_message = std::string("expected exception '") +
                          tc.expected_error + "' but validation succeeded";
      }
    } catch (const std::exception &e) {
      if (tc.expected_error == 0) {
        passed = false;
        failure_message = std::string("unexpected exception: ") + e.what();
      } else if (std::string(tc.expected_error) != e.what()) {
        passed = false;
        failure_message = std::string("expected exception '") +
                          tc.expected_error + "', got '" + e.what() + "'";
      }
    }
  }

  if (passed) {
    std::cout << "[PASS] " << tc.name << std::endl;
  } else {
    std::cout << "[FAIL] " << tc.name << " -> " << failure_message
              << std::endl;
  }
  return passed;
}

int main() {
  const ValidationCase brace_cases[] = {
      {"empty_config", "", "Error: Empty configuration file."},
      {"leading_opening_brace", "{\nhttp { server { } }",
       "Error: Unexpected opening brace."},
      {"consecutive_opening_brace", "http {{ server { } } }",
       "Error: Unexpected opening brace."},
      {"stray_closing_brace", "}\nhttp { server { } }",
       "Error: Unexpected closing brace."},
      {"unmatched_opening_brace", "http { server { }",
       "Error: Unmatched opening brace."},
      {"well_nested_structure", "http { server { location /img { } } }", 0}};

  const ValidationCase required_context_cases[] = {
      {"missing_http_context", "server { listen 8080; }",
       "Error: Missing required 'http' context."},
      {"missing_server_context", "http { }",
       "Error: Missing required 'server' context."},
      {"http_without_brace", "http server { }",
       "Error: 'http' context must be followed by '{'."},
      {"server_without_brace", "http { server listen 80; }",
       "Error: 'server' context must be followed by '{'."},
      {"http_unexpected_closing_brace", "http } server { }",
       "Error: Unexpected closing brace after 'http' context."},
      {"http_unexpected_eof", "server { } http",
       "Error: Unexpected end of file after 'http' context."},
      {"location_without_path", "http { server { location { } } }",
       "Error: 'location' directive missing path before '{'."},
      {"location_without_block", "http { server { location /images; } }",
       "Error: 'location' directive must be followed by a path and then '{'."},
      {"valid_required_contexts", "http { server { location / { } } }", 0}};

  size_t total = 0;
  size_t failures = 0;

  for (size_t i = 0; i < sizeof(brace_cases) / sizeof(brace_cases[0]); ++i) {
    ++total;
    if (!run_case(brace_cases[i], BRACE_VALIDATOR)) {
      ++failures;
    }
  }

  for (size_t i = 0;
       i < sizeof(required_context_cases) / sizeof(required_context_cases[0]);
       ++i) {
    ++total;
    if (!run_case(required_context_cases[i], REQUIRED_CONTEXT_VALIDATOR)) {
      ++failures;
    }
  }

  std::cout << "\nTest summary: " << (total - failures) << "/" << total
            << " passed." << std::endl;

  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
