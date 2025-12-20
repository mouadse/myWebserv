/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:48:24 by webserv          #+#    #+#             */
/*   Updated: 2025/12/12 13:03:48 by webserv           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <sstream>

HTTPRequest::HTTPRequest()
    : state(START_LINE), body_state(NONE), parse_offset(0), is_chunked(false),
      content_length(0), body_bytes_received(0), current_chunk_size(0),
      current_chunk_received(0) {
  buffer.clear();
  headers.clear();
  body.clear();
  query_params.clear();
  error_message = "";
}

HTTPRequest::HTTPRequest(const std::string &raw_request)
    : state(START_LINE), body_state(NONE), parse_offset(0), is_chunked(false),
      content_length(0), body_bytes_received(0), current_chunk_size(0),
      current_chunk_received(0) {
  buffer.clear();
  headers.clear();
  body.clear();
  query_params.clear();
  error_message = "";
  std::vector<char> data(raw_request.begin(), raw_request.end());
  addData(data);
}

void HTTPRequest::addData(const std::vector<char> &new_chunk) {
  if (new_chunk.empty())
    return;
  addData(&new_chunk[0], new_chunk.size());
}

void HTTPRequest::addData(const char *data, size_t len) {
  if (data == NULL || len == 0)
    return;
  buffer.insert(buffer.end(), data, data + len);
  while (state != DONE && state != ERROR) {
    bool phaseCompleted = false;
    if (state == START_LINE)
      phaseCompleted = tryParseRequestLine();
    else if (state == HEADERS)
      phaseCompleted = tryParseHeaders();
    else if (state == BODY)
      phaseCompleted = tryParseBody();
    else
      break;
    if (!phaseCompleted)
      break;
  }
  if (parse_offset > 0) {
    if (parse_offset >= buffer.size()) {
      buffer.clear();
      parse_offset = 0;
    } else if (parse_offset > 4096) {
      std::vector<char> tmp(buffer.begin() + parse_offset, buffer.end());
      buffer.swap(tmp);
      parse_offset = 0;
    }
  }
}

bool HTTPRequest::tryParseRequestLine() {
  // Use std::search for efficient CRLF detection
  static const char crlf[] = {'\r', '\n'};
  std::vector<char>::iterator it =
      std::search(buffer.begin() + parse_offset, buffer.end(), crlf, crlf + 2);
  if (it == buffer.end())
    return false;

  size_t line_end = it - buffer.begin();
  std::string line(buffer.begin() + parse_offset, buffer.begin() + line_end);
  parse_offset = line_end + 2;
  std::istringstream iss(line);
  std::string method_token, target_token, version_token;
  if (!(iss >> method_token >> target_token >> version_token)) {
    state = ERROR;
    error_message = "Invalid request line format";
    return false;
  }
  if (method_token != "GET" && method_token != "POST" &&
      method_token != "DELETE") {
    state = ERROR;
    error_message = "Method Not Allowed";
    return false;
  }
  if (version_token != "HTTP/1.1") {
    state = ERROR;
    error_message = "Unsupported HTTP version";
    return false;
  }
  if (target_token.size() > MAX_URI_SIZE) {
    state = ERROR;
    error_message = "URI Too Long";
    return false;
  }

  method = method_token;
  target = target_token;
  version = version_token;
  state = HEADERS;
  parseQueryParams();
  return true;
}

bool HTTPRequest::tryParseHeaders() {
  // Use std::search for efficient double-CRLF detection
  static const char crlfcrlf[] = {'\r', '\n', '\r', '\n'};
  std::vector<char>::iterator crlfIt = std::search(
      buffer.begin() + parse_offset, buffer.end(), crlfcrlf, crlfcrlf + 4);
  if (crlfIt == buffer.end())
    return false;

  size_t pos = crlfIt - buffer.begin();
  std::string headers_block(buffer.begin() + parse_offset,
                            buffer.begin() + pos);
  if (headers_block.size() > MAX_HEADER_SIZE) {
    state = ERROR;
    error_message = "Request Header Fields Too Large";
    return false;
  }
  parse_offset = pos + 4;
  std::istringstream stream(headers_block);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r')
      line.erase(line.size() - 1);
    if (line.empty())
      continue;
    size_t colon = line.find(':');
    if (colon == std::string::npos) {
      state = ERROR;
      error_message = "Malformed header line (missing colon)";
      return false;
    }
    std::string key = line.substr(0, colon);
    std::string value = line.substr(colon + 1);
    while (!key.empty() && (key[0] == ' ' || key[0] == '\t'))
      key.erase(0, 1);
    while (!key.empty() &&
           (key[key.size() - 1] == ' ' || key[key.size() - 1] == '\t'))
      key.erase(key.size() - 1);
    while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
      value.erase(0, 1);
    while (!value.empty() &&
           (value[value.size() - 1] == ' ' || value[value.size() - 1] == '\t'))
      value.erase(value.size() - 1);
    for (size_t i = 0; i < key.size(); ++i)
      key[i] = static_cast<char>(std::tolower(key[i]));
    headers[key] = value;
  }
  std::map<std::string, std::string>::iterator it =
      headers.find("transfer-encoding");
  if (it != headers.end() && it->second == "chunked")
    is_chunked = true;
  it = headers.find("content-length");
  if (it != headers.end()) {
    const char *str = it->second.c_str();
    char *endptr;
    errno = 0;
    unsigned long len = std::strtoul(str, &endptr, 10);
    if (errno == ERANGE || endptr == str || *endptr != '\0') {
      state = ERROR;
      error_message = "Invalid Content-Length";
      return false;
    }
    content_length = static_cast<size_t>(len);
    // Pre-reserve body capacity to avoid reallocations
    body.reserve(content_length);
  }
  if (is_chunked && content_length > 0) {
    state = ERROR;
    error_message = "Bad request: chunked AND content-length";
    return false;
  }
  if (is_chunked || content_length > 0)
    state = BODY;
  else
    state = DONE;

  return (true);
}

bool HTTPRequest::tryParseBody() {
  if (!is_chunked && content_length == 0) {
    state = DONE;
    return true;
  }
  if (is_chunked) {
    while (parse_offset < buffer.size() && body_state != CHUNK_END) {
      switch (body_state) {
      case NONE:
      case CHUNK_SIZE:
        if (!parseChunkSizeLine())
          return false;
        break;

      case CHUNK_DATA:
        if (!parseChunkData())
          return false;
        break;

      case CHUNK_CRLF:
        if (!parseChunkEnding())
          return false;
        break;

      default:
        break;
      }
    }
    if (body_state == CHUNK_END) {
      state = DONE;
      return true;
    }
    return false;
  }
  size_t available = buffer.size() - parse_offset;
  size_t remaining = content_length - body_bytes_received;
  size_t to_copy = (available < remaining) ? available : remaining;
  if (to_copy > 0) {
    body.insert(body.end(), buffer.begin() + parse_offset,
                buffer.begin() + parse_offset + to_copy);
    parse_offset += to_copy;
    body_bytes_received += to_copy;
  }
  if (body_bytes_received >= content_length) {
    state = DONE;
    return true;
  }
  return false;
}

bool HTTPRequest::parseChunkSizeLine() {
  std::vector<char> crlf(2);
  crlf[0] = '\r';
  crlf[1] = '\n';
  std::vector<char>::iterator it = std::search(
      buffer.begin() + parse_offset, buffer.end(), crlf.begin(), crlf.end());
  if (it == buffer.end())
    return false;
  std::string line(buffer.begin() + parse_offset, it);
  char *endptr = 0;
  unsigned long size = std::strtoul(line.c_str(), &endptr, 16);
  if (endptr == line.c_str() || *endptr != '\0') {
    state = ERROR;
    error_message = "Invalid chunk size line";
    return false;
  }
  parse_offset += (it - (buffer.begin() + parse_offset)) + 2;
  current_chunk_size = static_cast<size_t>(size);
  current_chunk_received = 0;
  if (current_chunk_size == 0) {
    body_state = CHUNK_END;
  } else {
    body_state = CHUNK_DATA;
  }
  return true;
}

bool HTTPRequest::parseChunkData() {
  size_t available = buffer.size() - parse_offset;
  size_t remaining = current_chunk_size - current_chunk_received;
  size_t to_copy = (available < remaining) ? available : remaining;
  if (to_copy > 0) {
    body.insert(body.end(), buffer.begin() + parse_offset,
                buffer.begin() + parse_offset + to_copy);
    parse_offset += to_copy;
    current_chunk_received += to_copy;
  }
  if (current_chunk_received == current_chunk_size) {
    body_state = CHUNK_CRLF;
  }
  return true;
}

bool HTTPRequest::parseChunkEnding() {
  if (buffer.size() - parse_offset < 2)
    return false;
  if (buffer[parse_offset] != '\r' || buffer[parse_offset + 1] != '\n') {
    state = ERROR;
    error_message = "Invalid chunk CRLF sequence";
    return false;
  }
  parse_offset += 2;
  if (current_chunk_size == 0)
    body_state = CHUNK_END;
  else
    body_state = CHUNK_SIZE;
  return true;
}

void HTTPRequest::parseQueryParams() {
  std::string::size_type qpos = target.find('?');
  if (qpos == std::string::npos)
    return;

  std::string path = target.substr(0, qpos);
  std::string query_string = target.substr(qpos + 1);
  target = path;
  std::string::size_type start = 0;
  while (start < query_string.size()) {
    std::string::size_type amp = query_string.find('&', start);
    if (amp == std::string::npos)
      amp = query_string.size();
    std::string pair = query_string.substr(start, amp - start);
    std::string::size_type eq = pair.find('=');
    std::string key, value;
    if (eq != std::string::npos) {
      key = pair.substr(0, eq);
      value = pair.substr(eq + 1);
    } else {
      key = pair;
      value = "";
    }
    if (!key.empty())
      query_params[key] = value;
    start = amp + 1;
  }
}

void HTTPRequest::reset() {
  if (parse_offset < buffer.size()) {
    std::vector<char> leftover(buffer.begin() + parse_offset, buffer.end());
    buffer.swap(leftover);
  } else {
    buffer.clear();
  }
  parse_offset = 0;
  method.clear();
  target.clear();
  version.clear();
  headers.clear();
  query_params.clear();
  body.clear();
  state = START_LINE;
  body_state = NONE;
  is_chunked = false;
  content_length = 0;
  body_bytes_received = 0;
  current_chunk_size = 0;
  current_chunk_received = 0;
  error_message.clear();
}

bool HTTPRequest::isComplete() const { return state == DONE; }

bool HTTPRequest::hasError() const { return state == ERROR; }

bool HTTPRequest::hasBody() const {
  return !body.empty() || content_length > 0 || is_chunked;
}

std::string HTTPRequest::getMethod() const { return method; }

std::string HTTPRequest::getTarget() const { return target; }

std::string HTTPRequest::getVersion() const { return version; }

std::string HTTPRequest::getHeader(const std::string &key) const {
  std::string k = key;
  for (size_t i = 0; i < k.size(); ++i)
    k[i] = std::tolower(k[i]);

  std::map<std::string, std::string>::const_iterator it = headers.find(k);
  if (it != headers.end())
    return it->second;

  return "";
}

std::vector<char> HTTPRequest::getBody() const { return body; }

std::string HTTPRequest::getBodyAsString() const {
  if (body.empty())
    return "";
  return std::string(&body[0], body.size());
}

std::string HTTPRequest::getQueryParam_ByName(const std::string &key) const {
  std::map<std::string, std::string>::const_iterator it =
      query_params.find(key);
  if (it != query_params.end())
    return it->second;
  return "";
}

std::map<std::string, std::string> HTTPRequest::getQueryParams() const {
  return query_params;
}

const std::string &HTTPRequest::getErrorMessage() const {
  return error_message;
}

void HTTPRequest::printRequest() const {
  std::cout << method << " " << target << " " << version << "\n";
  for (std::map<std::string, std::string>::const_iterator it = headers.begin();
       it != headers.end(); ++it) {
    std::cout << it->first << ": " << it->second << "\n";
  }
  std::cout << "\n";
  if (!body.empty()) {
    std::string body_str(body.begin(), body.end());
    std::cout << body_str << "\n";
  }
}