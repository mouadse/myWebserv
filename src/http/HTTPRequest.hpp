/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:48:34 by webserv          #+#    #+#             */
/*   Updated: 2025/12/12 13:04:00 by webserv           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

class HTTPRequest {
public:
  enum ParseState { START_LINE, HEADERS, BODY, DONE, ERROR };

  enum BodyParseState { NONE, CHUNK_SIZE, CHUNK_DATA, CHUNK_CRLF, CHUNK_END };

private:
  static const size_t MAX_URI_SIZE = 8192;
  static const size_t MAX_HEADER_SIZE = 32768;
  ParseState state;
  BodyParseState body_state;
  std::vector<char> buffer;
  size_t parse_offset;
  std::string method;
  std::string target;
  std::string version;
  std::map<std::string, std::string> headers;
  std::vector<char> body;
  std::map<std::string, std::string> query_params;
  std::string query_string;
  std::string error_message;
  bool is_chunked;
  size_t content_length;
  size_t body_bytes_received;
  size_t current_chunk_size;
  size_t current_chunk_received;
  bool tryParseRequestLine();
  bool tryParseHeaders();
  bool tryParseBody();
  bool parseChunkSizeLine();
  bool parseChunkData();
  bool parseChunkEnding();
  void parseQueryParams();

public:
  HTTPRequest();
  HTTPRequest(const std::string &raw_request);
  void addData(const std::vector<char> &new_chunk);
  void addData(const char *data, size_t len); // Zero-copy overload
  void reset();
  bool isComplete() const;
  bool hasError() const;
  bool hasBody() const;
  std::string getMethod() const;
  std::string getTarget() const;
  std::string getVersion() const;
  std::string getHeader(const std::string &key) const;
  std::string getBodyAsString() const;
  std::vector<char> getBody() const;
  std::string getQueryString() const;
  std::string getQueryParam_ByName(const std::string &key) const;
  std::map<std::string, std::string> getQueryParams() const;
  const std::string &getErrorMessage() const;
  void printRequest() const;
};

#endif