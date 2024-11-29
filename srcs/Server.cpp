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
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

// Initialisation de la variable statique
volatile sig_atomic_t Server::signal_received = 0;

// Constructeur
Server::Server(const std::string& configFile) : running(false)
{
    logMessage("INFO", "Initialisation du serveur.");
    if (!_config.parseConfigFile(configFile))
        throw std::runtime_error("Parsing failed.");
    logMessage("INFO", "Fichier de configuration analysé avec succès.");
    _ports = _config.getPorts();
    //_config.display();
    initSockets();
}

// Destructeur
Server::~Server()
{
    cleanupSockets();
}

void Server::logMessage(const std::string& level, const std::string& message) const
{
    // Obtenir l'heure actuelle
    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);

    // Format d'affichage manuel
    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);

    // Afficher le message log formaté
    std::cout << "[" << level << "] " << timeBuffer << " - " << message << std::endl;
}


// Convertir un entier en string
std::string Server::intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Initialisation des sockets
void Server::initSockets()
{
    for (size_t i = 0; i < _ports.size(); ++i)
    {
        int server_fd = createSocket();
        configureSocket(server_fd);
        bindSocket(server_fd, _ports[i]);
        listenOnSocket(server_fd);
        addServerSocketToPoll(server_fd);
    }
}

// Crée un socket
int Server::createSocket()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        throw std::runtime_error("Erreur : création du socket échouée.");
    logMessage("INFO", "Socket créé avec succès.");
    return server_fd;
}

// Configure les options du socket
void Server::configureSocket(int server_fd)
{
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(server_fd);
        throw std::runtime_error("Erreur : setsockopt échoué.");
    }
    logMessage("INFO", "Socket configuré avec SO_REUSEADDR.");
}

// Lie un socket à une adresse et un port
void Server::bindSocket(int server_fd, int port)
{
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0)
    {
        close(server_fd);
        throw std::runtime_error("Erreur : bind échoué.");
    }
    logMessage("INFO", "Socket attaché au port " + intToString(port) + ".");
    _addresses.push_back(address);
}

// Met un socket en écoute
void Server::listenOnSocket(int server_fd)
{
    if (listen(server_fd, 10) < 0)
    {
        close(server_fd);
        throw std::runtime_error("Erreur : listen échoué.");
    }
}

// Ajoute un socket serveur au tableau poll
void Server::addServerSocketToPoll(int server_fd)
{
    struct pollfd server_poll_fd = {};
    server_poll_fd.fd = server_fd;
    server_poll_fd.events = POLLIN;
    _poll_fds.push_back(server_poll_fd);
    _server_fds.push_back(server_fd);

    // Remplacement de l'affichage par logMessage
    logMessage("INFO", "Le serveur écoute sur le port " + intToString(ntohs(_addresses.back().sin_port)) + ".");
}


// Nettoyage des sockets
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

// Boucle principale
void Server::run()
{
    logMessage("INFO", "Le serveur démarre.");
    running = true;
    while (running)
    {
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), -1);
        if (poll_count < 0)
        {
            if (!running) break; // Serveur arrêté
            logMessage("ERROR", "Erreur : poll() échoué.");
            continue;
        }

        for (size_t i = 0; i < _poll_fds.size(); ++i)
        {
            if (_poll_fds[i].revents & POLLIN)
            {
                if (isServerSocket(_poll_fds[i].fd))
                {
                    logMessage("INFO", "Nouvelle connexion détectée.");
                    handleNewConnection(_poll_fds[i].fd);
                }
                else
                {
                    logMessage("INFO", "Requête client détectée.");
                    handleClientRequest(i);
                }
            }
        }
    }
}


// Vérifie si un descripteur de fichier appartient à un socket serveur
bool Server::isServerSocket(int fd) const
{
    return std::find(_server_fds.begin(), _server_fds.end(), fd) != _server_fds.end();
}

// Gère une nouvelle connexion
void Server::handleNewConnection(int server_fd)
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

    if (client_fd < 0)
    {
        logMessage("ERROR", "Erreur : accept échoué.");
        return;
    }

    struct pollfd client_poll_fd = {};
    client_poll_fd.fd = client_fd;
    client_poll_fd.events = POLLIN;

    _poll_fds.push_back(client_poll_fd);
    logMessage("INFO", "Nouvelle connexion acceptée.");
}


// Gère une requête client
void Server::handleClientRequest(int clientIndex)
{
    int client_fd = _poll_fds[clientIndex].fd;

    std::string buffer = readClientRequest(client_fd, clientIndex);
    if (buffer.empty()) return;

    HttpRequest request(buffer); // Passer le pointeur pour enregistrer les logs
    std::string response = request.handleRequest(_config);

    // Mini-parser pour extraire des informations de la réponse
    logResponseDetails(response, request.getPath());

    send(client_fd, response.c_str(), response.size(), 0);
}

void Server::logResponseDetails(const std::string& response, const std::string& path)
{
    // Extraire la première ligne de la réponse (status line)
    size_t statusLineEnd = response.find("\r\n");
    if (statusLineEnd == std::string::npos) {
        logMessage("ERROR", "Réponse mal formée pour le chemin : " + path);
        return;
    }

    std::string statusLine = response.substr(0, statusLineEnd);

    // Extraire le code de statut HTTP
    size_t statusCodeStart = statusLine.find(" ") + 1;
    size_t statusCodeEnd = statusLine.find(" ", statusCodeStart);
    std::string statusCode = statusLine.substr(statusCodeStart, statusCodeEnd - statusCodeStart);

    // Extraire le Content-Type (si présent)
    size_t contentTypeStart = response.find("Content-Type: ");
    std::string contentType = "Inconnu";
    if (contentTypeStart != std::string::npos) {
        size_t contentTypeEnd = response.find("\r\n", contentTypeStart);
        contentType = response.substr(contentTypeStart + 14, contentTypeEnd - (contentTypeStart + 14));
    }

    // Loguer les informations
    logMessage("INFO", "Réponse envoyée : " + statusCode + " pour " + path + " avec Content-Type : " + contentType);
}


// Lecture de la requête client
std::string Server::readClientRequest(int client_fd, int clientIndex)
{
    std::string buffer;
    char tempBuffer[1024];
    ssize_t bytes_read;

    while ((bytes_read = read(client_fd, tempBuffer, sizeof(tempBuffer) - 1)) > 0)
    {
        tempBuffer[bytes_read] = '\0';
        buffer += std::string(tempBuffer, bytes_read);
        if (buffer.find("\r\n\r\n") != std::string::npos) 
            break;
    }

    if (bytes_read <= 0)
    {
        logMessage("WARNING", "Connexion fermée ou erreur de lecture depuis le client.");
        removeClient(clientIndex);
        return "";
    }

    logMessage("INFO", "Requête complète reçue (" + intToString(buffer.size()) + " octets).");
    return buffer;
}



// Supprime un client du poll
void Server::removeClient(int index)
{
    if (_poll_fds[index].fd != -1)
    {
        logMessage("INFO", "Déconnexion du client avec le descripteur " + intToString(_poll_fds[index].fd) + ".");
        close(_poll_fds[index].fd);
        _poll_fds[index].fd = -1;
    }
    _poll_fds.erase(_poll_fds.begin() + index);
}

// Arrête le serveur
void Server::stop()
{
    logMessage("INFO", "Arrêt du serveur en cours...");
    running = false;
    cleanupSockets();
    logMessage("INFO", "Serveur arrêté avec succès.");
}


// Gestionnaire de signal
void Server::signalHandler(int signal)
{
    if (signal == SIGINT)
        signal_received = 1;
}
