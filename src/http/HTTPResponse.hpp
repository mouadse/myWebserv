/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebelkadi <ebelkadi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:49:06 by webserv          #+#    #+#             */
/*   Updated: 2025/12/02 17:28:33 by ebelkadi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <stdexcept>
#include <iostream>

class HttpResponse {

    private:
        int statusCode;
        std::string statusMessage;
        std::map<std::string, std::string> headers;
        std::string body;
        void setContentLength(size_t len);
    public:
        HttpResponse();
        HttpResponse(int code, const std::string &message);
        void setStatus(int code, const std::string &message);
        void setHeader(const std::string &name, const std::string &value);
        bool hasHeader(const std::string &name) const;
        const std::string &getHeader(const std::string &name) const;
        void setBody(const std::string &bodyContent);
        const std::string &getBody() const;
        std::string toString() const;
        void setContentType(const std::string &type);
        void printResponse() const;
};

#endif
