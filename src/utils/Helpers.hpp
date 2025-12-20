/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Helpers.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/08 01:49:08 by ebelkadi          #+#    #+#             */
/*   Updated: 2025/12/20 21:58:42 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <sstream>
#include <string>
#include <map>
#include <vector>
#include "../server/server.hpp"


class Server;

class Helpers {
	public:
		Helpers();
		Helpers(const Helpers &other);
		Helpers &operator=(const Helpers &other);
		~Helpers();

        template <typename T>
		static std::string toString(const T &value)
        {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
        static std::string toString(const std::vector<char> &value)
        {
            return std::string(value.begin(), value.end());
        }
        static std::string toString(const std::map<std::string, std::string> &value)
        {
            std::ostringstream oss;
            bool first = true;
            for (std::map<std::string, std::string>::const_iterator it = value.begin(); it != value.end(); ++it)
            {
                if (!first)
                    oss << ", ";
                oss << it->first << ": " << it->second;
                first = false;
            }
            return oss.str();
        }

        
        struct FdInfo {
            int serverIndex;
            int type;
        };

        static FdInfo findServerByFd(int fd, const std::vector<Server> &servers);

        static std::string escapeHtml(const std::string &data);

};

