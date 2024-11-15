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
#include <algorithm>
#include <csignal>
#include "ServerConfig.hpp"

class Server {
public:
    // Constructeur
    Server(const std::string& configFile);
    ~Server();

    // Méthodes principales
    void initSockets();               // Crée le socket et le configure
    void bindSocket();               // Attache le socket à une adresse et un port
    void listenSocket();             // Met le socket en écoute
    void run();                      // Boucle principale qui gère les connexions et les requêtes
    void handleNewConnection(int server_fd);      // Gère l'acceptation des nouvelles connexions
    void handleClientRequest(int clientIndex); // Traite les requêtes des clients connectés
    void stop();
    std::string generateHttpResponse(const std::string& requestedPath);
    std::string extractRequestedPath(const std::string& buffer);
    std::string readClientRequest(int client_fd, int clientIndex);
    std::string intToString(int value);
    bool isRunning() const;
    void    setRunning(bool status);
    static void signalHandler(int signal);
    static volatile sig_atomic_t signal_received;



private:
    bool running;
    std::vector<int> _server_fds;      // Liste des descripteurs de fichiers des sockets
    std::vector<int> _ports;           // Liste des ports sur lesquels le serveur écoute
    std::vector<sockaddr_in> _addresses;                     // Port sur lequel le serveur écoute
    std::vector<pollfd> _poll_fds;   // Vecteur de pollfd pour gérer les connexions client
    ServerConfig    _config;

    void addClient(int client_fd);   // Ajoute un client au vecteur de poll
    void removeClient(int index);    // Supprime un client du vecteur de poll
    std::string createHttpResponse(int statusCode, const std::string& contentType, const std::string& body); // Génère une réponse HTTP simple
    std::string getStatusMessage(int statusCode);

};

#endif // SERVER_HPP