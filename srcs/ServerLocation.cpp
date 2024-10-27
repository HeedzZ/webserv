#include "ServerLocation.hpp"
#include <iostream>

ServerLocation::ServerLocation(const std::string& path) : path(path)
{}

const std::string& ServerLocation::getPath() const
{
    return path;
}

void ServerLocation::setRootLocation(const std::string& rootPath)
{
    root = rootPath;
}

const std::string& ServerLocation::getRootLocation() const
{
    return root;
}

void ServerLocation::setIndexLocation(const std::string& indexPage)
{
    index = indexPage;
    this->display();
}

const std::string& ServerLocation::getIndexLocation() const
{
    return index;
}

void ServerLocation::addCgiExtension(const std::string& ext, const std::string& prog)
{
    cgi_extensions[ext] = prog;
}

const std::map<std::string, std::string>& ServerLocation::getCgiExtensions() const
{
    return cgi_extensions;
}

void ServerLocation::display() const
{
    std::cout << "Location Path: " << path << std::endl;
    std::cout << "CGI Extensions:\n";
    
    for (std::map<std::string, std::string>::const_iterator it = cgi_extensions.begin(); it != cgi_extensions.end(); ++it)
        std::cout << "  Extension: " << it->first << " -> Program: " << it->second << std::endl;

    std::cout << "root : " << root << std::endl;

    std::cout << "index : " << index << std::endl;

    std::cout << "-----------------------\n";
}