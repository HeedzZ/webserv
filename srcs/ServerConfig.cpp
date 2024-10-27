#include "ServerConfig.hpp"
#include <iostream>

ServerConfig::ServerConfig() : port(80), root("./www/main"), index("index.html")
{}

void ServerConfig::setPort(int serverPort)
{
    port = serverPort;
}

int ServerConfig::getPort() const
{
    return port;
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

bool ServerConfig::parseConfigFile(const std::string& filepath)
{
    std::ifstream configFile(filepath.c_str()); // Utilisez c_str() ici
    if (!configFile.is_open())
    {
        std::cerr << "Could not open the file: " << filepath << std::endl;
        return false;
    }
    
    std::string line;
    ServerLocation* currentLocation = NULL;

    while (std::getline(configFile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "server" && line.find('{') != std::string::npos)
            continue;
        else if (token == "listen")
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
        if (token == "error_page")
        {
            int code;
            std::string path;
            iss >> code >> path;
            path.resize(path.size() - 1);
            setErrorPage(code, path);
        }
        else if (token == "location")
        {
            std::string locationPath = extractLocationPath(line);
            currentLocation = new ServerLocation(locationPath);
            locations.push_back(*currentLocation);
        }
        else if (currentLocation)
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
        }
    }
    return true;
}



void ServerConfig::display() const
{
    std::cout << "Server Configuration:\n";
    std::cout << "Port: " << port << std::endl;
    std::cout << "Root: " << root << std::endl;
    std::cout << "Index: " << index << std::endl;
     std::cout << "Error Pages:\n";
    for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it)
        std::cout << "  Error Code " << it->first << ": " << it->second << "\n";
}
