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

class HttpRequest
{
private:
	std::string _method;
    std::string _path;
    std::string _httpVersion;
    std::string _body;
    std::map<std::string, std::string> _headers;
	
public:
	HttpRequest();
	~HttpRequest();
	HttpRequest parseHttpRequest(const std::string rawRequest);
	std::string handleRequest();
	std::string handleGet();
	std::string handlePost();
	std::string handleDelete();
	std::string getPath() const;
};

#endif

