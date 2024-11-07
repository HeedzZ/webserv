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

    bool locationFound = false;
    const std::vector<ServerLocation>& locations = config.getLocations();
    for (std::vector<ServerLocation>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        if (this->_path == it->getPath())
        {
            fullPath = it->getRoot() + it->getIndex();
            locationFound = true;
            break;
        }
    }
    if (locationFound == false)
        fullPath = config.getRoot() + _path;

    if (!locationFound)
    {
        if (this->_path == "/")
            fullPath = config.getRoot() + config.getIndex();
    }

    if (fullPath.find(".php") != std::string::npos) {
        return executeCGI(fullPath, config);
    }

    std::ifstream file(fullPath.c_str(), std::ios::binary);
    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size > 0 && size < std::numeric_limits<std::streamsize>::max())
        {
            std::vector<char> buffer(static_cast<size_t>(size));
            if (file.read(buffer.data(), size))
            {
                std::string fileContent(buffer.data(), size);
                std::string contentType = getMimeType(fullPath);

                std::ostringstream oss;
                oss << fileContent.size();
                response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Length: " + oss.str() + "\r\n";
                response += "Content-Type: " + contentType + "\r\n";
                response += "\r\n";
                response += fileContent;
            }
        }
    }
    else
        return findErrorPage(config, 404);
    return response;
}


std::string HttpRequest::getMimeType(const std::string& filePath) {
    // Tableau associatif pour les types MIME les plus courants
    std::map<std::string, std::string> mimeTypes;
    mimeTypes[".php"] = "text/html";
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

std::string HttpRequest::executeCGI(const std::string& scriptPath, ServerConfig& config) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return "500 Internal Server Error: Pipe creation failed";
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Processus enfant
        close(pipefd[0]); // Ferme la lecture
        dup2(pipefd[1], STDOUT_FILENO); // Redirige la sortie standard vers le pipe
        close(pipefd[1]);

        if (_method == "POST") {
            // Rediriger l'entrée standard pour les requêtes POST
            int input_fd[2];
            if (pipe(input_fd) == -1) {
                exit(1); // Échec de la création du pipe
            }

            pid_t input_pid = fork();
            if (input_pid == 0) {
                // Processus enfant : écrire le corps de la requête
                close(input_fd[0]);
                write(input_fd[1], _body.c_str(), _body.size());
                close(input_fd[1]);
                exit(0);
            } else {
                close(input_fd[1]);
                dup2(input_fd[0], STDIN_FILENO);
                close(input_fd[0]);
            }
        }

        // Configuration des arguments et de l'environnement pour execve
        char *args[] = {
            const_cast<char*>("/usr/bin/php"),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };

        // Configuration des variables d'environnement
        std::vector<std::string> envVars;
        envVars.push_back("REQUEST_METHOD=POST");
        envVars.push_back("SCRIPT_FILENAME=" + scriptPath);
        envVars.push_back("CONTENT_LENGTH=" + intToString(_body.size()));
        envVars.push_back("CONTENT_TYPE=" + _headers["Content-Type"]);

        std::vector<char*> env;
        for (size_t i = 0; i < envVars.size(); ++i) {
            env.push_back(const_cast<char*>(envVars[i].c_str()));
        }
        env.push_back(NULL);

        execve("/usr/bin/php", args, env.data());
        std::cerr << "Erreur d'exécution du script CGI." << std::endl;
        exit(1);
    } else if (pid > 0) {
        // Processus parent
        close(pipefd[1]); // Ferme l'écriture

        std::string output;
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }

        close(pipefd[0]);
        int status;
        waitpid(pid, &status, 0);

        std::ostringstream oss;
        oss << output.size();
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Length: " + oss.str() + "\r\n";
        response += "Content-Type: text/html\r\n"; // Modifier si nécessaire
        response += "\r\n";
        response += output;
        return response;
    } else {
        return findErrorPage(config, 500);
    }
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
std::string HttpRequest::uploadTxt(ServerConfig& config, std::string response)
{
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

    response = "HTTP/1.1 201 Created\r\n";
    response += "Content-Length: 0\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "\r\n";
    return response;
}

std::string HttpRequest::uploadFile(ServerConfig& config, std::string response, std::string contentType)
{
    std::string boundary = "--" + contentType.substr(contentType.find("boundary=") + 9);
    size_t fileStartPos = _body.find("filename=\"");
    if (fileStartPos == std::string::npos)
        return findErrorPage(config, 400);

    fileStartPos += 10;
    size_t fileNameEndPos = _body.find("\"", fileStartPos);
    std::string fileName = _body.substr(fileStartPos, fileNameEndPos - fileStartPos);
    size_t contentStart = _body.find("\r\n\r\n", fileNameEndPos) + 4;
    size_t contentEnd = _body.find(boundary, contentStart) - 2;
    std::string fileContent = _body.substr(contentStart, contentEnd - contentStart);
    std::string targetPath = "upload/" + fileName;
    std::ofstream outFile(targetPath.c_str(), std::ios::binary);
    if (!outFile.is_open())
        return findErrorPage(config, 500);
    outFile.write(fileContent.c_str(), fileContent.size() - 46);
    outFile.close();

    response = "HTTP/1.1 201 Created\r\n";
    response += "Content-Length: 0\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "\r\n";
    return response;
}

std::string HttpRequest::handlePost(ServerConfig& config) {
    // Vérification du Content-Length
    std::map<std::string, std::string>::const_iterator it = this->_headers.find("Content-Length");
    if (it == this->_headers.end()) {
        return findErrorPage(config, 411);
    }

    int contentLength;
    std::istringstream lengthStream(it->second);
    lengthStream >> contentLength;

    if (this->_body.size() != static_cast<std::string::size_type>(contentLength)) {
        return findErrorPage(config, 400);
    }

    // Transmettre la requête au script CGI
    std::string scriptPath = config.getRoot() + this->_path; // Modifier si nécessaire

    if (scriptPath.find(".php") != std::string::npos) {
        // Exécuter le script PHP et renvoyer la réponse
        return executeCGI(scriptPath, config);
    }

    return findErrorPage(config, 415); // Unsupported Media Type si le type n'est pas géré
}



std::string HttpRequest::handleDelete(ServerConfig& config)
{
    std::string response;
    
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

std::string HttpRequest::findErrorPage(ServerConfig& config, int errorCode)
{
    std::string errorPagePath = config.getErrorPage(errorCode);
    std::string fullPath = config.getRoot() + errorPagePath;

    std::ifstream errorFile(fullPath.c_str());
    std::string content;
    if (errorFile)
    {
        std::string line;
        while (std::getline(errorFile, line))
            content += line + "\n";
    }
    else
        content = "<html><body><h1>Error " + intToString(errorCode) + "</h1><p>Page not found.</p></body></html>";
    std::ostringstream response;

    response << "HTTP/1.1 " << errorCode << " Error\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "\r\n";
    response << content;

    return response.str();
}


HttpRequest::~HttpRequest()
{
}