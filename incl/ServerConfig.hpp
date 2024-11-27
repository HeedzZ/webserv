#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "ServerLocation.hpp"
#include <fstream>
#include <sstream>

class ServerConfig {
private:
    std::vector<int>                ports;                   // Server listening port
    std::string                     root;                         // Server root directory
    std::string                     index;                        // Default index file
    std::map<int, std::string>      error_pages;   // Error pages (404, 403, etc.)
    std::vector<ServerLocation>     locations;    // List of location blocks
    std::string                     _serverName;
    int                            _hasListen;
    int                            _hasRoot;

public:
    // Default constructor
    ServerConfig();

    // Getters and Setters
    void setPort(int serverPort);
    const std::vector<int>& getPorts() const;

    void setRoot(const std::string& rootPath);
    const std::string& getRoot() const;

    void setIndex(const std::string& indexPage);
    const std::string& getIndex() const;

    void setErrorPage(int code, const std::string& path);
    std::string getErrorPage(int errorCode) const;

    void setServerName(const std::string& name);
    const std::string& getServerName(void);


    void addLocation(const ServerLocation& location);
    const std::vector<ServerLocation>& getLocations() const;

    void display() const;
	bool parseConfigFile(const std::string& filepath);
    void parseLocationDirective(const std::string& token, const std::string& line, ServerLocation* currentLocation, bool& inLocationBlock);
    bool parseServerDirective(const std::string& token, std::istringstream& iss, int& hasListen, int& hasRoot);
    bool processServerOrLocation(const std::string& token, std::istringstream& iss, const std::string& line, ServerLocation*& currentLocation, bool& inLocationBlock);
    bool parseErrorPageDirective(std::istringstream& iss);
    bool parseRootDirective(std::istringstream& iss, int& hasRoot);
    void parseIndexDirective(const std::string& line, ServerLocation* currentLocation);

    std::string extractLocationPath(const std::string& line);
};

#endif
