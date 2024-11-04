#include "ServerConfig.hpp"
#include <iostream>

ServerConfig::ServerConfig() : root("./www/main"), index("index.html")
{}

void ServerConfig::setPort(int serverPort)
{
    ports.push_back(serverPort);
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

const std::map<int, std::string>& ServerConfig::getErrorPages() const
{
    return error_pages;
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


std::string ServerConfig::extractLocationPath(const std::string& line)
{
    std::istringstream iss(line);
    std::string keyword, path;
    iss >> keyword >> path;
    return path;
}

// start parsing
bool ServerConfig::parseConfigFile(const std::string& filepath)
{
    std::ifstream configFile(filepath.c_str());
    if (!configFile.is_open())
    {
        std::cerr << "Could not open the file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    ServerLocation* currentLocation = NULL;
    bool inLocationBlock = false;

    while (std::getline(configFile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::string token;
        std::istringstream iss(line);
        iss >> token;

        if (token == "server" && line.find('{') != std::string::npos)
            continue;

        if (!inLocationBlock)
        {
            if (token == "location")
            {
                std::string locationPath = extractLocationPath(line);
                currentLocation = new ServerLocation(locationPath);
                inLocationBlock = true;
                parseLocationDirective(token, line, currentLocation, inLocationBlock);
            }
            else
                parseServerDirective(token, iss);
        }
        else
        {
            if (token == "}")
                locations.push_back(*currentLocation);
            parseLocationDirective(token, line, currentLocation, inLocationBlock);
        }
    }

    return true;
}

// parse server config
void ServerConfig::parseServerDirective(const std::string& token, std::istringstream& iss)
{
    if (token == "listen")
    {
        int port;
        iss >> port;
        setPort(port);
    }
    else if (token == "root")
    {
        std::string rootPath;
        iss >> rootPath;
        rootPath.resize(rootPath.size() - 1);
        setRoot(rootPath);
    }
    else if (token == "index")
    {
        std::string indexPage;
        iss >> indexPage;
        indexPage.resize(indexPage.size() - 1);
        setIndex(indexPage);
    }
    else if (token == "error_page")
    {
        int code;
        std::string path;
        iss >> code >> path;
        path.resize(path.size() - 1);
        setErrorPage(code, path);
    }
}

//parse Locations config
void ServerConfig::parseLocationDirective(const std::string& token, const std::string& line, ServerLocation* currentLocation, bool& inLocationBlock)
{
    if (token == "root")
    {
        std::string rootValue = line.substr(line.find(token) + token.length() + 1);
        rootValue.resize(rootValue.size() - 2);
        currentLocation->setRoot(rootValue);
    }
    else if (token == "index")
    {
        std::string indexValue = line.substr(line.find(token) + token.length() + 1);
        indexValue.resize(indexValue.size() - 2);
        currentLocation->setIndex(indexValue);
    }
    else if (token == "}")
        inLocationBlock = false;
}

void ServerConfig::display() const
{
    std::cout << "Server Configuration:\n";

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

    std::cout << "Error Pages:\n";
    for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it)
    {
        std::cout << "  Error Code " << it->first << ": " << it->second << "\n";
    }

    // Afficher les configurations de chaque `ServerLocation`
    std::cout << "Locations:\n";
    for (std::vector<ServerLocation>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        it->display();
        std::cout << "-----------------------\n";
    }
}
