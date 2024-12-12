#ifndef SERVERLOCATION_HPP
#define SERVERLOCATION_HPP

#include <string>
#include <map>

class ServerLocation {
private:
    std::string path;
    std::string root;
    std::string index;
    std::map<std::string, std::string> cgi_extensions;
    bool allowGet;
    bool allowPost;
    bool allowDelete;

public:
    // Constructor
    ServerLocation(const std::string& path);

    // Getters and Setters
    const std::string& getPath() const;
    void setRoot(const std::string& rootPath);
    const std::string& getRoot() const;
    
    void setIndex(const std::string& indexPage);
    const std::string& getIndex() const;

    void addCgiExtension(const std::string& extension, const std::string& program);
    const std::map<std::string, std::string>& getCgiExtensions() const;
    
    bool isGetAllowed() const;
    bool isPostAllowed() const;
    bool isDeleteAllowed() const;

    void setAllowedMethods(const std::string& methodsLine);

    void display() const;
};

#endif
