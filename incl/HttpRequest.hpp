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
	std::string handleRequest(const HttpRequest& request);
	std::string handleGet(const HttpRequest& request);
	std::string handlePost(const HttpRequest& request);
	std::string handleDelete(const HttpRequest& request);
};

#endif

