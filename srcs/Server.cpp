/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ymostows <ymostows@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/22 16:44:32 by ymostows          #+#    #+#             */
/*   Updated: 2024/10/22 16:44:32 by ymostows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "HttpRequest.hpp"
#include "ServerConfig.hpp"


// Initialize static variable
volatile sig_atomic_t Server::signal_received = 0;

int Server::createSocket()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
        throw std::runtime_error(logMessageError("ERROR", "Failed to create socket."));
    return server_fd;
}


void Server::initSockets()
{
    for (size_t i = 0; i < _configs.size(); ++i) {
        const std::vector<int>& ports = _configs[i].getPorts();
        const std::string& host = _configs[i].getHost();
        //const std::string& serverName = _configs[i].getServerName();

        for (size_t j = 0; j < ports.size(); ++j) {
            bool configExists = false;

            for (std::map<int, ServerConfig*>::iterator it = _socketToConfig.begin(); it != _socketToConfig.end(); ++it) {
                ServerConfig* existingConfig = it->second;

                if (existingConfig->getServerName().empty() && 
                    existingConfig->getHost() == host && 
                    std::find(existingConfig->getPorts().begin(), existingConfig->getPorts().end(), ports[j]) != existingConfig->getPorts().end()) {
                    configExists = true;
                    //logMessage("INFO", "Configuration already exists for " + host + ":" + intToString(ports[j]));
                    break;
                }
            }

            if (configExists)
                continue;
            int server_fd = createSocket();
            try {
                configureSocket(server_fd);

                sockaddr_in address = {};
                address.sin_family = AF_INET;
                address.sin_addr.s_addr = inet_addr(host.c_str());
                address.sin_port = htons(ports[j]);

                if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0)
                {
                    close(server_fd);
                    throw std::runtime_error("Failed to bind socket for host: " + host);
                }

                _addresses.push_back(address);
                listenOnSocket(server_fd);
                _socketToConfig[server_fd] = &_configs[i];
                addServerSocketToPoll(server_fd);
                logMessage("INFO", "Server is listening on " + host + ":" + intToString(ports[j]));
            } 
            catch (const std::exception& e)
            {
                close(server_fd);
                logMessage("ERROR", e.what());
                continue;
            }
        }
    }
}

void Server::configureSocket(int server_fd)
{
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(server_fd);
        throw std::runtime_error(logMessageError("ERROR", "Failed to configure socket options (SO_REUSEADDR)."));
    }
}

void Server::listenOnSocket(int server_fd)
{
    if (listen(server_fd, 10) < 0)
    {
        close(server_fd);
        throw std::runtime_error(logMessageError("ERROR", "Failed to set socket to listen."));
    }
}

void Server::addServerSocketToPoll(int server_fd)
{
    struct pollfd server_poll_fd = {};
    server_poll_fd.fd = server_fd;
    server_poll_fd.events = POLLIN;
    _poll_fds.push_back(server_poll_fd);
    _server_fds.push_back(server_fd);
}

ServerConfig* Server::getConfigForSocket(int socket)
{
    if (_socketToConfig.find(socket) != _socketToConfig.end())
        return _socketToConfig[socket];
    return NULL;
}

ServerConfig* Server::getConfigForRequest(const std::string& hostHeader, int connectedPort)
{

	if (hostHeader.empty())
        return &_configs[0];

    std::string hostWithoutPort = hostHeader;
    int portFromHeader = connectedPort;
    size_t colonPos = hostHeader.find(':');
    if (colonPos != std::string::npos) {
        hostWithoutPort = hostHeader.substr(0, colonPos);
        std::istringstream ss(hostHeader.substr(colonPos + 1));
        ss >> portFromHeader;
    }

    ServerConfig* fallbackConfig = NULL;

    for (size_t i = 0; i < _configs.size(); ++i) {
        const std::string& configHost = _configs[i].getHost();
        const std::string& configServerName = _configs[i].getServerName();
        const std::vector<int>& configPorts = _configs[i].getPorts();

		if (!configServerName.empty())
		{
			if (hostHeader == configServerName) {
				if (std::find(configPorts.begin(), configPorts.end(), portFromHeader) != configPorts.end())
					return &_configs[i];
			}
		}
		else
        if (hostWithoutPort == configHost) {
            if (std::find(configPorts.begin(), configPorts.end(), portFromHeader) != configPorts.end()) {
                if (!fallbackConfig) {
                    fallbackConfig = &_configs[i];
                }
            }
        }
    }

    if (fallbackConfig)
        return fallbackConfig;
    return &_configs[0];
}

void Server::cleanupSockets()
{
    for (size_t i = 0; i < _server_fds.size(); ++i)
    {
        if (_server_fds[i] != -1)
        {
            close(_server_fds[i]);
            _server_fds[i] = -1;
        }
    }
    _poll_fds.clear();
}

void Server::run()
{
    logMessage("INFO", "Server is running...");
    running = true;

    while (running)
    {
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), -1);

        if (poll_count < 0)
        {
            if (!running)
                break;
            logMessage("ERROR", "Poll failed with error: " + std::string(strerror(errno)));
            continue;
        }

        if (poll_count == 0)
        {
            logMessage("DEBUG", "Poll timeout expired (should not happen with -1 timeout).");
            continue;
        }

        for (size_t i = 0; i < _poll_fds.size(); ++i)
        {
            if (_poll_fds[i].revents & POLLIN)
            {
                if (isServerSocket(_poll_fds[i].fd))
                    handleNewConnection(_poll_fds[i].fd);
                else
                    handleClientRequest(i);
            }
            _poll_fds[i].revents = 0;
        }
    }
}


bool Server::isServerSocket(int fd) const
{
    return std::find(_server_fds.begin(), _server_fds.end(), fd) != _server_fds.end();
}

void Server::handleNewConnection(int server_fd)
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

    if (client_fd < 0)
    {
        logMessage("ERROR", "Failed to accept new connection.");
        return;
    }

    struct pollfd client_poll_fd = {};
    client_poll_fd.fd = client_fd;
    client_poll_fd.events = POLLIN;

    _poll_fds.push_back(client_poll_fd);

    ServerConfig* config = _socketToConfig[server_fd];
    if (config)
        _socketToConfig[client_fd] = config;

    else
        logMessage("WARNING", "Could not find server configuration for client.");
}


void Server::handleClientRequest(int clientIndex)
{
    int client_fd = _poll_fds[clientIndex].fd;

    std::string buffer = readClientRequest(client_fd, clientIndex);
    if (buffer.empty()) return;

    HttpRequest request(buffer);

	std::cout << buffer << std::endl;
    std::string hostHeader = request.getHeaderValue("Host");

    int connectedPort = -1;
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    if (getsockname(client_fd, (struct sockaddr*)&addr, &addrLen) == 0) {
        connectedPort = ntohs(addr.sin_port);
    } else {
        logMessage("ERROR", "Failed to get connected port for client FD: " + intToString(client_fd));
        removeClient(clientIndex);
        return;
    }

    ServerConfig* config = getConfigForRequest(hostHeader, connectedPort);

    if (!config) {
        logMessage("ERROR", "No matching configuration found for Host: " + hostHeader);
        removeClient(clientIndex);
        return;
    }

    std::string response = request.handleRequest(*config);
    logMessage("INFO", request.getMethod() + " " + request.getPath() + " " + request.getHttpVersion() + + "\" " + intToString(request.extractStatusCode(response)) + " " + intToString(response.size()) + " \"" + request.getHeaderValue("User-Agent") + "\"");
    send(client_fd, response.c_str(), response.size(), 0);

}

std::string Server::readClientRequest(int client_fd, int clientIndex) {
    std::string buffer;
    char tempBuffer[1024];
    ssize_t bytes_read;

    // Lecture initiale pour capturer les en-têtes
    while ((bytes_read = read(client_fd, tempBuffer, sizeof(tempBuffer) - 1)) > 0) {
        tempBuffer[bytes_read] = '\0';
        buffer += std::string(tempBuffer, bytes_read);

        if (buffer.find("\r\n\r\n") != std::string::npos)
            break;
    }

    if (bytes_read <= 0) {
        removeClient(clientIndex);
        return "";
    }

    size_t transferEncodingPos = buffer.find("Transfer-Encoding: chunked");
    if (transferEncodingPos != std::string::npos) {
        std::string body;
        size_t bodyStart = buffer.find("\r\n\r\n") + 4;
        size_t currentPos = bodyStart;

        while (true) {
            // Lire plus de données si nécessaire
            size_t nextChunkPos = buffer.find("\r\n", currentPos);
            while (nextChunkPos == std::string::npos) {
                bytes_read = read(client_fd, tempBuffer, sizeof(tempBuffer) - 1);
                if (bytes_read <= 0) {
                    logMessage("WARNING", "Connection lost while reading chunks");
                    removeClient(clientIndex);
                    return "";
                }
                tempBuffer[bytes_read] = '\0';
                buffer += std::string(tempBuffer, bytes_read);
                nextChunkPos = buffer.find("\r\n", currentPos);
            }

            // Extraire et nettoyer la taille du chunk
            std::string chunkSizeHex = buffer.substr(currentPos, nextChunkPos - currentPos);
            // Supprimer les espaces et les retours à la ligne
            chunkSizeHex.erase(std::remove_if(chunkSizeHex.begin(), chunkSizeHex.end(), isspace), chunkSizeHex.end());

            // Si la taille est vide, on saute les lignes superflues
            if (chunkSizeHex.empty()) {
                currentPos = nextChunkPos + 2;
                continue;
            }

            // Conversion hexadécimale
            size_t chunkSize;
            std::istringstream(chunkSizeHex) >> std::hex >> chunkSize;

            if (chunkSize == 0) {
                // Vérification et correction : ajouter CRLF si manquant
                if (buffer.find("\r\n", nextChunkPos + 2) == std::string::npos) {
                    logMessage("ERROR", "Missing final CRLF after last chunk. Adding it.");
                    buffer += "\r\n";
                }

                // Reconstituer la requête finale
                std::string finalRequest = buffer.substr(0, transferEncodingPos);
                finalRequest += "Content-Length: " + intToString(body.length());
                finalRequest += buffer.substr(transferEncodingPos + 26, bodyStart - (transferEncodingPos + 26));
                finalRequest += body;
                return finalRequest;
            }

            size_t chunkDataStart = nextChunkPos + 2;

            // Vérification et correction : données du chunk manquantes
            while (buffer.size() < chunkDataStart + chunkSize + 2) {
                bytes_read = read(client_fd, tempBuffer, sizeof(tempBuffer) - 1);
                if (bytes_read <= 0) {
                    logMessage("WARNING", "Connection lost while reading chunk data");
                    removeClient(clientIndex);
                    return "";
                }
                tempBuffer[bytes_read] = '\0';
                buffer += std::string(tempBuffer, bytes_read);
            }

            // Ajouter les données du chunk au corps
            std::string chunkData = buffer.substr(chunkDataStart, chunkSize);

            // Vérification et correction : retirer les caractères superflus
            while (!chunkData.empty() && (chunkData[0] == '\r' || chunkData[0] == '\n')) {
                logMessage("DEBUG", "Removing extra CRLF from chunk data.");
                chunkData.erase(0, 1);
            }

            body += chunkData;

            // Vérification et correction : CRLF après les données du chunk
            size_t chunkEnd = chunkDataStart + chunkSize;
            if (buffer.substr(chunkEnd, 2) != "\r\n") {
                logMessage("ERROR", "Chunk data not properly terminated with \r\n. Adding it.");
                buffer.insert(chunkEnd, "\r\n");
            }

            // Mise à jour de la position pour le prochain chunk
            currentPos = chunkEnd + 2;
        }
    }

    return buffer;
}


void Server::removeClient(int index)
{
    int client_fd = _poll_fds[index].fd;

    if (_socketToConfig.find(client_fd) != _socketToConfig.end())
        _socketToConfig.erase(client_fd);

    if (client_fd != -1)
        close(client_fd);
    _poll_fds.erase(_poll_fds.begin() + index);
}

void Server::stop()
{
    logMessage("INFO", "Stopping the server...");
    running = false;
    cleanupSockets();
    logMessage("INFO", "Server stopped successfully.");
}

void Server::signalHandler(int signal)
{
    if (signal == SIGINT)
        signal_received = 1;
}