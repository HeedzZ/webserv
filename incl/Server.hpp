/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ymostows <ymostows@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/22 16:45:45 by ymostows          #+#    #+#             */
/*   Updated: 2024/10/22 16:45:45 by ymostows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <sstream>
#include "ServerConfig.hpp"

class Server {
public:
    // Constructeur
    Server(int port, ServerConfig config);

    // Méthodes principales
    void initSocket();               // Crée le socket et le configure
    void bindSocket();               // Attache le socket à une adresse et un port
    void listenSocket();             // Met le socket en écoute
    void run(ServerConfig config);                      // Boucle principale qui gère les connexions et les requêtes
    void handleNewConnection();      // Gère l'acceptation des nouvelles connexions
    void handleClientRequest(int clientIndex, ServerConfig config); // Traite les requêtes des clients connectés
    std::string generateHttpResponse(const std::string& requestedPath);
    std::string extractRequestedPath(const std::string& buffer, const ServerConfig& config);
    std::string readClientRequest(int client_fd, int clientIndex);



private:
    int _server_fd;                  // Descripteur de fichier du socket serveur
    sockaddr_in _address;            // Structure d'adresse pour le serveur
    int _addrlen;                    // Taille de la structure d'adresse
    int _port;                       // Port sur lequel le serveur écoute
    std::vector<pollfd> _poll_fds;   // Vecteur de pollfd pour gérer les connexions client
    ServerConfig    _config;

    void addClient(int client_fd);   // Ajoute un client au vecteur de poll
    void removeClient(int index);    // Supprime un client du vecteur de poll
    std::string createHttpResponse(int statusCode, const std::string& contentType, const std::string& body); // Génère une réponse HTTP simple
    std::string getStatusMessage(int statusCode);
};

#endif // SERVER_HPP