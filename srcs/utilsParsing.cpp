#include "ServerConfig.hpp"
#include <iostream>

ServerConfig::ServerConfig() : root("var/www/main"), index("index.html"), _host("127.0.0.1"), _hasListen(false), _hasRoot(false), _valid(true), clientMaxBodySize(100000000), _insideLimitExcept(false)
{}

void ServerConfig::setPort(int serverPort)
{
    ports.push_back(serverPort);
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
    return ports;
}

void ServerConfig::setRoot(const std::string& rootPath)
{
    root = rootPath;
}

const std::string& ServerConfig::getRoot() const
{
    return root;
}

void ServerConfig::setIndex(const std::string& indexPage)
{
    index = indexPage;
}

const std::string& ServerConfig::getIndex() const
{
    return index;
}

void ServerConfig::setErrorPage(int code, const std::string& path)
{
    error_pages[code] = path;
}

std::string ServerConfig::getErrorPage(int errorCode) const
{
    std::map<int, std::string>::const_iterator it = error_pages.find(errorCode);
    if (it != error_pages.end())
        return it->second;
    return "";
}

void ServerConfig::addLocation(const ServerLocation& location)
{
    locations.push_back(location);
    location.display();
}

const std::vector<ServerLocation>& ServerConfig::getLocations() const
{
    return locations;
}

void ServerConfig::setHost(const std::string& host)
{
    if (!isValidIP(host)) {
        std::cerr << "Invalid IP address format: " << host << std::endl;
        throw std::invalid_argument("Invalid IP address format");
    }
    _host = host;
}

const std::string& ServerConfig::getHost() const
{
    return _host;
}

size_t ServerConfig::getClientMaxBodySize() const
{
    return clientMaxBodySize;
}

void ServerConfig::setClientMaxBodySize(size_t size)
{
    clientMaxBodySize = size;
}


std::string ServerConfig::extractLocationPath(const std::string& line)
{
    std::istringstream iss(line);
    std::string keyword, path;
    iss >> keyword >> path;
    return path;
}

int	ServerConfig::getValid() const
{
    return _valid;
}

std::string ServerConfig::toString() const
{
    std::ostringstream oss;
        oss << "Root: " << root << "\n"
        << "Index: " << index << "\n"
        << "Server Name: " << _serverName << "\n";


    return oss.str();
}

bool ServerConfig::isValidIP(const std::string& ip) const
{
    int segments = 0;  
    int value = 0;     
    int charCount = 0;

    for (size_t i = 0; i < ip.size(); ++i)
    {
        char c = ip[i];

        if (c == '.')
        {
            if (charCount == 0 || value > 255)
                return false;
            segments++;
            value = 0;
            charCount = 0;
        }
        else if (c >= '0' && c <= '9')
        {
            value = value * 10 + (c - '0');
            charCount++;
            if (charCount > 3 || value > 255)
                return false;
        }
        else
            return false;
    }
    if (segments != 3 || charCount == 0 || value > 255)
        return false;
    return true;
}

void ServerConfig::handleClientMaxBodySizeDirective(std::istringstream& iss, const std::string& line)
{
    size_t size;
    iss >> size;
    if (size <= 0)
        throw std::runtime_error("Invalid value for client_max_body_size: " + line);
    setClientMaxBodySize(size);
}

void ServerConfig::display() const
{
    std::cout << "Server Configuration:\n";
    std::cout << "Server Name: " << _serverName << std::endl;

    std::cout << "Ports: ";
    for (size_t i = 0; i < ports.size(); ++i)
    {
        std::cout << ports[i];
        if (i < ports.size() - 1)
            std::cout << ", ";
    }
    std::cout << std::endl;

    std::cout << "Root: " << root << std::endl;
    std::cout << "Index: " << index << std::endl;
    std::cout << "Host: " << _host << std::endl;

    std::cout << "Error Pages:\n";
    for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it)
    {
        std::cout << "  Error Code " << it->first << ": " << it->second << "\n";
    }

    std::cout << "Locations:\n";
    for (std::vector<ServerLocation>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        it->display();
        std::cout << "-----------------------\n";
    }
}