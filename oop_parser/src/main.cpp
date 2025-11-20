#include "../include/ConfigParser.hpp"

#include <iostream>

int main(int argc, char **argv)
{
	std::string config_path = "example.conf";
	if (argc > 1)
		config_path = argv[1];

	try
	{
		ConfigParser parser;
		parser.createCluster(config_path);
		parser.print(std::cout);
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (1);
	}
	return (0);
}
