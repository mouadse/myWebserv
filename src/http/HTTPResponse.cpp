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

HttpResponse::HttpResponse(){}
HttpResponse::HttpResponse(int code, const std::string &message)
    : statusCode(code), statusMessage(message) {}

void HttpResponse::setStatus(int code, const std::string &message) {
    statusCode = code;
    statusMessage = message;
}

void HttpResponse::setHeader(const std::string &name, const std::string &value) {
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
    body = bodyContent;
    setContentLength(body.size());
}

const std::string &HttpResponse::getBody() const {
    return (body);
}

std::string HttpResponse::toString() const {
    std::ostringstream ss;

    ss << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
    std::map<std::string, std::string>::const_iterator it = headers.begin();
    while (it != headers.end()) {
        ss << it->first << ": " << it->second << "\r\n";
        ++it;
    }
    ss << "\r\n";
    ss << body;
    return ss.str();
}

void HttpResponse::setContentLength(size_t len) {
    std::ostringstream ss;
    ss << len;
    setHeader("Content-Length", ss.str());
}


void HttpResponse::setContentType(const std::string &type) {
    setHeader("Content-Type", type);
}

void HttpResponse::printResponse() const {
    std::string responseStr = toString();
    std::cout << responseStr << std::endl;
}
