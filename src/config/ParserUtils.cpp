#include "ParserUtils.hpp"

bool isAllDigits(const std::string &value) {
  for (size_t i = 0; i < value.length(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(value[i]))) {
      return false;
    }
  }
  return true;
}

int stoiStrict(const std::string &str) {
  if (!isAllDigits(str)) {
    throw std::invalid_argument("Invalid integer string: " + str);
  }
  std::stringstream ss(str);
  long value = 0;
  ss >> value;

  if (ss.fail() || value > std::numeric_limits<int>::max()) {
    throw std::invalid_argument("Value is out of bounds: " + str);
  }
  return (static_cast<int>(value));
}

unsigned int hexToUint(const std::string &hex) {
  unsigned int res = 0;
  std::stringstream ss;
  ss << std::hex << hex;
  ss >> res;
  if (ss.fail()) {
    throw std::invalid_argument("Invalid hexadecimal string: " + hex);
  }
  return res;
}

std::string trimWhitespace(const std::string &value) {
  const std::string whitespace = " \t\n\r\f\v";
  if (value.empty()) {
    return value;
  }
  size_t start = value.find_first_not_of(whitespace);
  if (start == std::string::npos) {
    return "";
  }
  size_t end = value.find_last_not_of(whitespace);
  return value.substr(start, end - start + 1);
}

void enforceTrailingSemicolon(std::string &token, const std::string &context) {
  if (token.empty() || token[token.size() - 1] != ';')
    throw std::invalid_argument("Token is invalid in " + context +
                                " (missing semicolon)");
  token.erase(token.size() - 1); // Remove the semicolon
}

std::string statusCodeToString(short statusCode) {
  switch (statusCode) {
  case 100:
    return ("Continue");
  case 101:
    return ("Switching Protocol");
  case 200:
    return ("OK");
  case 201:
    return ("Created");
  case 202:
    return ("Accepted");
  case 203:
    return ("Non-Authoritative Information");
  case 204:
    return ("No Content");
  case 205:
    return ("Reset Content");
  case 206:
    return ("Partial Content");
  case 300:
    return ("Multiple Choice");
  case 301:
    return ("Moved Permanently");
  case 302:
    return ("Moved Temporarily");
  case 303:
    return ("See Other");
  case 304:
    return ("Not Modified");
  case 307:
    return ("Temporary Redirect");
  case 308:
    return ("Permanent Redirect");
  case 400:
    return ("Bad Request");
  case 401:
    return ("Unauthorized");
  case 403:
    return ("Forbidden");
  case 404:
    return ("Not Found");
  case 405:
    return ("Method Not Allowed");
  case 406:
    return ("Not Acceptable");
  case 407:
    return ("Proxy Authentication Required");
  case 408:
    return ("Request Timeout");
  case 409:
    return ("Conflict");
  case 410:
    return ("Gone");
  case 411:
    return ("Length Required");
  case 412:
    return ("Precondition Failed");
  case 413:
    return ("Payload Too Large");
  case 414:
    return ("URI Too Long");
  case 415:
    return ("Unsupported Media Type");
  case 416:
    return ("Requested Range Not Satisfiable");
  case 417:
    return ("Expectation Failed");
  case 418:
    return ("I'm a teapot");
  case 421:
    return ("Misdirected Request");
  case 425:
    return ("Too Early");
  case 426:
    return ("Upgrade Required");
  case 428:
    return ("Precondition Required");
  case 429:
    return ("Too Many Requests");
  case 431:
    return ("Request Header Fields Too Large");
  case 451:
    return ("Unavailable for Legal Reasons");
  case 500:
    return ("Internal Server Error");
  case 501:
    return ("Not Implemented");
  case 502:
    return ("Bad Gateway");
  case 503:
    return ("Service Unavailable");
  case 504:
    return ("Gateway Timeout");
  case 505:
    return ("HTTP Version Not Supported");
  case 506:
    return ("Variant Also Negotiates");
  case 507:
    return ("Insufficient Storage");
  case 510:
    return ("Not Extended");
  case 511:
    return ("Network Authentication Required");
  default:
    return ("Undefined");
  }
}
