#include "ServerConfig.hpp"
#include <iostream>

ServerConfig::ServerConfig() : root("./www/main"), index("index.html"), _hasListen(false), _hasRoot(false)
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

std::string ServerConfig::getErrorPage(int errorCode) const {
    std::map<int, std::string>::const_iterator it = error_pages.find(errorCode);
    if (it != error_pages.end()) {
        return it->second;  // Chemin de la page d'erreur définie
    }
    return "";  // Retourne une chaîne vide si aucune page n'est définie pour ce code d'erreur
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
    std::ifstream configFile(filepath.c_str());
    if (!configFile.is_open())
    {
        std::cerr << "Could not open the file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    ServerLocation* currentLocation = NULL;
    bool inLocationBlock = false;
    try 
    {
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
                    for (std::vector<ServerLocation>::const_iterator it = locations.begin(); it != locations.end(); ++it)
                    {
                        if (it->getPath() == locationPath)
                        {
                            std::cerr << "Duplicate location path: " << locationPath << std::endl;
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
            }
            else
            {
                if (token == "}")
                {
                    if (currentLocation != NULL)
                    {
                        locations.push_back(*currentLocation);
                        delete currentLocation;
                    }
                    inLocationBlock = false;
                }
                else
                    parseLocationDirective(token, line, currentLocation, inLocationBlock);
            }
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        delete currentLocation;
        return false;
    }
    if (!_hasListen || !_hasRoot)
    {
        std::cerr << "Missing/invalid essential configuration parameters." << std::endl;
        return false;
    }
    if (currentLocation != NULL)
        delete currentLocation;
    return true;
}


bool ServerConfig::parseServerDirective(const std::string& token, std::istringstream& iss, int& hasListen, int& hasRoot)
{
    if (token == "listen")
    {
        int port;
        if (!(iss >> port) || port < 1 || port > 65535) // Étape 4: Vérifier la validité du port
        {
            std::cerr << "Invalid or missing port in configuration." << std::endl;
            return false;
        }
        setPort(port);
        hasListen++;
    }
    else if (token == "root")
    {
        std::string rootPath;
        iss >> rootPath;
        rootPath.resize(rootPath.size() - 1);
        if (rootPath.empty()) // Étape 2: Vérifier que le rootPath est défini
        {
            std::cerr << "Root path is missing in configuration." << std::endl;
            return false;
        }
        setRoot(rootPath);
        hasRoot++;
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
        path.resize(path.size() - 1); // Supprime le dernier caractère (ex : un point-virgule)
        if (path.empty())
        {
            std::cerr << "Error page path is missing or invalid." << std::endl;
            return false;
        }
        std::ifstream testFile(path.c_str());
        if (!testFile.is_open())
        {
            std::cerr << "Error page file does not exist: " << path << std::endl;
            return false;
        }
        testFile.close();
        setErrorPage(code, path);
    }
    return true;
}

//parse Locations config
void ServerConfig::parseLocationDirective(const std::string& token, const std::string& line, ServerLocation* currentLocation, bool& inLocationBlock)
{
    if (token == "server_name" || token == "listen")
    {
        std::cerr << "Directive '" << token << "' is not allowed inside a location block." << std::endl;
        inLocationBlock = false;
        throw std::runtime_error("Invalid directive in location block");
    }
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

    std::cout << "Locations:\n";
    for (std::vector<ServerLocation>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        it->display();
        std::cout << "-----------------------\n";
    }
}
