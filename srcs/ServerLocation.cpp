#include "ServerLocation.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

ServerLocation::ServerLocation(const std::string& path) : path(path), allowGet(true), allowPost(true), allowDelete(true)
{}

bool ServerLocation::isGetAllowed() const
{
    return allowGet;
}

bool ServerLocation::isPostAllowed() const
{
    return allowPost;
}

bool ServerLocation::isDeleteAllowed() const
{
    return allowDelete;
}

const std::string& ServerLocation::getPath() const
{
    return path;
}

void ServerLocation::setRoot(const std::string& rootPath)
{
    this->root = rootPath;
}

const std::string& ServerLocation::getRoot() const
{
    return root;
}

void ServerLocation::setIndex(const std::string& indexPage)
{
    this->index = indexPage;
}

const std::string& ServerLocation::getIndex() const
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

void ServerLocation::setAllowedMethods(const std::string& methodsLine)
{
    allowGet = false;
    allowPost = false;
    allowDelete = false;

    std::istringstream iss(methodsLine);
    std::string method;

    // Active uniquement les méthodes spécifiées
    while (iss >> method) {
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        if (method == "GET")
            allowGet = true;
        else if (method == "POST")
            allowPost = true;
        else if (method == "DELETE")
            allowDelete = true;
    }
}

void ServerLocation::display() const
{
    std::cout << "Location Path: " << path << std::endl;
    std::cout << "CGI Extensions:\n";
    
    for (std::map<std::string, std::string>::const_iterator it = cgi_extensions.begin(); it != cgi_extensions.end(); ++it)
        std::cout << "  Extension: " << it->first << " -> Program: " << it->second << std::endl;

    std::cout << "root : " << root << std::endl;

    std::cout << "index : " << index << std::endl;

    std::cout << "Allowed Methods:\n";
    std::cout << "  GET: " << (allowGet ? "Yes" : "No") << std::endl;
    std::cout << "  POST: " << (allowPost ? "Yes" : "No") << std::endl;
    std::cout << "  DELETE: " << (allowDelete ? "Yes" : "No") << std::endl;

    std::cout << "-----------------------\n";
    std::cout << "-----------------------\n";
}