#include "ServerConfig.hpp"
#include <iostream>

ServerConfig::ServerConfig() : root("./www/main"), index("index.html"), _host("127.0.0.1"), _hasListen(false), _hasRoot(false), _valid(true)
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

bool ServerConfig::parseLine(const std::string& line, int& serverIndex) {
        // Vérifier si la ligne démarre un nouveau bloc "server"
    if (line.find("server {") != std::string::npos) {
        serverIndex++; // Incrémenter l'index du serveur
        return true;   // Indiquer qu'un nouveau serveur a été trouvé
    }

    // Vérifier si la ligne est une accolade fermante "}"
    if (line.find("}") != std::string::npos) {
        return true;   // Indiquer la fin d'un bloc de serveur
    }

    // Ignorer toutes les autres lignes
    return true;
}

bool ServerConfig::isValidIP(const std::string& ip) const
{
    int segments = 0;  // Nombre de segments dans l'adresse
    int value = 0;     // Valeur numérique de chaque segment
    int charCount = 0; // Nombre de caractères dans un segment

    for (size_t i = 0; i < ip.size(); ++i)
    {
        char c = ip[i];

        if (c == '.')
        {
            if (charCount == 0 || value > 255)
                return false; // Segment vide ou valeur trop grande
            segments++;
            value = 0;
            charCount = 0;
        }
        else if (c >= '0' && c <= '9')
        {
            value = value * 10 + (c - '0');
            charCount++;
            if (charCount > 3 || value > 255)
                return false; // Plus de 3 chiffres ou valeur trop grande
        }
        else
            return false; // Caractère non valide
    }

    // Vérifie le dernier segment
    if (segments != 3 || charCount == 0 || value > 255) {
        return false;
    }

    return true;
}

ServerConfig* ServerConfig::parseServerBlock(std::ifstream& filepath)
{
    std::string line;
    ServerLocation* currentLocation = NULL;
    bool inLocationBlock = false;

    while (std::getline(filepath, line))
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
            if (processServerOrLocation(token, iss, line, currentLocation, inLocationBlock) == false)
            {
                throw std::runtime_error("Invalid directive in location block");
                return NULL; // Retourne NULL en cas d'erreur
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
                else
                {
                    if (!_hasListen || !_hasRoot)
                        throw std::runtime_error("Missing/invalid essential configuration parameters.");
                    return this; // Retourne un pointeur vers l'instance actuelle
                }
            }
            else
            {
                parseLocationDirective(token, line, currentLocation, inLocationBlock);
            }
        }
    }
    return NULL; // Retourne un pointeur vers l'instance actuelle
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
    path.resize(path.size() - 1);
    if (path.empty())
    {
        throw std::runtime_error("Error page path is missing or invalid.");
        return false;
    }
    std::ifstream testFile(path.c_str());
    if (!testFile.is_open())
    {
        throw std::runtime_error("Error page file does not exist");
        return false;
    }
    testFile.close();
    setErrorPage(code, path);
    return true;
}

void ServerConfig::parseLocationDirective(const std::string& token, const std::string& line, ServerLocation* currentLocation, bool& inLocationBlock)
{
    if (token == "server_name" || token == "listen")
    {
        std::cerr << "Directive '" << token << "' is not allowed inside a location block." << std::endl;
        throw std::runtime_error("Invalide Directive");
        inLocationBlock = false;
    }
    if (token == "root")
    {
        std::string rootValue = line.substr(line.find(token) + token.length() + 1);
        rootValue.resize(rootValue.size() - 2);
        currentLocation->setRoot(rootValue);

        std::ifstream testFile(rootValue.c_str());
        if (!testFile.is_open())
            throw std::runtime_error("The specified root directory does not exist");
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

    std::string fullPath = "html/" + indexValue;

    std::ifstream testFile(fullPath.c_str());
    if (!testFile.is_open())
        throw std::runtime_error("The specified index file does not exist");
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



