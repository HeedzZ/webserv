#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <iostream>
#include <iostream>
#include <fstream> 
#include <sstream>
#include <string>  
#include <cstdlib>
#include <map>
#include <vector>  


class ServerConfig {

private:
	std::string _pathToConfig;
	int _port;
    std::string _server_name;
    std::string _root;
    std::string _index;
    std::map<int, std::string> _error_pages;

public:
	ServerConfig(std::string av);
	void	parseConfigFile();
	~ServerConfig();

	int getPort();
};

#endif