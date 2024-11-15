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

volatile sig_atomic_t Server::signal_received = 0;

std::string Server::intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Constructeur
Server::Server(const std::string& configFile) : _server_fds(0)
{
    if(_config.parseConfigFile(configFile) == false)
        throw std::runtime_error("Parsing failed.");
    _ports = _config.getPorts();
    _config.display();
    initSockets();
}
Server::~Server() {
    for (size_t i = 0; i < _server_fds.size(); ++i) {
        if (_server_fds[i] != -1) {
        close(_server_fds[i]);
        _server_fds[i] = -1; // Marquer le descripteur comme invalide
    }
    }
}

void Server::initSockets() {
    for (size_t i = 0; i < _ports.size(); ++i) {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "Erreur : création du socket échouée pour le port " << _ports[i] << std::endl;
            exit(EXIT_FAILURE);
        }

        // Activer l'option SO_REUSEADDR
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Erreur : setsockopt(SO_REUSEADDR) échouée" << std::endl;
            exit(EXIT_FAILURE);
        }

        sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(_ports[i]);
        _addresses.push_back(address);

        if (bind(server_fd, (sockaddr*)&_addresses[i], sizeof(_addresses[i])) < 0) {
            std::cerr << "Erreur : bind échoué pour le port " << _ports[i] << std::endl;
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 10) < 0) {
            std::cerr << "Erreur : listen échoué pour le port " << _ports[i] << std::endl;
            exit(EXIT_FAILURE);
        }

        _server_fds.push_back(server_fd);

        struct pollfd server_poll_fd;
        server_poll_fd.fd = server_fd;
        server_poll_fd.events = POLLIN;
        _poll_fds.push_back(server_poll_fd);

        std::cout << "Serveur en écoute sur le port " << _ports[i] << std::endl;
    }
}

void Server::run() {
    this->running = true;
    while (running) {
        int poll_count = poll(_poll_fds.data(), _poll_fds.size(), -1);
        if (poll_count < 0 && running) {
            std::cerr << "Erreur : poll() échoué" << std::endl;
            continue;
        }

        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].revents & POLLIN) {
                if (std::find(_server_fds.begin(), _server_fds.end(), _poll_fds[i].fd) != _server_fds.end()) {
                    handleNewConnection(_poll_fds[i].fd);
                } else {
                    handleClientRequest(i);
                }
            }
        }
    }
}

void Server::handleNewConnection(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd < 0) {
        std::cerr << "Erreur lors de l'acceptation de la nouvelle connexion" << std::endl;
        return;
    }

    // Initialisation sécurisée de pollfd
    struct pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    client_pollfd.revents = 0; // Initialiser à 0 pour éviter des valeurs indéfinies

    _poll_fds.push_back(client_pollfd);
    std::cout << "Nouvelle connexion acceptée sur le port " << ntohs(client_addr.sin_port) << std::endl;
}


void Server::handleClientRequest(int clientIndex)
{
    int client_fd = _poll_fds[clientIndex].fd;

    std::string buffer = readClientRequest(client_fd, clientIndex);
    if (buffer.empty()) return;
    std::cout << "Requête reçue : \n" << buffer << std::endl;

    HttpRequest request(buffer);

    std::string response = request.handleRequest(_config);
    std::cout << "\033[34m\n" << response << "\n\033[0m" << std::endl;
    send(client_fd, response.c_str(), response.size(), 0);
}



std::string Server::readClientRequest(int client_fd, int clientIndex) {
    std::string buffer;
    char tempBuffer[1024];
    ssize_t bytes_read;

    // Lire les données du client
    while ((bytes_read = read(client_fd, tempBuffer, sizeof(tempBuffer) - 1)) > 0) {
        tempBuffer[bytes_read] = '\0';
        buffer += std::string(tempBuffer, bytes_read);
        if (buffer.find("\r\n\r\n") != std::string::npos) break;
    }

    if (bytes_read <= 0) {
        removeClient(clientIndex);
        return "";
    }

    // Extraire Content-Length pour lire le corps
    size_t contentLength = 0;
    size_t headerEndPos = buffer.find("\r\n\r\n");
    if (headerEndPos != std::string::npos) {
        size_t contentLengthPos = buffer.find("Content-Length:");
        if (contentLengthPos != std::string::npos) {
            std::istringstream(buffer.substr(contentLengthPos + 15)) >> contentLength;
        }

        // Lire le reste du corps si nécessaire
        size_t bodyStartPos = headerEndPos + 4;
        while (buffer.size() - bodyStartPos < contentLength) {
            bytes_read = read(client_fd, tempBuffer, sizeof(tempBuffer) - 1);
            if (bytes_read <= 0) {
                removeClient(clientIndex);
                return "";
            }
            tempBuffer[bytes_read] = '\0';
            buffer += std::string(tempBuffer, bytes_read);
        }
    }

    return buffer;
}

std::string Server::extractRequestedPath(const std::string& buffer)
{
    // Extraire le chemin demandé depuis la requête HTTP
    size_t pathStart = buffer.find(" ") + 1;
    size_t pathEnd = buffer.find(" ", pathStart);
    std::string requestedPath;

    if (pathStart != std::string::npos && pathEnd != std::string::npos)
        requestedPath = buffer.substr(pathStart, pathEnd - pathStart);
    else
        requestedPath = "/";

    if (requestedPath == "/")
    {
        requestedPath = _config.getIndex();
        if (requestedPath.empty())
            requestedPath = "/index.html";
    }

    return "html/" + requestedPath;
}

std::string Server::generateHttpResponse(const std::string& requestedPath)
{
    std::ifstream file(requestedPath.c_str());
    if (file)
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string body = buffer.str();

        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Content-Length: " + intToString(body.size()) + "\r\n\r\n" +
               body;
    }
    else
    {
        std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
        return "HTTP/1.1 404 Not Found\r\n"
               "Content-Type: text/html\r\n"
               "Content-Length: " + intToString(body.size()) + "\r\n\r\n" +
               body;
    }
}

void Server::removeClient(int index) {
    // Ferme le descripteur de fichier du client pour libérer les ressources
    if (_poll_fds[index].fd != -1) {
        close(_poll_fds[index].fd);
        _poll_fds[index].fd = -1; // Marquer le descripteur comme invalide
    }

    // Supprime le descripteur de poll de la liste
    _poll_fds.erase(_poll_fds.begin() + index);

    std::cout << "Connexion client fermée et supprimée de la liste" << std::endl;
}

void Server::stop() {
    running = false;
    // Arrêter le serveur en fermant tous les descripteurs de socket

    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        if (_poll_fds[i].fd != -1) {
            close(_poll_fds[i].fd);
            _poll_fds[i].fd = -1; // Marquer le descripteur comme invalide
        }
    }
    _poll_fds.clear();

    // Fermer tous les sockets du serveur
    for (size_t i = 0; i < _server_fds.size(); ++i) {
        if (_server_fds[i] != -1) {
            close(_server_fds[i]);
            _server_fds[i] = -1; // Assurez-vous que le descripteur est marqué comme invalide après la fermeture
        }
    }


    // Mettre à jour l'état du serveur
    std::cout << "Serveur arrêté avec succès." << std::endl;
}

bool Server::isRunning() const {
    return this->running;
}

void    Server::setRunning(bool status)
{
    this->running = status;
}

void Server::signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "Signal (" << signal << ") reçu. Arrêt du serveur en cours..." << std::endl;
        signal_received = true;
    }
}