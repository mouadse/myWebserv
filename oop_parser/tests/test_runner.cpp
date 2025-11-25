#include "../include/ConfigParser.hpp"

#include <arpa/inet.h>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct TestCase {
  std::string name;
  std::string config_path;
  bool expect_success;
  std::string expected_error_hint;
  bool (*verifier)(const ConfigParser &, std::string &);
};

struct TestOutcome {
  bool passed;
  std::string details;
  double elapsed_ms;
};

static const Location *findLocation(const ServerConfig &server,
                                    const std::string &path) {
  const std::vector<Location> &locations = server.getLocations();
  for (size_t i = 0; i < locations.size(); ++i) {
    if (locations[i].getPath() == path)
      return (&locations[i]);
  }
  return (NULL);
}

static bool checkAllowedMethods(const Location &location, bool get, bool post,
                                bool del, bool put, bool head,
                                std::string &message) {
  const std::vector<short> &methods = location.getMethods();
  if (methods.size() < 5) {
    message = "Location methods vector shorter than expected";
    return (false);
  }
  const bool expected[] = {get, post, del, put, head};
  for (size_t i = 0; i < 5; ++i) {
    if (!!methods[i] != expected[i]) {
      std::stringstream ss;
      ss << "Allowed methods mismatch for " << location.getPath() << " (idx "
         << i << ")";
      message = ss.str();
      return (false);
    }
  }
  return (true);
}

static bool verifyValidBasic(const ConfigParser &parser, std::string &message) {
  std::vector<ServerConfig> servers = parser.getServers();
  if (servers.size() != 1) {
    message = "Expected exactly one server";
    return (false);
  }
  const ServerConfig &server = servers[0];
  if (server.getServerName() != "basic_instance") {
    message = "server_name was not preserved";
    return (false);
  }
  if (server.getPort() != 8081) {
    message = "listen directive was not parsed correctly";
    return (false);
  }
  if (server.getHost() != inet_addr("127.0.0.1")) {
    message = "host directive did not normalize localhost";
    return (false);
  }
  if (server.getRoot() != "./www") {
    message = "root was not normalized as expected";
    return (false);
  }
  if (server.getIndex() != "index.html") {
    message = "server index fallback missing";
    return (false);
  }
  if (server.getClientMaxBodySize() != 4096) {
    message = "client_max_body_size mismatch";
    return (false);
  }
  if (server.getAutoindex()) {
    message = "autoindex should be OFF";
    return (false);
  }
  std::map<short, std::string>::const_iterator err =
      server.getErrorPages().find(404);
  if (err == server.getErrorPages().end() || err->second != "/errors/404.html") {
    message = "error_page 404 not registered";
    return (false);
  }
  const Location *root = findLocation(server, "/");
  if (!root) {
    message = "Missing / location";
    return (false);
  }
  if (root->getRootLocation() != server.getRoot()) {
    message = "/ location root should inherit server root";
    return (false);
  }
  if (!checkAllowedMethods(*root, true, true, false, false, false, message))
    return (false);
  if (root->getIndexLocation() != "index.html") {
    message = "/ location did not inherit index";
    return (false);
  }
  if (root->getMaxBodySize() != server.getClientMaxBodySize()) {
    message = "/ location did not inherit max body size";
    return (false);
  }

  const Location *upload = findLocation(server, "/upload");
  if (!upload) {
    message = "Missing /upload location";
    return (false);
  }
  if (!checkAllowedMethods(*upload, false, true, false, false, false, message))
    return (false);
  if (upload->getMaxBodySize() != 1024) {
    message = "/upload location max body size mismatch";
    return (false);
  }
  if (upload->getIndexLocation() != "index.html") {
    message = "/upload did not keep its index";
    return (false);
  }
  if (upload->getAutoindex()) {
    message = "/upload should keep autoindex off by default";
    return (false);
  }
  return (true);
}

static bool verifyValidMulti(const ConfigParser &parser, std::string &message) {
  std::vector<ServerConfig> servers = parser.getServers();
  if (servers.size() != 2) {
    message = "Expected a two server cluster";
    return (false);
  }
  const ServerConfig &alpha = servers[0];
  const ServerConfig &beta = servers[1];
  if (alpha.getPort() != 8082 || alpha.getHost() != inet_addr("127.0.0.1")) {
    message = "alpha listen/host mismatch";
    return (false);
  }
  if (alpha.getServerName() != "alpha") {
    message = "alpha server_name mismatch";
    return (false);
  }
  if (alpha.getClientMaxBodySize() != 2048) {
    message = "alpha client_max_body_size mismatch";
    return (false);
  }
  if (alpha.getIndex() != "index.html") {
    message = "alpha index not set";
    return (false);
  }
  if (alpha.getErrorPages().find(500)->second != "/errors/500.html") {
    message = "alpha did not register error_page 500";
    return (false);
  }
  const Location *alpha_root = findLocation(alpha, "/");
  if (!alpha_root) {
    message = "alpha missing / location";
    return (false);
  }
  if (!checkAllowedMethods(*alpha_root, true, false, false, false, false,
                           message))
    return (false);

  const Location *alpha_cgi = findLocation(alpha, "/cgi-bin");
  if (!alpha_cgi) {
    message = "alpha missing /cgi-bin";
    return (false);
  }
  if (alpha_cgi->getCgiPath().size() != 2 ||
      alpha_cgi->getCgiExtension().size() != 2) {
    message = "CGI location did not capture path/ext pairings";
    return (false);
  }
  if (alpha_cgi->getExtensionPath().size() != 2 ||
      !alpha_cgi->getExtensionPath().count(".py") ||
      !alpha_cgi->getExtensionPath().count(".sh")) {
    message = "CGI extension mapping incomplete";
    return (false);
  }
  if (alpha_cgi->getIndexLocation() != "handler.py") {
    message = "CGI index not preserved";
    return (false);
  }

  if (beta.getPort() != 8083 || beta.getServerName() != "beta") {
    message = "beta listen/server_name mismatch";
    return (false);
  }
  if (!beta.getAutoindex()) {
    message = "Second server should inherit autoindex=on";
    return (false);
  }
  if (beta.getClientMaxBodySize() != kDefaultMaxBodySize) {
    message = "beta should keep default max body size";
    return (false);
  }

  const Location *download = findLocation(beta, "/download");
  if (!download) {
    message = "beta missing /download";
    return (false);
  }
  if (!checkAllowedMethods(*download, true, false, false, false, false,
                           message))
    return (false);

  const Location *deep = findLocation(beta, "/deep");
  if (!deep) {
    message = "beta missing /deep";
    return (false);
  }
  if (deep->getRootLocation() != "./www/errors") {
    message = "/deep root override failed";
    return (false);
  }
  if (deep->getIndexLocation() != "404.html") {
    message = "/deep index override failed";
    return (false);
  }

  const Location *limited = findLocation(beta, "/limited");
  if (!limited) {
    message = "Second server /limited location missing";
    return (false);
  }
  if (limited->getMaxBodySize() != 64) {
    message = "Second server /limited location missing overrides";
    return (false);
  }
  if (!checkAllowedMethods(*limited, true, false, true, false, false, message))
    return (false);
  return (true);
}

static bool verifyValidDefaults(const ConfigParser &parser,
                                std::string &message) {
  std::vector<ServerConfig> servers = parser.getServers();
  if (servers.size() != 1) {
    message = "Expected exactly one server";
    return (false);
  }
  const ServerConfig &server = servers[0];
  if (server.getPort() != 8092) {
    message = "listen directive missing in defaults config";
    return (false);
  }
  if (server.getHost() != inet_addr("127.0.0.1")) {
    message = "default host was not set to localhost";
    return (false);
  }
  if (!server.getServerName().empty()) {
    message = "server_name should default to empty";
    return (false);
  }
  if (server.getClientMaxBodySize() != kDefaultMaxBodySize) {
    message = "default client_max_body_size incorrect";
    return (false);
  }
  if (server.getIndex() != "index.html") {
    message = "default index incorrect";
    return (false);
  }
  if (server.getAutoindex()) {
    message = "server autoindex should default to off";
    return (false);
  }
  const Location *root = findLocation(server, "/");
  if (!root) {
    message = "Missing / location in defaults config";
    return (false);
  }
  if (!checkAllowedMethods(*root, true, false, true, false, false, message))
    return (false);
  if (root->getRootLocation() != server.getRoot()) {
    message = "root location did not inherit server root";
    return (false);
  }
  if (!root->getIndexLocation().empty() &&
      root->getIndexLocation() != "index.html") {
    message = "root location index was unexpectedly altered";
    return (false);
  }
  if (root->getMaxBodySize() != server.getClientMaxBodySize()) {
    message = "root location did not inherit max body size";
    return (false);
  }

  const Location *errors = findLocation(server, "/errors");
  if (!errors) {
    message = "Missing /errors location";
    return (false);
  }
  if (errors->getRootLocation() != "./www/errors") {
    message = "/errors root override failed";
    return (false);
  }
  if (!errors->getAutoindex()) {
    message = "/errors autoindex should be on";
    return (false);
  }
  if (errors->getIndexLocation() != "404.html") {
    message = "/errors index override failed";
    return (false);
  }
  if (errors->getMaxBodySize() != server.getClientMaxBodySize()) {
    message = "/errors max body size should be inherited";
    return (false);
  }
  return (true);
}

static bool verifyValidCgiExtended(const ConfigParser &parser,
                                   std::string &message) {
  std::vector<ServerConfig> servers = parser.getServers();
  if (servers.size() != 1) {
    message = "Expected one server in cgi_extended config";
    return (false);
  }
  const ServerConfig &server = servers[0];
  if (server.getPort() != 8093 || server.getHost() != inet_addr("127.0.0.1")) {
    message = "cgi_extended listen/host mismatch";
    return (false);
  }
  if (server.getServerName() != "cgi_extended") {
    message = "cgi_extended server_name mismatch";
    return (false);
  }
  if (!server.getAutoindex()) {
    message = "cgi_extended should keep server autoindex on";
    return (false);
  }
  if (server.getClientMaxBodySize() != 1024) {
    message = "cgi_extended client_max_body_size mismatch";
    return (false);
  }
  if (server.getErrorPages().find(404)->second != "/errors/404.html" ||
      server.getErrorPages().find(500)->second != "/errors/500.html") {
    message = "cgi_extended error pages not registered";
    return (false);
  }

  const Location *cgi = findLocation(server, "/cgi-bin");
  if (!cgi) {
    message = "Missing /cgi-bin location";
    return (false);
  }
  if (cgi->getCgiPath().size() != 2 || cgi->getCgiExtension().size() != 2) {
    message = "cgi location did not capture both cgi_path and cgi_ext";
    return (false);
  }
  if (cgi->getExtensionPath().size() != 2 ||
      !cgi->getExtensionPath().count(".py") ||
      !cgi->getExtensionPath().count(".sh")) {
    message = "cgi extension mapping incomplete";
    return (false);
  }
  if (cgi->getIndexLocation() != "handler.py") {
    message = "cgi index missing";
    return (false);
  }
  if (cgi->getRootLocation() != "./www") {
    message = "cgi root override not applied";
    return (false);
  }

  const Location *download = findLocation(server, "/download");
  if (!download) {
    message = "Missing /download location";
    return (false);
  }
  if (!checkAllowedMethods(*download, true, false, false, false, false,
                           message))
    return (false);
  if (download->getReturn() != "/errors/404.html") {
    message = "/download return directive missing";
    return (false);
  }

  const Location *limited = findLocation(server, "/limited");
  if (!limited) {
    message = "Missing /limited location";
    return (false);
  }
  if (!checkAllowedMethods(*limited, false, true, true, false, false, message))
    return (false);
  if (limited->getMaxBodySize() != 16) {
    message = "/limited max body size override missing";
    return (false);
  }
  if (limited->getAutoindex()) {
    message = "/limited autoindex should be off";
    return (false);
  }
  return (true);
}

static bool containsSubstring(const std::string &value,
                              const std::string &needle) {
  if (needle.empty())
    return (true);
  return (value.find(needle) != std::string::npos);
}

static TestOutcome runSingleTest(const TestCase &test) {
  TestOutcome outcome;
  outcome.passed = false;
  outcome.elapsed_ms = 0.0;
  const clock_t start = clock();
  ConfigParser parser;
  try {
    parser.createCluster(test.config_path);
    if (!test.expect_success) {
      outcome.details = "Expected failure but parsing succeeded";
    } else if (test.verifier) {
      std::string reason;
      if (test.verifier(parser, reason)) {
        outcome.passed = true;
      } else {
        outcome.details = reason;
      }
    } else {
      outcome.passed = true;
    }
  } catch (const std::exception &e) {
    if (test.expect_success) {
      outcome.details = e.what();
    } else if (!containsSubstring(e.what(), test.expected_error_hint)) {
      std::stringstream ss;
      ss << "Error message mismatch. Expected hint: '"
         << test.expected_error_hint << "' got: '" << e.what() << "'";
      outcome.details = ss.str();
    } else {
      outcome.passed = true;
    }
  }
  const clock_t end = clock();
  outcome.elapsed_ms =
      static_cast<double>(end - start) * 1000.0 / CLOCKS_PER_SEC;
  if (outcome.passed)
    outcome.details.clear();
  return (outcome);
}

static void printHeader(const std::string &filter) {
  std::cout << "==========================================\n";
  std::cout << " Config Parser Test Suite";
  if (!filter.empty())
    std::cout << "  (filter: " << filter << ")";
  std::cout << "\n==========================================\n";
}

static void printFooter(size_t passed, size_t failed, size_t skipped) {
  std::cout << "------------------------------------------\n";
  std::cout << " Summary: " << passed << " passed, " << failed
            << " failed, " << skipped << " skipped" << std::endl;
  std::cout << "------------------------------------------\n";
}

int main(int argc, char **argv) {
  std::string filter;
  if (argc > 1)
    filter = argv[1];

  const TestCase test_cases[] = {
      {"valid_basic", "tests/configs/valid_basic.conf", true, "",
       &verifyValidBasic},
      {"valid_multiserver", "tests/configs/valid_multiserver.conf", true,
       "", &verifyValidMulti},
      {"valid_defaults", "tests/configs/valid_defaults.conf", true, "",
       &verifyValidDefaults},
      {"valid_cgi_extended", "tests/configs/valid_cgi_extended.conf", true, "",
       &verifyValidCgiExtended},
      {"invalid_missing_semicolon",
       "tests/configs/invalid_missing_semicolon.conf", false,
       "missing semicolon", NULL},
      {"invalid_duplicate_directives",
       "tests/configs/invalid_duplicate_directives.conf", false,
       "Client_max_body_size is duplicated", NULL},
      {"invalid_duplicate_server_blocks",
       "tests/configs/invalid_duplicate_server_blocks.conf", false,
       "Failed server validation", NULL},
      {"invalid_cgi_block", "tests/configs/invalid_cgi_block.conf", false,
       "Failed CGI validation", NULL},
      {"invalid_location_root",
       "tests/configs/invalid_location_root.conf", false,
       "root of location is invalid", NULL},
      {"invalid_error_page_code",
       "tests/configs/invalid_error_page_code.conf", false,
       "Incorrect error code", NULL},
      {"invalid_location_path",
       "tests/configs/invalid_location_path.conf", false,
       "Failed path in location validation", NULL},
      {"invalid_duplicate_locations",
       "tests/configs/invalid_duplicate_locations.conf", false,
       "Locaition is duplicated", NULL},
      {"invalid_port_syntax", "tests/configs/invalid_port_syntax.conf", false,
       "Wrong syntax: port", NULL},
      {"invalid_host_syntax", "tests/configs/invalid_host_syntax.conf", false,
       "Wrong syntax: host", NULL},
      {"invalid_missing_port", "tests/configs/invalid_missing_port.conf", false,
       "Port not found", NULL},
      {"invalid_unknown_directive",
       "tests/configs/invalid_unknown_directive.conf", false,
       "Unsupported directive", NULL},
      {"invalid_duplicate_methods",
       "tests/configs/invalid_duplicate_methods.conf", false,
       "Allow_methods of location is duplicated", NULL},
      {"invalid_unsupported_method",
       "tests/configs/invalid_unsupported_method.conf", false,
       "Allow method not supported", NULL},
      {"invalid_cgi_autoindex",
       "tests/configs/invalid_cgi_autoindex.conf", false,
       "Parametr autoindex not allow for CGI", NULL},
      {"invalid_cgi_mismatch",
       "tests/configs/invalid_cgi_mismatch.conf", false,
       "Failed CGI validation", NULL},
      {"invalid_scope_trailing_text",
       "tests/configs/invalid_scope_trailing_text.conf", false,
       "server scope", NULL},
      {"invalid_error_page_missing_file",
       "tests/configs/invalid_error_page_missing_file.conf", false,
       "Incorrect path for error page file", NULL},
  };

  const size_t total_tests = sizeof(test_cases) / sizeof(TestCase);
  size_t executed = 0;
  size_t passed = 0;
  size_t failed = 0;
  size_t skipped = 0;

  printHeader(filter);

  for (size_t i = 0; i < total_tests; ++i) {
    const TestCase &test = test_cases[i];
    if (!filter.empty() && test.name.find(filter) == std::string::npos) {
      ++skipped;
      continue;
    }
    ++executed;
    std::cout << "[ RUN      ] " << std::setw(30) << std::left << test.name
              << std::right << std::flush;
    TestOutcome outcome = runSingleTest(test);
    if (outcome.passed) {
      ++passed;
      std::cout << "[   OK   ]  " << std::fixed << std::setprecision(2)
                << outcome.elapsed_ms << " ms" << std::endl;
    } else {
      ++failed;
      std::cout << "[ FAILED ]" << std::endl;
      if (!outcome.details.empty())
        std::cout << "             " << outcome.details << std::endl;
    }
  }

  printFooter(passed, failed, skipped);
  return (failed == 0 ? 0 : 1);
}
