#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <vector>

void init_conf_file(int ac, char **av, std::string &filename) {
  if (1 == ac) {
    filename = "nginx.conf";
  } else if (2 == ac) {
    filename = av[1];
  } else {
    std::cerr << "Usage: " << av[0] << " [config file path]" << std::endl;
    exit(EXIT_FAILURE);
  }
}

std::string read_config_file(const std::string &file_path) {
  struct stat st;
  if (stat(file_path.c_str(), &st) != 0) {
    std::string err_msg = "Error: Unable to find the configuration file ('" +
                          file_path + "'). " + std::string(strerror(errno));
    throw std::runtime_error(err_msg);
  }
  if (!S_ISREG(st.st_mode)) {
    std::string err_msg =
        "Error: The path '" + file_path + "' is not a regular file.";
    throw std::runtime_error(err_msg);
  }
  std::ifstream file_stream(file_path.c_str());
  if (!file_stream.is_open()) {
    std::string err_msg = "Error: Unable to open the configuration file ('" +
                          file_path + "'). " + std::string(strerror(errno));
    throw std::runtime_error(err_msg);
  }
  std::string file_content;
  std::string line;
  while (std::getline(file_stream, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1);
    }
    file_content += line;
    file_content.push_back('\n');
  }
  if (file_stream.bad()) {
    std::string err_msg = "Error: An error occurred while reading the file ('" +
                          file_path + "'). " + std::string(strerror(errno));
    throw std::runtime_error(err_msg);
  }
  file_stream.close();
  return file_content;
}

std::vector<std::string> tokenize_config_file(std::string const &content) {
  std::vector<std::string> tokens;
  std::string currentToken;

  bool is_delim[256];
  std::memset(is_delim, 0, sizeof(is_delim));
  is_delim[static_cast<unsigned char>('{')] = true;
  is_delim[static_cast<unsigned char>('}')] = true;
  is_delim[static_cast<unsigned char>(';')] = true;

  for (std::string::const_iterator it = content.begin(); it != content.end();
       ++it) {
    char ch = *it;

    if (is_delim[static_cast<unsigned char>(ch)]) {
      if (!currentToken.empty()) {
        tokens.push_back(currentToken);
        currentToken.clear();
      }
      tokens.push_back(std::string(1, ch));
    } else if (std::isspace(static_cast<unsigned char>(ch))) {
      if (!currentToken.empty()) {
        tokens.push_back(currentToken);
        currentToken.clear();
      }
    } else if (ch == '"' && currentToken.empty()) {

      ++it;
      while (it != content.end() && *it != '"') {
        currentToken += *it;
        ++it;
      }
      if (it == content.end()) {
        throw std::runtime_error(
            "unexpected end of file, expecting \";\" or \"}\"");
      }
      tokens.push_back(currentToken);
      currentToken.clear();
    } else if (ch == '#') {
      if (!currentToken.empty()) {
        tokens.push_back(currentToken);
        currentToken.clear();
      }
      while (it != content.end() && *it != '\n') {
        ++it;
      }
    } else {
      currentToken += ch;
    }
  }

  if (!currentToken.empty()) {
    tokens.push_back(currentToken);
  }
  return tokens;
}

// Time to validate the syntax of the config file
// The steps will be as follows:
// 1. Check for matching braces
// 2. Ensure directives end with a semicolon
// 3. Validate known directives and their context

static void validateBraces(const std::vector<std::string> &tokens) {
  // Code goes here
  if (tokens.empty()) {
    throw std::runtime_error("Error: Empty configuration file.");
  }

  std::size_t depth = 0;

  for (std::size_t i = 0; i < tokens.size(); ++i) {
    const std::string &token = tokens[i];

    if (token == "{") {
      // Reject leading '{' with no context.
      if (i == 0) {
        throw std::runtime_error("Error: Unexpected opening brace.");
      }

      const std::string &prev = tokens[i - 1];

      // Reject consecutive '{' tokens.
      if (prev == "{") {
        throw std::runtime_error("Error: Unexpected opening brace.");
      }

      // Reject patterns like '} ... {' according to legacy rules.
      if (prev == "}") {
        if (i < 2) {
          throw std::runtime_error("Error: Unexpected opening brace.");
        }
        const std::string &beforePrev = tokens[i - 2];
        if (beforePrev.find_first_not_of("server") == std::string::npos) {
          throw std::runtime_error("Error: Unexpected closing brace.");
        }
        throw std::runtime_error("Error: Unexpected opening brace.");
      }

      // Track nesting depth for later validation.
      ++depth;

      // Configuration cannot terminate immediately after an opening brace.
      if (i + 1 == tokens.size()) {
        throw std::runtime_error("Error: Unexpected closing brace.");
      }
    } else if (token == "}") {
      // Detect a stray '}' without a matching '{'.
      if (depth == 0) {
        throw std::runtime_error("Error: Unexpected closing brace.");
      }
      --depth;
    }
  }

  // Any leftover depth indicates at least one unmatched '{'.
  if (depth != 0) {
    throw std::runtime_error("Error: Unmatched opening brace.");
  }
}

static void validateRequiredContexts(const std::vector<std::string> &tokens) {
  // Code goes here
}

static void validateContexts(const std::vector<std::string> &tokens) {
  // Code goes here
}

static void validateDirectives(const std::vector<std::string> &tokens) {
  // Code goes here
}

void validate_syntax(const std::vector<std::string> &tokens) {
  // validateBraces(tokens);
  // validateRequiredContexts(tokens);
  // validateContexts(tokens);
  // validateDirectives(tokens);
}

int main(int argc, char *argv[]) {
  std::string config_file;
  init_conf_file(argc, argv, config_file);
  try {
    std::string config_content = read_config_file(config_file);

    std::vector<std::string> tokens = tokenize_config_file(config_content);
    // For demonstration, print the tokens
    for (size_t i = 0; i < tokens.size(); ++i) {
      std::cout << "Token " << i << ": " << tokens[i] << std::endl;
    }

  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
