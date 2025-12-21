/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:37:59 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/20 21:48:22 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <arpa/inet.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "../http/HTTPRequest.hpp"
#include "../http/HTTPResponse.hpp"
#include "./utils/Helpers.hpp"
#include "./utils/Logger.hpp"

class Client {
public:
  int fd;
  HTTPRequest request;
  HttpResponse response;
  std::vector<char> writeBuffer;
  size_t writeOffset;
  bool wantWrite;
  bool closeAfterWrite;
  bool fileResponse;
  static const size_t STREAM_CHUNK_SIZE = 64 * 1024;
  bool streaming;
  int streamFd;
  size_t streamRemaining;
  std::vector<char> streamBuffer;
  size_t streamBufferOffset;
  size_t streamBufferSize;
  bool cgiActive;
  pid_t cgiPid;
  int cgiInFd;
  int cgiOutFd;
  std::vector<char> cgiInput;
  size_t cgiInputOffset;
  std::vector<char> cgiOutput;
  bool cgiInputClosed;
  bool cgiOutputClosed;
  bool cgiExited;
  int cgiExitStatus;
  bool cgiTimedOut;
  bool cgiOutputOverflow;
  bool cgiStripBody;
  struct timeval cgiStartTime;
  Client(int fd);
};

#endif
