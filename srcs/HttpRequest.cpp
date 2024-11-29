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

std::string HttpRequest::resolveFilePath(const ServerConfig& config)
{
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

    if (!locationFound)
    {
        if (this->_path == "/")
            fullPath = config.getRoot() + config.getIndex();
        else
        {
            fullPath = _path;
            fullPath = config.getRoot() + fullPath.substr(1);
        } 
    }
    return fullPath;
}

bool HttpRequest::isFileAccessible(const std::string& filePath)
{
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0)
        return false;
    if (access(filePath.c_str(), R_OK) != 0)
        return false;
    return true;
}

std::string HttpRequest::readFile(const std::string& filePath)
{
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > 0 && size < std::numeric_limits<std::streamsize>::max())
    {
        std::vector<char> buffer(static_cast<size_t>(size));
        if (file.read(buffer.data(), size))
            return std::string(buffer.data(), size);
    }
    return "";
}

std::string HttpRequest::handleGet(ServerConfig& config)
{
    std::string fullPath = resolveFilePath(config);
    if (!isFileAccessible(fullPath))
        return findErrorPage(config, 404);

    if (fullPath.find(".py") != std::string::npos && fullPath.find("uploads/") == std::string::npos)
        return executeCGI(fullPath, config);

    std::string fileContent = readFile(fullPath);
    if (fileContent.empty())
        return findErrorPage(config, 500);

    std::ostringstream oss;
    oss << fileContent.size();

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + oss.str() + "\r\n";
    response += "Content-Type: " + getMimeType(fullPath) + "\r\n";
    response += "\r\n";
    response += fileContent;

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
    mimeTypes[".py"] = "text/html";

    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = filePath.substr(dotPos);
        if (mimeTypes.count(extension) > 0) {
            return mimeTypes[extension];
        }
    }

    return "application/octet-stream";
}

void HttpRequest::createPipes(int outputPipe[2], int inputPipe[2])
{
    if (pipe(outputPipe) == -1 || pipe(inputPipe) == -1)
        throw std::runtime_error("Échec de la création des pipes");
}

void HttpRequest::setupChildProcess(int outputPipe[2], int inputPipe[2], const std::string& scriptPath)
{
    close(outputPipe[0]);
    if (dup2(outputPipe[1], STDOUT_FILENO) == -1)
    {
        perror("Erreur de redirection de la sortie standard");
        exit(1);
    }
    close(outputPipe[1]);

    close(inputPipe[1]);
    if (dup2(inputPipe[0], STDIN_FILENO) == -1)
    {
        perror("Erreur de redirection de l'entrée standard");
        exit(1);
    }
    close(inputPipe[0]);

    setupCGIEnvironment(scriptPath);

    char* args[] = {(char*)"/usr/bin/python3", (char*)scriptPath.c_str(), NULL};
    execve("/usr/bin/python3", args, environ);

    perror("Erreur d'exécution du script CGI");
    exit(1);
}

void HttpRequest::setupCGIEnvironment(const std::string& scriptPath)
{
    setenv("REQUEST_METHOD", _method.c_str(), 1);
    setenv("SCRIPT_FILENAME", scriptPath.c_str(), 1);
    setenv("CONTENT_LENGTH", intToString(_body.size()).c_str(), 1);
    setenv("CONTENT_TYPE", _headers["Content-Type"].c_str(), 1);
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("REDIRECT_STATUS", "200", 1);
}

std::string HttpRequest::handleParentProcess(int outputPipe[2], int inputPipe[2], pid_t pid)
{
    close(outputPipe[1]);
    close(inputPipe[0]);

    ssize_t bytesWritten = write(inputPipe[1], _body.c_str(), _body.size());
    close(inputPipe[1]);
    if (bytesWritten == -1)
        throw std::runtime_error("Erreur lors de l'écriture dans le pipe d'entrée");

    std::string output;
    char buffer[1024];
    ssize_t bytesRead;
    while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    close(outputPipe[0]);

    int status;
    if (waitpid(pid, &status, 0) == -1)
        throw std::runtime_error("Erreur lors de l'attente du processus CGI");
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        throw std::runtime_error("Le script CGI a retourné une erreur");
    return constructCGIResponse(output);
}

std::string HttpRequest::constructCGIResponse(const std::string& output)
{
    std::ostringstream oss;
    oss << output.size();

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + oss.str() + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "\r\n";
    response += output;

    return response;
}

std::string HttpRequest::executeCGI(const std::string& scriptPath, ServerConfig& config)
{
    (void)config;
    
    try
    {
        int outputPipe[2], inputPipe[2];
        createPipes(outputPipe, inputPipe);

        pid_t pid = fork();
        if (pid == 0)
            setupChildProcess(outputPipe, inputPipe, scriptPath);
        else if (pid > 0)
            return handleParentProcess(outputPipe, inputPipe, pid);
        else
            throw std::runtime_error("Fork failed");
    }
    catch (const std::exception& e)
    {
        std::cerr << "Erreur CGI : " << e.what() << std::endl;
        return generateDefaultErrorPage(500);
    }
    return generateDefaultErrorPage(500);
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
    std::string targetPath = "html/uploads/" + fileName;
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
    std::string targetPath = "html/uploads/" + fileName;
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

std::string HttpRequest::handlePost(ServerConfig& config)
{
    std::string response;

    std::map<std::string, std::string>::const_iterator it = this->_headers.find("Content-Length");
    if (it == this->_headers.end())
        return findErrorPage(config, 411);

    int contentLength;
    std::istringstream lengthStream(it->second);
    lengthStream >> contentLength;

    if (contentLength == 0)
        return findErrorPage(config, 400);
    if (this->_body.size() != static_cast<std::string::size_type>(contentLength))
        return findErrorPage(config, 400);
    std::map<std::string, std::string>::const_iterator contentTypeHeader = _headers.find("Content-Type");
    if (contentTypeHeader == _headers.end())
        return findErrorPage(config, 400);

    std::string contentType = contentTypeHeader->second;
    if (contentType.find("application/json") != std::string::npos)
        return (uploadTxt(config, response));
    else if (contentType.find("multipart/form-data") != std::string::npos)
        return (uploadFile(config, response, contentType));
    else if (contentType.find("application/x-www-form-urlencoded") != std::string::npos)
    {
        std::string scriptPath = _path;
        scriptPath = config.getRoot() + scriptPath.substr(1);
        return executeCGI(scriptPath, config);
    }
    return findErrorPage(config, 415);
}




std::string HttpRequest::handleDelete(ServerConfig& config)
{
    std::string response;

     std::string resourcePath = "upload/" + _path;

    struct stat fileStat;
    if (stat(resourcePath.c_str(), &fileStat) != 0)
        return findErrorPage(config, 404);

    if (access(resourcePath.c_str(), W_OK) != 0)
        return findErrorPage(config, 403);

    std::map<std::string, std::string>::const_iterator methodIt = _headers.find("Allow");
    if (methodIt != _headers.end() && methodIt->second.find("DELETE") == std::string::npos)
    {
        response = "HTTP/1.1 405 Method Not Allowed\r\n";
        response += "Allow: " + methodIt->second + "\r\n";
        response += "\r\n";
        return response;
    }
    if (unlink(resourcePath.c_str()) != 0)
        return findErrorPage(config, 500);

    response = "HTTP/1.1 204 No Content\r\n";
    response += "Content-Length: 0\r\n";
    response += "\r\n";

    return response;
}

std::string HttpRequest::findErrorPage(ServerConfig& config, int errorCode)
{
    std::string errorPagePath = config.getErrorPage(errorCode);
    if (errorPagePath.empty())
    {
        std::cerr << "Erreur : Aucune page d'erreur définie pour le code " << errorCode << std::endl;
        return generateDefaultErrorPage(errorCode);
    }
    std::string fullPath =  errorPagePath;
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0 || access(fullPath.c_str(), R_OK) != 0)
    {
        std::cerr << "Erreur : La page d'erreur " << fullPath << " est introuvable ou inaccessible" << std::endl;
        return generateDefaultErrorPage(errorCode);
    }
    std::ifstream errorFile(fullPath.c_str());
    if (!errorFile.is_open())
    {
        std::cerr << "Erreur : Impossible d'ouvrir la page d'erreur " << fullPath << std::endl;
        return generateDefaultErrorPage(errorCode);
    }

    std::string content;
    std::string line;
    while (std::getline(errorFile, line))
        content += line + "\n";
    errorFile.close();

    std::ostringstream response;
    response << "HTTP/1.1 " << errorCode << " Error\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "\r\n";
    response << content;

    return response.str();
}

std::string HttpRequest::generateDefaultErrorPage(int errorCode)
{
    std::ostringstream response;
    response << "<html><body><h1>Error " << errorCode << "</h1><p>Page not found.</p></body></html>";

    std::ostringstream httpResponse;
    httpResponse << "HTTP/1.1 " << errorCode << " Error\r\n";
    httpResponse << "Content-Type: text/html\r\n";
    httpResponse << "Content-Length: " << response.str().size() << "\r\n";
    httpResponse << "\r\n";
    httpResponse << response.str();

    return httpResponse.str();
}

std::string HttpRequest::getPath() const
{
    return _path;
}

HttpRequest::~HttpRequest()
{
}