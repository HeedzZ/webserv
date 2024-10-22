#include "HttpRequest.hpp"

/*
GET /index.html HTTP/1.1  //Request Line GET == méthode HTTP // /index.html == Path // HTTP/1.1 == Version
Host: localhost 		  //HTTP Headers Host
User-Agent: curl/7.68.0	  //HTTP Headers client 
Accept: */				  //HTTP Headers ce que le client accept 
/**/
HttpRequest::HttpRequest()
{
	std::cout << "Default constructor called" << std::endl;
}

HttpRequest HttpRequest::ParseHttpRequest(const std::string rawRequest)
{
	HttpRequest request;
	std::string	line;
	std::istringstream requestStream(rawRequest);

	std::getline(requestStream, line);
	std::istringstream lineStream(line);
	lineStream >> _method >> _path >> _httpVersion;
	std::cout << "_method = "<< _method << std::endl;
	std::cout << "_path = "<< _path << std::endl;
	std::cout << "_httpVersion = "<< _httpVersion << std::endl;

	while (std::getline(requestStream, line) && line != "\r")
	{
		std::size_t delimiter = line.find(":");
		if (delimiter != std::string::npos)
		{
            std::string headerName = line.substr(0, delimiter);
            std::string headerValue = line.substr(delimiter + 2);  // Skip ": "
            _headers[headerName] = headerValue;
			std::cout << headerName << " = " <<  _headers[headerName] << std::endl;
        }
	}
	if (request._method == "POST")
        std::getline(requestStream, _body, '\0');
	return (request);
}

std::string HttpRequest::handleRequest(const HttpRequest& request)
{
	if (request._method.compare("GET") == 0)
		return handleGet(request);
	else if (request._method.compare("POST") == 0)
		return handlePost(request);
	else if (request._method.compare("DELETE") == 0)
		return handleDelete(request);
	else
		return ("HTTP/1.1 404 Not Found\r\n\r\n");
	
}

std::string HttpRequest::handleGet(const HttpRequest& request)
{
    std::string response;
    std::ifstream file(("." + request._path).c_str());

    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        char* buffer = new char[size + 1]; // Allouer de la mémoire pour le contenu du fichier
        if (file.read(buffer, size))
        {
            buffer[size] = '\0'; // Null-terminate the string
            std::string fileContent(buffer);
            delete[] buffer; // Libérer la mémoire

            std::ostringstream oss;
            oss << fileContent.size(); // Utilisation de std::ostringstream pour la conversion
            response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Length: " + oss.str() + "\r\n"; // Remplacer std::to_string
            response += "Content-Type: text/html\r\n";
            response += "\r\n";
            response += fileContent;
        }
    }
    else
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
    return response;
}


std::string HttpRequest::handlePost(const HttpRequest& request)
{
    std::string response;

    // Traiter les données envoyées via le body
    std::ofstream outfile("./uploaded_data.txt");
    outfile << request._body;
    outfile.close();

    response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "\r\n";
    response += "Data received and saved.";
    return response;
}


std::string HttpRequest::handleDelete(const HttpRequest& request)
{
    std::string response;
    
    // Supprimer le fichier
    if (std::remove(("." + request._path).c_str()) == 0)
	{
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "\r\n";
        response += "File deleted successfully.";
    }
	else
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
    return response;
}



HttpRequest::~HttpRequest()
{
	std::cout << "Destructor called" << std::endl;
}