#include "HttpRequest.hpp"

std::string HttpRequest::intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

HttpRequest::HttpRequest(const std::string rawRequest)
{
    std::istringstream requestStream(rawRequest);
    std::string line;

    if (std::getline(requestStream, line))
    {
        std::istringstream lineStream(line);
        lineStream >> _method >> _path >> _httpVersion;
    }

    while (std::getline(requestStream, line) && line != "\r")
    {
        std::size_t delimiter = line.find(":");
        if (delimiter != std::string::npos)
        {
            std::string headerName = line.substr(0, delimiter);
            std::string headerValue = line.substr(delimiter + 2);
            _headers[headerName] = headerValue;
        }
    }

    if (_method == "POST")
    {
        std::map<std::string, std::string>::const_iterator it = _headers.find("Content-Length");
        if (it != _headers.end())
        {
            int contentLength = std::atoi(it->second.c_str());
            if (contentLength > 0)
            {
                std::vector<char> buffer(contentLength);
                requestStream.read(&buffer[0], contentLength);
                _body.assign(buffer.begin(), buffer.end());
            }
        }
        std::cout << "set Body = " << _body << std::endl;
    }
}

std::string HttpRequest::handleRequest(ServerConfig& config)
{
	if (this->_method.compare("GET") == 0)
		return handleGet(config);
	else if (this->_method.compare("POST") == 0)
		return handlePost(config);
	else if (this->_method.compare("DELETE") == 0)
		return handleDelete(config);
	else
		return (findErrorPage(config, 404));
	
}

std::string HttpRequest::handleGet(ServerConfig& config)
{
    std::string response;
    std::string fullPath;

    // Vérifier si le chemin de la requête correspond à une location définie
    bool locationFound = false;
    const std::vector<ServerLocation>& locations = config.getLocations();
    for (std::vector<ServerLocation>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
        if (this->_path == it->getPath()) {
            fullPath = it->getRoot() + it->getIndex();  // Utiliser le root spécifié pour cette location
            locationFound = true;
            break;
        }
    }
    if (locationFound == false)
        fullPath = config.getRoot() + _path;

    // Si aucune location correspondante n'a été trouvée, utiliser la configuration par défaut
    if (!locationFound) {
        if (this->_path == "/") {
            fullPath = config.getRoot() + config.getIndex();  // Page index par défaut
        }
    }

    // Ouvrir et lire le fichier correspondant
    std::ifstream file(fullPath.c_str(), std::ios::binary);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size > 0 && size < std::numeric_limits<std::streamsize>::max()) {
            std::vector<char> buffer(static_cast<size_t>(size));
            if (file.read(buffer.data(), size)) {
                std::string fileContent(buffer.data(), size);
                
                // Déterminer le type MIME en utilisant le chemin complet
                std::string contentType = getMimeType(fullPath);

                // Construire la réponse HTTP
                std::ostringstream oss;
                oss << fileContent.size();
                response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Length: " + oss.str() + "\r\n";
                response += "Content-Type: " + contentType + "\r\n";
                response += "\r\n";
                response += fileContent;
            }
        }
    } else {
        return findErrorPage(config, 404);  // Page d'erreur 404 si le fichier ne peut pas être ouvert
    }
    return response;
}


std::string HttpRequest::getMimeType(const std::string& filePath) {
    // Tableau associatif pour les types MIME les plus courants
    std::map<std::string, std::string> mimeTypes;
    mimeTypes[".html"] = "text/html";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".js"] = "application/javascript";
    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".svg"] = "image/svg+xml";
    mimeTypes[".ico"] = "image/x-icon";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".xml"] = "application/xml";
    mimeTypes[".txt"] = "text/plain";

    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = filePath.substr(dotPos);
        if (mimeTypes.count(extension) > 0) {
            return mimeTypes[extension];
        }
    }

    return "application/octet-stream"; // Type MIME par défaut
}


std::string extractJsonValue(const std::string& json, const std::string& key)
{
    std::string keyPattern = "\"" + key + "\":\"";
    size_t keyPos = json.find(keyPattern);
    if (keyPos == std::string::npos)
        return "";

    size_t valueStart = keyPos + keyPattern.length();
    size_t valueEnd = json.find("\"", valueStart);
    if (valueEnd == std::string::npos)
        return "";

    return json.substr(valueStart, valueEnd - valueStart);
}

std::string HttpRequest::handlePost(ServerConfig& config)
{
    std::string response;

    std::map<std::string, std::string>::const_iterator it = this->_headers.find("Content-Length");
    if (it == this->_headers.end())
        return findErrorPage(config, 411);

    int contentLength;
    std::istringstream lengthStream(it->second);
    lengthStream >> contentLength;

    if (this->_body.size() != static_cast<std::string::size_type>(contentLength))
        return findErrorPage(config, 400);

    std::string fileName = extractJsonValue(this->_body, "fileName");
    std::string fileContent = extractJsonValue(this->_body, "fileContent");

    if (fileName.empty() || fileContent.empty())
        return findErrorPage(config, 400);

    std::string targetPath = "upload/" + fileName;

    std::ofstream outFile(targetPath.c_str(), std::ios::binary);
    if (!outFile.is_open())
        return findErrorPage(config, 500);

    outFile.write(fileContent.c_str(), fileContent.size());
    outFile.close();

    std::ostringstream oss;

    response = "HTTP/1.1 201 Created\r\n";
    oss << contentLength;
    response += "Content-Length: " + oss.str() + "\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "\r\n";

    return response;
}



std::string HttpRequest::handleDelete(ServerConfig& config)
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
        return (findErrorPage(config, 404));
    return response;
}

#include <sstream>  // Pour std::ostringstream en C++98

std::string HttpRequest::findErrorPage(ServerConfig& config, int errorCode) {
    // Obtenir le chemin de la page d'erreur depuis l'objet config
    std::string errorPagePath = config.getErrorPage(errorCode);
    std::string fullPath = config.getRoot() + errorPagePath;

    std::ifstream errorFile(fullPath.c_str());
    std::string content;
    if (errorFile) {
        // Lire tout le contenu du fichier
        std::string line;
        while (std::getline(errorFile, line)) {
            content += line + "\n";
        }
    } else {
        // Si le fichier ne peut pas être ouvert, créer une page d'erreur générique
        content = "<html><body><h1>Error " + intToString(errorCode) + "</h1><p>Page not found.</p></body></html>";
    }

    // Construire le début de la réponse HTTP
    std::ostringstream response;
    response << "HTTP/1.1 " << errorCode << " Error\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "\r\n";  // Séparation entre les en-têtes et le corps de la réponse
    response << content;  // Ajouter le contenu de la page d'erreur

    return response.str();
}


HttpRequest::~HttpRequest()
{
}