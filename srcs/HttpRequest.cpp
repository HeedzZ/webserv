#include "HttpRequest.hpp"


HttpRequest::HttpRequest(const std::string rawRequest)
{
    std::string	line;
	std::istringstream requestStream(rawRequest);

	std::getline(requestStream, line);
	std::istringstream lineStream(line);
	lineStream >> _method >> _path >> _httpVersion;

	while (std::getline(requestStream, line) && line != "\r")
	{
		std::size_t delimiter = line.find(":");
		if (delimiter != std::string::npos)
		{
            std::string headerName = line.substr(0, delimiter);
            std::string headerValue = line.substr(delimiter + 2);  // Skip ": "
            _headers[headerName] = headerValue;
        }
	}
	if (_method == "POST")
        std::getline(requestStream, _body, '\0');
}

std::string HttpRequest::handleRequest(ServerConfig& config)
{
	if (this->_method.compare("GET") == 0)
		return handleGet(config);
	else if (this->_method.compare("POST") == 0)
		return handlePost();
	else if (this->_method.compare("DELETE") == 0)
		return handleDelete();
	else
		return ("HTTP/1.1 404 Not Found\r\n\r\n");
	
}



std::string HttpRequest::handleGet(ServerConfig& config)
{
    std::string response;
    std::string fullPath;

    // Check if the request path matches any defined locations
    bool locationFound = false;
    const std::vector<ServerLocation>& locations = config.getLocations();
    std::cout << locations[0].getPath() << std::endl;
    for (std::vector<ServerLocation>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
        if (this->_path == it->getPath()) {
            fullPath = it->getIndex();  // Use the root specified for this location
            locationFound = true;
            std::cout << "LOCATION FOUND full path: " << fullPath << std::endl;
            break;
        }
    }

    // Si aucune location correspondante n'a été trouvée
    if (!locationFound) {
        return "HTTP/1.1 404 Not Found\r\n\r\n";
    }

    // Ouvrir et lire le fichier correspondant
    std::ifstream file(fullPath.c_str(), std::ios::binary);
    std::cout << fullPath.c_str() << std::endl;
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        if (size > 0 && size < std::numeric_limits<std::streamsize>::max()) {
            file.seekg(0, std::ios::beg);
            std::vector<char> buffer(static_cast<size_t>(size));
            if (file.read(buffer.data(), size)) {
                std::string fileContent(buffer.data(), size);
                std::ostringstream oss;
                oss << fileContent.size();
                response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Length: " + oss.str() + "\r\n";
                response += "Content-Type: text/html\r\n";
                response += "\r\n";
                response += fileContent;
            }
        }
    } else {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
    return response;
}



std::string HttpRequest::handlePost()
{
    std::string response;

    // Traiter les données envoyées via le body
    std::ofstream outfile("./uploaded_data.txt");
    outfile << this->_body;
    outfile.close();

    response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "\r\n";
    response += "Data received and saved.";
    return response;
}


std::string HttpRequest::handleDelete()
{
    std::string response;
    
    // Supprimer le fichier
    if (std::remove(("." + this->_path).c_str()) == 0)
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

std::string HttpRequest::getPath() const {
    return _path;
}

HttpRequest::~HttpRequest()
{
}