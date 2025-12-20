/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Helpers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/08 01:49:12 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/20 21:58:05 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Helpers.hpp"
#include "../server/server.hpp"
#include <vector>

Helpers::Helpers() {}

Helpers::Helpers(const Helpers &other) { (void)other; }

Helpers &Helpers::operator=(const Helpers &other) {
  if (this != &other) {
  }
  return *this;
}

Helpers::~Helpers() {}

Helpers::FdInfo Helpers::findServerByFd(int fd,
                                        const std::vector<Server> &servers) {
  FdInfo result;

  for (size_t j = 0; j < servers.size(); ++j) {
    if (fd == servers[j].server_fd) {
      result.serverIndex = j;
      result.type = 0;
      return result;
    }
  }

  for (size_t i = 0; i < servers.size(); ++i) {
    if (servers[i].hasClient(fd)) {
      result.serverIndex = i;
      result.type = 1;
      return result;
    }
  }

  result.serverIndex = -1;
  result.type = -1;
  return result;
}

std::string Helpers::escapeHtml(const std::string &data) {
  std::string buffer;
  buffer.reserve(data.size() * 1.1);
  for (size_t i = 0; i < data.size(); ++i) {
    switch (data[i]) {
    case '&':
      buffer += "&amp;";
      break;
    case '\"':
      buffer += "&quot;";
      break;
    case '\'':
      buffer += "&apos;";
      break;
    case '<':
      buffer += "&lt;";
      break;
    case '>':
      buffer += "&gt;";
      break;
    default:
      buffer += data[i];
      break;
    }
  }
  return buffer;
}
