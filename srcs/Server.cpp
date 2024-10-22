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

// Constructeur
Server::Server(int port) : _server_fd(0), _addrlen(sizeof(_address)), _port(port) {}

// Crée et configure le socket du serveur
void Server::initSocket() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0) {
        std::cerr << "Erreur : création du socket échouée" << std::endl;
        exit(EXIT_FAILURE);
    }

    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_port);
}

// Attache le socket à une adresse et un port
void Server::bindSocket() {
    if (bind(_server_fd, (sockaddr*)&_address, sizeof(_address)) < 0) {
        std::cerr << "Erreur : bind échoué" << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Met le socket en écoute pour les connexions entrantes
void Server::listenSocket() {
    if (listen(_server_fd, 10) < 0) {
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
void Server::run() {
    while (true) {
        int poll_count = poll(_poll_fds.data(), _poll_fds.size(), -1);
        if (poll_count < 0) {
            std::cerr << "Erreur : poll() échoué" << std::endl;
            continue;
        }

        // Parcourir les descripteurs surveillés pour gérer les événements
        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].revents & POLLIN) {
                if (_poll_fds[i].fd == _server_fd) {
                    handleNewConnection();
                } else {
                    handleClientRequest(i);
                }
            }
        }
    }
}

// Gère l'acceptation des nouvelles connexions
void Server::handleNewConnection() {
    int new_socket = accept(_server_fd, (sockaddr*)&_address, (socklen_t*)&_addrlen);
    if (new_socket < 0) {
        std::cerr << "Erreur : accept() échoué" << std::endl;
        return;
    }
    addClient(new_socket);
    std::cout << "Nouvelle connexion acceptée" << std::endl;
}

// Ajoute un client au vecteur de poll
void Server::addClient(int client_fd) {
    pollfd client_poll_fd;
    client_poll_fd.fd = client_fd;
    client_poll_fd.events = POLLIN; // Surveiller les données reçues
    _poll_fds.push_back(client_poll_fd);
}

// Supprime un client du vecteur de poll
void Server::removeClient(int index) {
    close(_poll_fds[index].fd);
    _poll_fds.erase(_poll_fds.begin() + index);
    std::cout << "Connexion client fermée" << std::endl;
}

void Server::handleClientRequest(int clientIndex) {
    HttpRequest request;
    int client_fd = _poll_fds[clientIndex].fd;
    std::string buffer;
    char tempBuffer[1024];  // Tampon temporaire pour lire les données
    ssize_t bytes_read;

    // Lire jusqu'à ce que le client ferme la connexion ou qu'une ligne vide soit reçue
    while (true) {
        bytes_read = read(client_fd, tempBuffer, sizeof(tempBuffer) - 1);
        if (bytes_read <= 0) {
            // Si bytes_read est 0, cela signifie que le client a fermé la connexion
            removeClient(clientIndex);
            return;
        }

        // Ajouter un terminator null à tempBuffer pour faire de la lecture une chaîne C
        tempBuffer[bytes_read] = '\0';
        
        // Ajouter les données lues au buffer
        buffer += std::string(tempBuffer, bytes_read);

        // Vérifier si la requête est terminée (ex: une ligne vide ou une condition personnalisée)
        if (buffer.find("\r\n\r\n") != std::string::npos) {
            break; // La requête HTTP est terminée
        }
    }

    // Afficher la requête reçue
    std::cout << "Requête reçue : \n" << buffer << std::endl;

    // Générer et envoyer la réponse
    request.parseHttpRequest(buffer);
    std::string response = request.handleRequest();
    send(client_fd, response.c_str(), response.size(), 0);
}

