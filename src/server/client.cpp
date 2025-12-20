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

Client::Client(int fd) {
  if (Logger::isDebugEnabled())
    Logger::debug("Client constructor called for FD: " + Helpers::toString(fd));

  if (fd < 0) {
    Logger::error("Invalid client file descriptor: " + Helpers::toString(fd));
    throw std::runtime_error("Invalid client file descriptor");
  }

  this->fd = fd;
  if (Logger::isDebugEnabled())
    Logger::debug("Client FD set to: " + Helpers::toString(this->fd));

  this->wantWrite = false;
  this->writeOffset = 0;
  this->closeAfterWrite = false;
  this->fileResponse = false;
  this->streaming = false;
  this->streamFd = -1;
  this->streamRemaining = 0;
  this->streamBufferOffset = 0;
  this->streamBufferSize = 0;
  this->streamBuffer.resize(STREAM_CHUNK_SIZE);
  if (Logger::isDebugEnabled())
    Logger::debug("Client state initialized");

  if (Logger::isDebugEnabled())
    Logger::debug("Client object created for FD: " + Helpers::toString(fd));
}
