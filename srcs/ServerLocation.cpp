#include "ServerLocation.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

ServerLocation::ServerLocation(const std::string& path) : path(path), _getAllowed(true), _postAllowed(true), _deleteAllowed(true)
{}

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

void ServerLocation::disableAllMethods()
{
    _getAllowed = false;
    _postAllowed = false;
    _deleteAllowed = false;
}

void ServerLocation::allowGet()
{
    _getAllowed = true;
}

void ServerLocation::allowPost()
{
    _postAllowed = true;
}

void ServerLocation::allowDelete()
{
    _deleteAllowed = true;
}

bool ServerLocation::isGetAllowed() const
{
    return _getAllowed;
}

bool ServerLocation::isPostAllowed() const
{
    return _postAllowed;
}

bool ServerLocation::isDeleteAllowed() const
{
    return _deleteAllowed;
}

/*void ServerLocation::setAllowedMethods(const std::string& methodsLine)
{
    allowGet = false;
    allowPost = false;
    allowDelete = false;

    std::istringstream iss(methodsLine);
    std::string method;

    while (iss >> method)
    {
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        if (method == "GET")
            allowGet = true;
        else if (method == "POST")
            allowPost = true;
        else if (method == "DELETE")
            allowDelete = true;
    }
}*/

void ServerLocation::display() const
{
    std::cout << "Location Path: " << path << std::endl;
    std::cout << "CGI Extensions:\n";
    
    for (std::map<std::string, std::string>::const_iterator it = cgi_extensions.begin(); it != cgi_extensions.end(); ++it)
        std::cout << "  Extension: " << it->first << " -> Program: " << it->second << std::endl;

    std::cout << "root : " << root << std::endl;

    std::cout << "index : " << index << std::endl;

    std::cout << "Allowed Methods:\n";
    std::cout << "  GET: " << (_getAllowed ? "Yes" : "No") << std::endl;
    std::cout << "  POST: " << (_postAllowed ? "Yes" : "No") << std::endl;
    std::cout << "  DELETE: " << (_deleteAllowed ? "Yes" : "No") << std::endl;

    std::cout << "-----------------------\n";
    std::cout << "-----------------------\n";
}