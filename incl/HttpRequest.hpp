#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string.h>
#include <vector>
#include <limits>
#include "ServerConfig.hpp"
#include <sys/stat.h>
#include "ServerLocation.hpp"

class HttpRequest
{
private:
	std::string _method;
    std::string _path;
    std::string _httpVersion;
    std::string _body;
    std::map<std::string, std::string> _headers;
	
public:
	HttpRequest(const std::string rawRequest);
	~HttpRequest();
	std::string handleRequest(ServerConfig& config);
	std::string handleGet(ServerConfig& config);
	std::string	handlePost(ServerConfig& config);
	std::string uploadTxt(ServerConfig& config, std::string response);
	std::string uploadFile(ServerConfig& config, std::string response, std::string contentType);
	std::string handleDelete(ServerConfig& config);
	std::string findErrorPage(ServerConfig& config, int errorCode);
	std::string getMimeType(const std::string& filePath);
	std::string getPath() const;
	std::string getMethod() const;
	std::string intToString(int value);
};

#endif

