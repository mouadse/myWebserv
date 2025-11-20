#include "../include/Location.hpp"

Location::Location() : _path(""), _root(""), _autoindex(false), _index(""), _methods(5, 0), _return(""), _alias(""), _client_max_body_size(kDefaultMaxBodySize) {}

Location::Location(const Location &other) : _path(other._path), _root(other._root), _autoindex(other._autoindex), _index(other._index), _methods(other._methods), _return(other._return), _alias(other._alias), _cgi_path(other._cgi_path), _cgi_ext(other._cgi_ext), _client_max_body_size(other._client_max_body_size), _ext_path(other._ext_path) {}

Location &Location::operator=(const Location &rhs)
{
	if (this != &rhs)
	{
		_path = rhs._path;
		_root = rhs._root;
		_autoindex = rhs._autoindex;
		_index = rhs._index;
		_methods = rhs._methods;
		_return = rhs._return;
		_alias = rhs._alias;
		_cgi_path = rhs._cgi_path;
		_cgi_ext = rhs._cgi_ext;
		_client_max_body_size = rhs._client_max_body_size;
		_ext_path = rhs._ext_path;
	}
	return (*this);
}

Location::~Location() {}

void Location::setPath(const std::string &parameter)
{
	_path = parameter;
}

void Location::setRootLocation(const std::string &parameter)
{
	if (ConfigFile::getTypePath(parameter) != 2)
		throw ConfigError("root of location is invalid: " + parameter);
	_root = parameter;
}

void Location::setMethods(const std::vector<std::string> &methods)
{
	_methods.assign(5, 0);
	for (size_t i = 0; i < methods.size(); i++)
	{
		if (methods[i] == "GET")
			_methods[0] = 1;
		else if (methods[i] == "POST")
			_methods[1] = 1;
		else if (methods[i] == "DELETE")
			_methods[2] = 1;
		else if (methods[i] == "PUT")
			_methods[3] = 1;
		else if (methods[i] == "HEAD")
			_methods[4] = 1;
		else
			throw ConfigError("Allow method not supported: " + methods[i]);
	}
}

static std::string dropSemicolonIfAny(std::string value, const std::string &context)
{
	if (!value.empty() && value[value.size() - 1] == ';')
		requireTrailingSemicolon(value, context);
	return (value);
}

void Location::setAutoindex(const std::string &parameter)
{
	std::string value = dropSemicolonIfAny(parameter, "autoindex");
	if (value == "on")
		_autoindex = true;
	else if (value == "off")
		_autoindex = false;
	else
		throw ConfigError("Wrong autoindex value: " + value);
}

void Location::setIndexLocation(const std::string &parameter)
{
	_index = dropSemicolonIfAny(parameter, "index");
}

void Location::setReturn(const std::string &parameter)
{
	_return = dropSemicolonIfAny(parameter, "return");
}

void Location::setAlias(const std::string &parameter)
{
	_alias = dropSemicolonIfAny(parameter, "alias");
}

void Location::setCgiPath(const std::vector<std::string> &path)
{
	_cgi_path = path;
}

void Location::setCgiExtension(const std::vector<std::string> &extension)
{
	_cgi_ext = extension;
}

void Location::setMaxBodySize(const std::string &parameter)
{
	std::string value = dropSemicolonIfAny(parameter, "client_max_body_size");
	unsigned long body_size = 0;
	if (!isDigits(value))
		throw ConfigError("Wrong syntax: client_max_body_size");
	body_size = ft_stoi(value);
	if (body_size == 0)
		throw ConfigError("Wrong syntax: client_max_body_size");
	_client_max_body_size = body_size;
}

void Location::setMaxBodySize(unsigned long parameter)
{
	_client_max_body_size = parameter;
}

const std::string &Location::getPath() const { return (_path); }
const std::string &Location::getRootLocation() const { return (_root); }
const std::string &Location::getIndexLocation() const { return (_index); }
const std::vector<short> &Location::getMethods() const { return (_methods); }
const std::vector<std::string> &Location::getCgiPath() const { return (_cgi_path); }
const std::vector<std::string> &Location::getCgiExtension() const { return (_cgi_ext); }
const bool &Location::getAutoindex() const { return (_autoindex); }
const std::string &Location::getReturn() const { return (_return); }
const std::string &Location::getAlias() const { return (_alias); }
const std::map<std::string, std::string> &Location::getExtensionPath() const { return (_ext_path); }
const unsigned long &Location::getMaxBodySize() const { return (_client_max_body_size); }

std::string Location::getPrintMethods() const
{
	std::string res;
	if (_methods[4])
		res.insert(0, "HEAD");
	if (_methods[3])
	{
		if (!res.empty())
			res.insert(0, ", ");
		res.insert(0, "PUT");
	}
	if (_methods[2])
	{
		if (!res.empty())
			res.insert(0, ", ");
		res.insert(0, "DELETE");
	}
	if (_methods[1])
	{
		if (!res.empty())
			res.insert(0, ", ");
		res.insert(0, "POST");
	}
	if (_methods[0])
	{
		if (!res.empty())
			res.insert(0, ", ");
		res.insert(0, "GET");
	}
	return (res);
}
