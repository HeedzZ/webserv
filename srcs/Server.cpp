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


std::string intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Constructeur
Server::Server(int port, ServerConfig config) : _server_fd(0), _addrlen(sizeof(_address)), _port(port), _config(config)
{}

// Crée et configure le socket du serveur
void Server::initSocket()
{
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
    {
        std::cerr << "Erreur : création du socket échouée" << std::endl;
        exit(EXIT_FAILURE);
    }

    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_port);
}

// Attache le socket à une adresse et un port
void Server::bindSocket()
{
    if (bind(_server_fd, (sockaddr*)&_address, sizeof(_address)) < 0)
    {
        std::cerr << "Erreur : bind échoué" << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Met le socket en écoute pour les connexions entrantes
void Server::listenSocket()
{
    if (listen(_server_fd, 10) < 0)
    {
        std::cerr << "Erreur : listen échoué" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Serveur en écoute sur le port " << _port << std::endl;

    // Ajouter le socket du serveur au vecteur pour poll
    pollfd server_poll_fd;
    server_poll_fd.fd = _server_fd;
    server_poll_fd.events = POLLIN; // Surveiller les nouvelles connexions
    _poll_fds.push_back(server_poll_fd);
}

// Boucle principale du serveur
void Server::run(ServerConfig config)
{
    while (true)
    {
        int poll_count = poll(_poll_fds.data(), _poll_fds.size(), -1);
        if (poll_count < 0)
        {
            std::cerr << "Erreur : poll() échoué" << std::endl;
            continue;
        }
        // Parcourir les descripteurs surveillés pour gérer les événements
        for (size_t i = 0; i < _poll_fds.size(); ++i)
        {
            if (_poll_fds[i].revents & POLLIN)
            {
                if (_poll_fds[i].fd == _server_fd)
                    handleNewConnection();
                else
                    handleClientRequest(i, config);
            }
        }
    }
}

// Gère l'acceptation des nouvelles connexions
void Server::handleNewConnection()
{
    int new_socket = accept(_server_fd, (sockaddr*)&_address, (socklen_t*)&_addrlen);
    if (new_socket < 0)
    {
        std::cerr << "Erreur : accept() échoué" << std::endl;
        return;
    }
    std::cout << "Nouvelle connexion acceptée" << std::endl;
    addClient(new_socket);
}

// Ajoute un client au vecteur de poll
void Server::addClient(int client_fd)
{
    pollfd client_poll_fd;
    client_poll_fd.fd = client_fd;
    client_poll_fd.events = POLLIN; // Surveiller les données reçues
    _poll_fds.push_back(client_poll_fd);
}

// Supprime un client du vecteur de poll
void Server::removeClient(int index)
{
    close(_poll_fds[index].fd);
    _poll_fds.erase(_poll_fds.begin() + index);
    std::cout << "Connexion client fermée" << std::endl;
}

void Server::handleClientRequest(int clientIndex, ServerConfig config)
{
    int client_fd = _poll_fds[clientIndex].fd;

    std::string buffer = readClientRequest(client_fd, clientIndex);
    if (buffer.empty()) return;
    std::cout << "Requête reçue : \n" << buffer << std::endl;

    HttpRequest request(buffer);

    std::string response = request.handleRequest(config);
    send(client_fd, response.c_str(), response.size(), 0);
}



std::string Server::readClientRequest(int client_fd, int clientIndex)
{
    std::string buffer;
    char tempBuffer[1024];
    ssize_t bytes_read;

    while (true)
    {
        bytes_read = read(client_fd, tempBuffer, sizeof(tempBuffer) - 1);
        if (bytes_read <= 0)
        {
            removeClient(clientIndex);
            return "";
        }

        tempBuffer[bytes_read] = '\0';
        buffer += std::string(tempBuffer, bytes_read);
        if (buffer.find("\r\n\r\n") != std::string::npos)
            break;
    }

    return buffer;
}

std::string Server::extractRequestedPath(const std::string& buffer, const ServerConfig& config)
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
        requestedPath = config.getIndex();
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



