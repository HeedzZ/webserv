#include "ServerConfig.hpp"
#include <iostream>

void ServerConfig::setPort(int serverPort)
{
    _ports.push_back(serverPort);
}

void ServerConfig::setServerName(const std::string& name)
{
    _serverName = name;
}


const std::string& ServerConfig::getServerName(void)
{ 
    return _serverName;
}

const std::vector<int>& ServerConfig::getPorts() const
{
    return _ports;
}

void ServerConfig::setRoot(const std::string& rootPath)
{
    _root = rootPath;
}

const std::string& ServerConfig::getRoot() const
{
    return _root;
}

void ServerConfig::setIndex(const std::string& indexPage)
{
    _index = indexPage;
}

const std::string& ServerConfig::getIndex() const
{
    return _index;
}

void ServerConfig::setErrorPage(int code, const std::string& path)
{
    _error_pages[code] = path;
}

std::string ServerConfig::getErrorPage(int errorCode) const
{
    std::map<int, std::string>::const_iterator it = _error_pages.find(errorCode);
    if (it != _error_pages.end())
        return it->second;
    return "";
}

void ServerConfig::addLocation(const ServerLocation& location)
{
    _locations.push_back(location);
    //_location.display();
}

const std::vector<ServerLocation>& ServerConfig::getLocations() const
{
    return _locations;
}

void ServerConfig::setHost(const std::string& host)
{
    /*if (!isValidIP(host)) {
        std::cerr << "Invalid IP address format: " << host << std::endl;
        throw std::invalid_argument("Invalid IP address format");
    }*/
    _host = host;
}

const std::string& ServerConfig::getHost() const
{
    return _host;
}

size_t ServerConfig::getClientMaxBodySize() const
{
    return _clientMaxBodySize;
}

void ServerConfig::setClientMaxBodySize(size_t size)
{
    _clientMaxBodySize = size;
}