#include "ServerConfig.hpp"
#include <iostream>

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
            parseLocationDirective(token, line, currentLocation, inLocationBlock);
        if (token == "client_max_body_size")
            handleClientMaxBodySizeDirective(iss, line);
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
        hostValue.resize(hostValue.size() - 1);
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
        setErrorPage(404, ("main/errors/404.html"));
        setErrorPage(500, ("main/errors/500.html"));
        setErrorPage(411, ("main/errors/411.html"));
        setErrorPage(400, ("main/errors/400.html"));
        setErrorPage(405, ("main/errors/403.html"));
        setErrorPage(405, ("main/errors/405.html"));
        setErrorPage(413, ("main/errors/413.html"));
        setErrorPage(415, ("main/errors/415.html"));
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

void ServerConfig::parseLocationDirective(const std::string& token, const std::string& line, ServerLocation* currentLocation, bool& inLocationBlock)
{
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
