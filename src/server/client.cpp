/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/29 13:37:51 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/20 21:47:52 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "client.hpp"

Client::Client(int fd)
    : fd(fd),
      writeOffset(0),
      wantWrite(false),
      closeAfterWrite(false),
      fileResponse(false),
      streaming(false),
      streamFd(-1),
      streamRemaining(0),
      streamBufferOffset(0),
      streamBufferSize(0),
      cgiActive(false),
      cgiPid(-1),
      cgiInFd(-1),
      cgiOutFd(-1),
      cgiInputOffset(0),
      cgiInputClosed(false),
      cgiOutputClosed(false),
      cgiExited(false),
      cgiExitStatus(0),
      cgiTimedOut(false),
      cgiStripBody(false),
      cgiStartMs(0) {
  if (Logger::isDebugEnabled())
    Logger::debug("Client constructor called for FD: " + Helpers::toString(fd));

  if (fd < 0) {
    Logger::error("Invalid client file descriptor: " + Helpers::toString(fd));
    throw std::runtime_error("Invalid client file descriptor");
  }

  if (Logger::isDebugEnabled())
    Logger::debug("Client FD set to: " + Helpers::toString(this->fd));

  this->streamBuffer.resize(STREAM_CHUNK_SIZE);
  
  if (Logger::isDebugEnabled())
    Logger::debug("Client state initialized");

  if (Logger::isDebugEnabled())
    Logger::debug("Client object created for FD: " + Helpers::toString(fd));
}
