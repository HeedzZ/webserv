#ifndef SERVERLOCATION_HPP
#define SERVERLOCATION_HPP

#include <string>
#include <map>

class ServerLocation {
private:
    std::string path;                      // Path for location, e.g., "/toukoum"
    std::string root;                      // Root directory for the location
    std::string index;                     // Index file
    std::map<std::string, std::string> cgi_extensions; // CGI extensions and programs

public:
    // Constructor
    ServerLocation(const std::string& path);

    // Getters and Setters
    const std::string& getPath() const;
    void setRootLocation(const std::string& rootPath);
    const std::string& getRootLocation() const;
    
    void setIndexLocation(const std::string& indexPage);
    const std::string& getIndexLocation() const;

    void addCgiExtension(const std::string& extension, const std::string& program);
    const std::map<std::string, std::string>& getCgiExtensions() const;
    
    // Method to display the location's configuration (for debugging)
    void display() const;
};

#endif
