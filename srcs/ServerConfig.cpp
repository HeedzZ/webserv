#include "ServerConfig.hpp"
//std::cout << port << std::endl;
ServerConfig::ServerConfig(std::string av) : _pathToConfig(av)
{
}

void ServerConfig::parseConfigFile()
{
    std::ifstream configFile(_pathToConfig.c_str());
    if (!configFile.is_open())
	{
        std::cerr << "Impossible d'ouvrir le fichier de configuration : " << _pathToConfig << std::endl;
        return;
    }
    std::string line;
    while (std::getline(configFile, line))
	{
		if (line.find("listen") != std::string::npos)
		{
		    std::istringstream iss(line);
		    std::string command;
		    int port;

		    iss >> command >> port;
		    if (command == "listen")
		        _port = port;
		    else
		        _port = 8080;
		}
		else if (line.find("error_page") != std::string::npos)
		{
            std::string errorCodeStr = line.substr(line.find(" ") + 1);
            std::istringstream iss(errorCodeStr);
            int errorCode;
            iss >> errorCode;
        }
    }
}

int ServerConfig::getPort()
{
	return (this->_port);
}

ServerConfig::~ServerConfig()
{

}