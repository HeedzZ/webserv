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
    std::vector<int>               ports;                   // Server listening port
    std::string                    root;                         // Server root directory
    std::string                    index;                        // Default index file
    std::map<int, std::string>     error_pages;   // Error pages (404, 403, etc.)
    std::vector<ServerLocation>    locations;    // List of location blocks
    std::string                    _serverName;
    std::string                    _host;
    int                            _hasListen;
    int                            _hasRoot;
    int                            _valid;
    size_t                         clientMaxBodySize;
public:
    // Default constructor
    ServerConfig();

    // Getters and Setters
    void setPort(int serverPort);
    const std::vector<int>& getPorts() const;

    size_t getClientMaxBodySize() const;
    void setClientMaxBodySize(size_t size);

    void setRoot(const std::string& rootPath);
    const std::string& getRoot() const;

    void setIndex(const std::string& indexPage);
    const std::string& getIndex() const;

    void setErrorPage(int code, const std::string& path);
    std::string getErrorPage(int errorCode) const;

    void setServerName(const std::string& name);
    const std::string& getServerName(void);

    void setHost(const std::string& host);
    const std::string& getHost() const;

    void addLocation(const ServerLocation& location);
    const std::vector<ServerLocation>& getLocations() const;

	int	getValid() const;
    std::string toString() const;

	ServerConfig* parseServerBlock(std::ifstream& filepath);
    void parseLocationDirective(const std::string& token, const std::string& line, ServerLocation* currentLocation, bool& inLocationBlock);
    bool parseServerDirective(const std::string& token, std::istringstream& iss, int& hasListen, int& hasRoot);
    bool processServerOrLocation(const std::string& token, std::istringstream& iss, const std::string& line, ServerLocation*& currentLocation, bool& inLocationBlock);
    bool parseErrorPageDirective(std::istringstream& iss);
    bool parseRootDirective(std::istringstream& iss, int& hasRoot);
    void parseIndexDirective(const std::string& line, ServerLocation* currentLocation);

    std::string extractLocationPath(const std::string& line);
    bool isValidIP(const std::string& ip) const;
    void handleClientMaxBodySizeDirective(std::istringstream& iss, const std::string& line);
    void display() const;
};

#endif
