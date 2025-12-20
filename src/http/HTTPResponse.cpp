/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebelkadi <ebelkadi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:48:56 by webserv          #+#    #+#             */
/*   Updated: 2025/12/02 17:28:08 by ebelkadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPResponse.hpp"

HttpResponse::HttpResponse()
    : statusCode(200), statusMessage("OK"), fileBodyEnabled(false),
      fileOffset(0), fileLength(0) {}

HttpResponse::HttpResponse(int code, const std::string &message)
    : statusCode(code), statusMessage(message), fileBodyEnabled(false),
      fileOffset(0), fileLength(0) {}

void HttpResponse::setStatus(int code, const std::string &message) {
  statusCode = code;
  statusMessage = message;
}

void HttpResponse::setHeader(const std::string &name,
                             const std::string &value) {
  headers[name] = value;
}

bool HttpResponse::hasHeader(const std::string &name) const {
  return (headers.find(name) != headers.end());
}

const std::string &HttpResponse::getHeader(const std::string &name) const {
  std::map<std::string, std::string>::const_iterator it = headers.find(name);
  if (it == headers.end())
    throw std::runtime_error("Header not found: " + name);
  return (it->second);
}

void HttpResponse::setBody(const std::string &bodyContent) {
  clearFileBody();
  body.assign(bodyContent.begin(), bodyContent.end());
  setContentLength(body.size());
}

void HttpResponse::setBody(const std::vector<char> &bodyContent) {
  clearFileBody();
  body = bodyContent;
  setContentLength(body.size());
}

void HttpResponse::setBody(const char *data, size_t len) {
  clearFileBody();
  body.assign(data, data + len);
  setContentLength(body.size());
}

void HttpResponse::setFileBody(const std::string &path, off_t offset,
                               size_t length) {
  body.clear();
  fileBodyEnabled = true;
  filePath = path;
  fileOffset = offset;
  fileLength = length;
  setContentLength(length);
}

bool HttpResponse::hasFileBody() const { return fileBodyEnabled; }

const std::string &HttpResponse::getFilePath() const { return filePath; }

off_t HttpResponse::getFileOffset() const { return fileOffset; }

size_t HttpResponse::getFileLength() const { return fileLength; }

std::string HttpResponse::getBodyAsString() const {
  if (body.empty())
    return "";
  return std::string(&body[0], body.size());
}

const std::vector<char> &HttpResponse::getBody() const { return body; }

// Efficient pre-sized buffer construction using memcpy
void HttpResponse::toBuffer(std::vector<char> &out) const {
  // Build status line
  std::ostringstream statusLine;
  statusLine << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
  std::string sl = statusLine.str();

  // Build headers
  std::string hdrs;
  for (std::map<std::string, std::string>::const_iterator it = headers.begin();
       it != headers.end(); ++it) {
    hdrs += it->first + ": " + it->second + "\r\n";
  }
  hdrs += "\r\n";

  // Calculate total size and pre-allocate
  size_t totalSize = sl.size() + hdrs.size() + body.size();
  size_t oldSize = out.size();
  out.resize(oldSize + totalSize);

  // Use memcpy for efficient copying
  char *dest = &out[oldSize];
  std::memcpy(dest, sl.c_str(), sl.size());
  dest += sl.size();
  std::memcpy(dest, hdrs.c_str(), hdrs.size());
  dest += hdrs.size();
  if (!body.empty()) {
    std::memcpy(dest, &body[0], body.size());
  }
}

std::string HttpResponse::toString() const {
  std::vector<char> buf;
  toBuffer(buf);
  if (buf.empty())
    return "";
  return std::string(&buf[0], buf.size());
}

void HttpResponse::setContentLength(size_t len) {
  std::ostringstream ss;
  ss << len;
  setHeader("Content-Length", ss.str());
}

void HttpResponse::setContentType(const std::string &type) {
  setHeader("Content-Type", type);
}

void HttpResponse::clearFileBody() {
  fileBodyEnabled = false;
  filePath.clear();
  fileOffset = 0;
  fileLength = 0;
}

void HttpResponse::printResponse() const {
  std::string responseStr = toString();
  std::cout << responseStr << std::endl;
}
