/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:49:06 by webserv          #+#    #+#             */
/*   Updated: 2025/12/20 21:43:52 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

class HttpResponse {

private:
  int statusCode;
  std::string statusMessage;
  std::map<std::string, std::string> headers;
  std::vector<char> body;
  bool fileBodyEnabled;
  std::string filePath;
  off_t fileOffset;
  size_t fileLength;
  void setContentLength(size_t len);
  void clearFileBody();

public:
  HttpResponse();
  HttpResponse(int code, const std::string &message);
  void setStatus(int code, const std::string &message);
  void setHeader(const std::string &name, const std::string &value);
  bool hasHeader(const std::string &name) const;
  const std::string &getHeader(const std::string &name) const;
  void setBody(const std::string &bodyContent);
  void setBody(const std::vector<char> &bodyContent);
  void setBody(const char *data, size_t len);
  void setFileBody(const std::string &path, off_t offset, size_t length);
  bool hasFileBody() const;
  const std::string &getFilePath() const;
  off_t getFileOffset() const;
  size_t getFileLength() const;
  std::string getBodyAsString() const;
  const std::vector<char> &getBody() const;
  std::string toString() const;
  void toBuffer(std::vector<char> &out) const;
  void setContentType(const std::string &type);
  void printResponse() const;
};

#endif
