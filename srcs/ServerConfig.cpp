#include "ServerConfig.hpp"
#include <iostream>

ServerConfig::ServerConfig() : root("var/www/main"), index("index.html"), _host("127.0.0.1"), _hasListen(false), _hasRoot(false), _valid(true)
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

ServerConfig* ServerConfig::parseServerBlock(std::ifstream& filepath)
{
    std::string line;
    ServerLocation* currentLocation = NULL;
    bool inLocationBlock = false;
    bool inServerBlock = false;
    int blockDepth = 0;

    while (std::getline(filepath, line))
    {
        line = line.substr(line.find_first_not_of(" \t"));
        if (line.empty() || line[0] == '#')
            continue;

        std::string token;
        std::istringstream iss(line);
        iss >> token;

        if (!inServerBlock)
        {
            if (token == "server")
            {
                size_t bracePos = line.find('{');
                if (bracePos != std::string::npos)
                {
                    inServerBlock = true;
                    blockDepth = 1;
                    continue;
                }
                else
                    throw std::runtime_error("Expected '{' after 'server'. Line: " + line);
            }
            throw std::runtime_error("Expected 'server {' to start a server block. Line: " + line);
        }

        if (token == "}")
        {
            blockDepth--;
            if (blockDepth == 0)
            {
                if (!_hasListen || !_hasRoot)
                    throw std::runtime_error("Missing essential configuration parameters (listen/root).");
                return this;
            }

            if (inLocationBlock && blockDepth == 1)
            {
                inLocationBlock = false;
                if (currentLocation)
                {
                    locations.push_back(*currentLocation);
                    delete currentLocation;
                    currentLocation = NULL;
                }
                continue;
            }
            continue;
        }

        if (!inLocationBlock)
        {
            if (token == "location" && line.find('{') != std::string::npos)
            {
                inLocationBlock = true;
                blockDepth++;
                std::string locationPath;
                iss >> locationPath;
                currentLocation = new ServerLocation(locationPath);
                continue;
            }

            if (!processServerOrLocation(token, iss, line, currentLocation, inLocationBlock))
                throw std::runtime_error("Invalid directive in server block: " + line);
        }
        else
        {
            parseLocationDirective(token, line, currentLocation, inLocationBlock);
        }
        if (token == "client_max_body_size")
        {
            size_t size;
            iss >> size;
            if (size <= 0)
                throw std::runtime_error("Invalid value for client_max_body_size: " + line);
            setClientMaxBodySize(size);
        }
    }

    throw std::runtime_error("Unexpected end of file. Missing '}' for server block.");
}

bool ServerConfig::processServerOrLocation(const std::string& token, std::istringstream& iss, const std::string& line, ServerLocation*& currentLocation, bool& inLocationBlock)
{
    if (token == "location")
    {
        std::string locationPath = extractLocationPath(line);
        for (std::vector<ServerLocation>::const_iterator it = locations.begin(); it != locations.end(); ++it)
        {
            if (it->getPath() == locationPath)
            {
                throw std::runtime_error("Duplicate location path");
                return false;
            }
        }
        currentLocation = new ServerLocation(locationPath);
        inLocationBlock = true;
        parseLocationDirective(token, line, currentLocation, inLocationBlock);
    }
    else
    {
        if (!parseServerDirective(token, iss, _hasListen, _hasRoot))
            return false;
    }
    return true;
}

bool ServerConfig::parseServerDirective(const std::string& token, std::istringstream& iss, int& hasListen, int& hasRoot)
{
    if (token == "listen")
    {
        int port;
        if (!(iss >> port) || port < 1 || port > 65535)
        {
            throw std::runtime_error("Invalid or missing port in configuration.");
            return false;
        }
        setPort(port);
        hasListen++;
    }
    else if (token == "root")
    {
        return parseRootDirective(iss, hasRoot);
    }
    else if (token == "index")
    {
        std::string indexPage;
        iss >> indexPage;
        indexPage.resize(indexPage.size() - 1);
        setIndex(indexPage);
    }
    else if (token == "error_page")
        return parseErrorPageDirective(iss);
    else if (token == "server_name")
    {
        std::string name;
        iss >> name;
        name.resize(name.size() - 1);
        setServerName(name);
    }
    else if (token == "host")
    {
        std::string hostValue;
        iss >> hostValue;
        hostValue.resize(hostValue.size() - 1); // Suppression de caractères de fin comme ';'
        if (hostValue.empty()) {
            throw std::runtime_error("Host value is missing in configuration.");
            return false;
        }
        if (!isValidIP(hostValue))
        {
            throw std::runtime_error("Host value is missing in configuration.");
            return false;
        }
        _host = hostValue;
    }
    if (error_pages.empty())
    {
        setErrorPage(404, ("/main/errors/404.html"));
        setErrorPage(500, ("/main/errors/500.html"));
        setErrorPage(411, ("/main/errors/411.html"));
        setErrorPage(400, ("/main/errors/400.html"));
        setErrorPage(405, ("/main/errors/405.html"));
    }
    return true;
}

bool ServerConfig::parseRootDirective(std::istringstream& iss, int& hasRoot)
{
    std::string rootPath;
    iss >> rootPath;
    rootPath.resize(rootPath.size() - 1);
    if (rootPath.empty())
    {
        throw std::runtime_error("Root path is missing in configuration.");
        return false;
    }
    setRoot(rootPath);
    hasRoot++;
    return true;
}

bool ServerConfig::parseErrorPageDirective(std::istringstream& iss)
{
    int code;
    std::string path;

    iss >> code >> path;

    if (!path.empty() && path[path.size() - 1] == ';')
        path.resize(path.size() - 1);

    if (path.empty())
    {
        throw std::runtime_error("Error page path is missing or invalid.");
        return false;
    }
    std::string fullPath = root + path;
    std::ifstream testFile(fullPath.c_str());
    if (!testFile.is_open())
    {
        throw std::runtime_error("Error page file does not exist: " + path);
        return false;
    }

    testFile.close();

    setErrorPage(code, path);
    return true;
}

void ServerConfig::parseLocationDirective(const std::string& token, const std::string& line, ServerLocation* currentLocation, bool& inLocationBlock) {
    if (token == "limit_except")
    {
        std::string methods = line.substr(line.find("limit_except") + 12);
        currentLocation->setAllowedMethods(methods);
    }
    else if (token == "root")
    {
        std::string rootValue = line.substr(line.find(token) + token.length() + 1);
        rootValue.resize(rootValue.size() - 2);
        currentLocation->setRoot(rootValue);

        std::ifstream testFile(rootValue.c_str());
        if (!testFile.is_open())
            throw std::runtime_error("The specified root directory does not exist: " + rootValue);
    }
    else if (token == "index")
        parseIndexDirective(line, currentLocation);
    else if (token == "}")
        inLocationBlock = false;
}


void ServerConfig::parseIndexDirective(const std::string& line, ServerLocation* currentLocation)
{
    std::string indexValue = line.substr(line.find("index") + 6);
    indexValue.resize(indexValue.size() - 2);

    currentLocation->setIndex(indexValue);

    std::string fullPath = currentLocation->getRoot() + indexValue;

    std::ifstream testFile(fullPath.c_str());
    if (!testFile.is_open())
        throw std::runtime_error("The specified index file does not exist " + fullPath);
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