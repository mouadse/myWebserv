#include "../include/ServerConfig.hpp"

#include <unistd.h>
#include <cerrno>

ServerConfig::ServerConfig() : _port(0), _host(0), _server_name(""), _root(""), _client_max_body_size(kDefaultMaxBodySize), _index(""), _autoindex(false), _error_pages(), _locations(), _server_address(), _listen_fd(-1)
{
	initErrorPages();
}

ServerConfig::~ServerConfig() {}

ServerConfig::ServerConfig(const ServerConfig &other) : _port(other._port), _host(other._host), _server_name(other._server_name), _root(other._root), _client_max_body_size(other._client_max_body_size), _index(other._index), _autoindex(other._autoindex), _error_pages(other._error_pages), _locations(other._locations), _server_address(other._server_address), _listen_fd(other._listen_fd) {}

ServerConfig &ServerConfig::operator=(const ServerConfig & rhs)
{
	if (this != &rhs)
	{
		_port = rhs._port;
		_host = rhs._host;
		_server_name = rhs._server_name;
		_root = rhs._root;
		_client_max_body_size = rhs._client_max_body_size;
		_index = rhs._index;
		_autoindex = rhs._autoindex;
		_error_pages = rhs._error_pages;
		_locations = rhs._locations;
		_server_address = rhs._server_address;
		_listen_fd = rhs._listen_fd;
	}
	return (*this);
}

void ServerConfig::initErrorPages(void)
{
	_error_pages[301] = "";
	_error_pages[302] = "";
	_error_pages[400] = "";
	_error_pages[401] = "";
	_error_pages[402] = "";
	_error_pages[403] = "";
	_error_pages[404] = "";
	_error_pages[405] = "";
	_error_pages[406] = "";
	_error_pages[500] = "";
	_error_pages[501] = "";
	_error_pages[502] = "";
	_error_pages[503] = "";
	_error_pages[505] = "";
}

static std::string normalizeWithSemicolon(std::string value, const std::string &context)
{
	requireTrailingSemicolon(value, context);
	return (value);
}

void ServerConfig::setServerName(std::string server_name)
{
	_server_name = normalizeWithSemicolon(server_name, "server_name");
}

void ServerConfig::setHost(std::string parametr)
{
	parametr = normalizeWithSemicolon(parametr, "host");
	if (parametr == "localhost")
		parametr = "127.0.0.1";
	if (!isValidHost(parametr))
		throw ConfigError("Wrong syntax: host");
	_host = inet_addr(parametr.data());
}

void ServerConfig::setRoot(std::string root)
{
	root = normalizeWithSemicolon(root, "root");
	if (ConfigFile::getTypePath(root) == 2)
	{
		_root = root;
		return ;
	}
	char dir[1024];
	if (!getcwd(dir, sizeof(dir)))
		throw ConfigError("Failed to resolve working directory");
	std::string full_root = std::string(dir) + "/" + root;
	if (ConfigFile::getTypePath(full_root) != 2)
		throw ConfigError("Wrong syntax: root");
	_root = full_root;
}

void ServerConfig::setPort(std::string parametr)
{
	parametr = normalizeWithSemicolon(parametr, "port");
	for (size_t i = 0; i < parametr.length(); i++)
	{
		if (!std::isdigit(parametr[i]))
			throw ConfigError("Wrong syntax: port");
	}
	unsigned int port = ft_stoi((parametr));
	if (port < 1 || port > 65636)
		throw ConfigError("Wrong syntax: port");
	_port = static_cast<uint16_t>(port);
}

void ServerConfig::setClientMaxBodySize(std::string parametr)
{
	parametr = normalizeWithSemicolon(parametr, "client_max_body_size");
	if (!isDigits(parametr))
		throw ConfigError("Wrong syntax: client_max_body_size");
	unsigned long body_size = ft_stoi(parametr);
	if (body_size == 0)
		throw ConfigError("Wrong syntax: client_max_body_size");
	_client_max_body_size = body_size;
}

void ServerConfig::setIndex(std::string index)
{
	_index = normalizeWithSemicolon(index, "index");
}

void ServerConfig::setAutoindex(std::string autoindex)
{
	autoindex = normalizeWithSemicolon(autoindex, "autoindex");
	if (autoindex != "on" && autoindex != "off")
		throw ConfigError("Wrong syntax: autoindex");
	if (autoindex == "on")
		_autoindex = true;
}

void ServerConfig::setErrorPages(std::vector<std::string> parametr)
{
	if (parametr.empty())
		return;
	if (parametr.size() % 2 != 0)
		throw ConfigError("Error page initialization failed");
	for (size_t i = 0; i + 1 < parametr.size(); i += 2)
	{
		for (size_t j = 0; j < parametr[i].size(); j++)
		{
			if (!std::isdigit(parametr[i][j]))
				throw ConfigError("Error code is invalid");
		}
		if (parametr[i].size() != 3)
			throw ConfigError("Error code is invalid");
		short code_error = ft_stoi(parametr[i]);
		if (statusCodeString(code_error) == "Undefined" || code_error < 400)
			throw ConfigError("Incorrect error code: " + parametr[i]);
		std::string path = parametr[i + 1];
		if (!path.empty() && path[path.size() - 1] == ';')
			path = normalizeWithSemicolon(path, "error_page");
		if (ConfigFile::getTypePath(path) != 2)
		{
			if (ConfigFile::getTypePath(_root + path) != 1)
				throw ConfigError("Incorrect path for error page file: " + _root + path);
			if (ConfigFile::checkFile(_root + path, 0) == -1 || ConfigFile::checkFile(_root + path, 4) == -1)
				throw ConfigError("Error page file :" + _root + path + " is not accessible");
		}
		std::map<short, std::string>::iterator it = _error_pages.find(code_error);
		if (it != _error_pages.end())
			_error_pages[code_error] = path;
		else
			_error_pages.insert(std::make_pair(code_error, path));
	}
}

void ServerConfig::setLocation(std::string path, std::vector<std::string> parametr)
{
	Location new_location;
	bool flag_methods = false;
	bool flag_autoindex = false;
	bool flag_max_size = false;
	int valid;

	new_location.setPath(path);
	for (size_t i = 0; i < parametr.size(); i++)
	{
		if (parametr[i] == "root" && (i + 1) < parametr.size())
		{
			if (!new_location.getRootLocation().empty())
				throw ConfigError("Root of location is duplicated");
			std::string value = parametr[++i];
			value = normalizeWithSemicolon(value, "location root");
			if (ConfigFile::getTypePath(value) == 2)
				new_location.setRootLocation(value);
			else
				new_location.setRootLocation(_root + value);
		}
		else if ((parametr[i] == "allow_methods" || parametr[i] == "methods") && (i + 1) < parametr.size())
		{
			if (flag_methods)
				throw ConfigError("Allow_methods of location is duplicated");
			std::vector<std::string> methods;
			while (++i < parametr.size())
			{
				if (parametr[i].find(";") != std::string::npos)
				{
					std::string value = parametr[i];
					value = normalizeWithSemicolon(value, "allow_methods");
					methods.push_back(value);
					break ;
				}
				else
				{
					methods.push_back(parametr[i]);
					if (i + 1 >= parametr.size())
						throw ConfigError("Token is invalid");
				}
			}
			new_location.setMethods(methods);
			flag_methods = true;
		}
		else if (parametr[i] == "autoindex" && (i + 1) < parametr.size())
		{
			if (path == "/cgi-bin")
				throw ConfigError("Parametr autoindex not allow for CGI");
			if (flag_autoindex)
				throw ConfigError("Autoindex of location is duplicated");
			std::string value = parametr[++i];
			new_location.setAutoindex(value);
			flag_autoindex = true;
		}
		else if (parametr[i] == "index" && (i + 1) < parametr.size())
		{
			if (!new_location.getIndexLocation().empty())
				throw ConfigError("Index of location is duplicated");
			std::string value = parametr[++i];
			new_location.setIndexLocation(value);
		}
		else if (parametr[i] == "return" && (i + 1) < parametr.size())
		{
			if (path == "/cgi-bin")
				throw ConfigError("Parametr return not allow for CGI");
			if (!new_location.getReturn().empty())
				throw ConfigError("Return of location is duplicated");
			std::string value = parametr[++i];
			new_location.setReturn(value);
		}
		else if (parametr[i] == "alias" && (i + 1) < parametr.size())
		{
			if (path == "/cgi-bin")
				throw ConfigError("Parametr alias not allow for CGI");
			if (!new_location.getAlias().empty())
				throw ConfigError("Alias of location is duplicated");
			std::string value = parametr[++i];
			new_location.setAlias(value);
		}
		else if (parametr[i] == "cgi_ext" && (i + 1) < parametr.size())
		{
			std::vector<std::string> extension;
			while (++i < parametr.size())
			{
				if (parametr[i].find(";") != std::string::npos)
				{
					std::string value = parametr[i];
					value = normalizeWithSemicolon(value, "cgi_ext");
					extension.push_back(value);
					break ;
				}
				else
				{
					extension.push_back(parametr[i]);
					if (i + 1 >= parametr.size())
						throw ConfigError("Token is invalid");
				}
			}
			new_location.setCgiExtension(extension);
		}
		else if (parametr[i] == "cgi_path" && (i + 1) < parametr.size())
		{
			std::vector<std::string> path_list;
			while (++i < parametr.size())
			{
				if (parametr[i].find(";") != std::string::npos)
				{
					std::string value = parametr[i];
					value = normalizeWithSemicolon(value, "cgi_path");
					path_list.push_back(value);
					break ;
				}
				else
				{
					path_list.push_back(parametr[i]);
					if (i + 1 >= parametr.size())
						throw ConfigError("Token is invalid");
				}
				if (parametr[i].find("/python") == std::string::npos && parametr[i].find("/bash") == std::string::npos)
					throw ConfigError("cgi_path is invalid");
			}
			new_location.setCgiPath(path_list);
		}
		else if (parametr[i] == "client_max_body_size" && (i + 1) < parametr.size())
		{
			if (flag_max_size)
				throw ConfigError("Maxbody_size of location is duplicated");
			std::string value = parametr[++i];
			new_location.setMaxBodySize(value);
			flag_max_size = true;
		}
		else if (i < parametr.size())
			throw ConfigError("Parametr in a location is invalid");
	}
	if (new_location.getPath() != "/cgi-bin" && new_location.getIndexLocation().empty())
		new_location.setIndexLocation(_index);
	if (!flag_max_size)
		new_location.setMaxBodySize(_client_max_body_size);
	valid = isValidLocation(new_location);
	if (valid == 1)
		throw ConfigError("Failed CGI validation");
	else if (valid == 2)
		throw ConfigError("Failed path in location validation");
	else if (valid == 3)
		throw ConfigError("Failed redirection file in location validation");
	else if (valid == 4)
		throw ConfigError("Failed alias file in location validation");
	_locations.push_back(new_location);
}

void	ServerConfig::setFd(int fd)
{
	_listen_fd = fd;
}

bool ServerConfig::isValidHost(std::string host) const
{
	struct sockaddr_in sockaddr;
	return (inet_pton(AF_INET, host.c_str(), &(sockaddr.sin_addr)) ? true : false);
}

bool ServerConfig::isValidErrorPages()
{
	std::map<short, std::string>::const_iterator it;
	for (it = _error_pages.begin(); it != _error_pages.end(); it++)
	{
		if (it->first < 100 || it->first > 599)
			return (false);
		if (ConfigFile::checkFile(getRoot() + it->second, 0) < 0 || ConfigFile::checkFile(getRoot() + it->second, 4) < 0)
			return (false);
	}
	return (true);
}

int ServerConfig::isValidLocation(Location &location) const
{
	if (location.getPath() == "/cgi-bin")
	{
		if (location.getCgiPath().empty() || location.getCgiExtension().empty() || location.getIndexLocation().empty())
			return (1);
		if (ConfigFile::checkFile(location.getIndexLocation(), 4) < 0)
		{
			std::string path = location.getRootLocation() + location.getPath() + "/" + location.getIndexLocation();
			if (ConfigFile::getTypePath(path) != 1)
			{
				char root[1024];
				if (!getcwd(root, sizeof(root)))
					return (1);
				location.setRootLocation(root);
				path = std::string(root) + location.getPath() + "/" + location.getIndexLocation();
			}
			if (path.empty() || ConfigFile::getTypePath(path) != 1 || ConfigFile::checkFile(path, 4) < 0)
				return (1);
		}
		if (location.getCgiPath().size() != location.getCgiExtension().size())
			return (1);
		std::vector<std::string>::const_iterator it;
		for (it = location.getCgiPath().begin(); it != location.getCgiPath().end(); ++it)
		{
			if (ConfigFile::getTypePath(*it) < 0)
				return (1);
		}
		std::vector<std::string>::const_iterator it_path;
		for (it = location.getCgiExtension().begin(); it != location.getCgiExtension().end(); ++it)
		{
			std::string tmp = *it;
			if (tmp != ".py" && tmp != ".sh" && tmp != "*.py" && tmp != "*.sh")
				return (1);
			for (it_path = location.getCgiPath().begin(); it_path != location.getCgiPath().end(); ++it_path)
			{
				std::string tmp_path = *it_path;
				if (tmp == ".py" || tmp == "*.py")
				{
					if (tmp_path.find("python") != std::string::npos)
						location._ext_path.insert(std::make_pair(".py", tmp_path));
				}
				else if (tmp == ".sh" || tmp == "*.sh")
				{
					if (tmp_path.find("bash") != std::string::npos)
						location._ext_path[".sh"] = tmp_path;
				}
			}
		}
		if (location.getCgiPath().size() != location.getExtensionPath().size())
			return (1);
	}
	else
	{
		if (location.getPath().empty() || location.getPath()[0] != '/')
			return (2);
		if (location.getRootLocation().empty())
			location.setRootLocation(_root);
		if (ConfigFile::isFileExistAndReadable(location.getRootLocation() + location.getPath() + "/", location.getIndexLocation()))
			return (5);
		if (!location.getReturn().empty())
		{
			if (ConfigFile::isFileExistAndReadable(location.getRootLocation(), location.getReturn()))
				return (3);
		}
		if (!location.getAlias().empty())
		{
			if (ConfigFile::isFileExistAndReadable(location.getRootLocation(), location.getAlias()))
			 	return (4);
		}
	}
	return (0);
}

const std::string &ServerConfig::getServerName() const { return (_server_name); }
const std::string &ServerConfig::getRoot() const { return (_root); }
const bool &ServerConfig::getAutoindex() const { return (_autoindex); }
const in_addr_t &ServerConfig::getHost() const { return (_host); }
const uint16_t &ServerConfig::getPort() const { return (_port); }
const size_t &ServerConfig::getClientMaxBodySize() const { return (_client_max_body_size); }
const std::vector<Location> &ServerConfig::getLocations() const { return (_locations); }
const std::map<short, std::string> &ServerConfig::getErrorPages() const { return (_error_pages); }
const std::string &ServerConfig::getIndex() const { return (_index); }
int ServerConfig::getFd() const { return (_listen_fd); }

const std::string &ServerConfig::getPathErrorPage(short key) const
{
	std::map<short, std::string>::const_iterator it = _error_pages.find(key);
	if (it == _error_pages.end())
		throw ConfigError("Error_page does not exist");
	return (it->second);
}

std::vector<Location>::const_iterator ServerConfig::getLocationKey(const std::string &key) const
{
	std::vector<Location>::const_iterator it;
	for (it = _locations.begin(); it != _locations.end(); it++)
	{
		if (it->getPath() == key)
			return (it);
	}
	throw ConfigError("Error: path to location not found");
}

void ServerConfig::checkToken(std::string &parametr)
{
	requireTrailingSemicolon(parametr, "directive");
}

bool ServerConfig::checkLocaitons() const
{
	if (_locations.size() < 2)
		return (false);
	std::vector<Location>::const_iterator it1;
	std::vector<Location>::const_iterator it2;
	for (it1 = _locations.begin(); it1 != _locations.end() - 1; it1++) {
		for (it2 = it1 + 1; it2 != _locations.end(); it2++) {
			if (it1->getPath() == it2->getPath())
				return (true);
		}
	}
	return (false);
}

void ServerConfig::setupServer(void)
{
	_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listen_fd  == -1 )
		throw ConfigError(std::string("socket error: ") + strerror(errno));
	int option_value = 1;
	setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int));
	std::memset(&_server_address, 0, sizeof(_server_address));
	_server_address.sin_family = AF_INET;
	_server_address.sin_addr.s_addr = _host;
	_server_address.sin_port = htons(_port);
	if (bind(_listen_fd, (struct sockaddr *) &_server_address, sizeof(_server_address)) == -1)
	{
		close(_listen_fd);
		throw ConfigError(std::string("bind error: ") + strerror(errno));
	}
}
