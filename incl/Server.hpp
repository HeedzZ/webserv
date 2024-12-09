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
#include <stdexcept>
#include <fstream>
#include <arpa/inet.h>
#include "ServerConfig.hpp"

class Server {
public:
    // Constructeur et destructeur
    Server(const std::string& configFile);
    ~Server();

    // Méthodes principales
    void initSockets();
    void run();                      
    void stop();
    bool isRunning() const;
    void setRunning(bool status);

    // Gestion des signaux
    static void signalHandler(int signal);

    std::string intToString(int value);
    void logMessage(const std::string& level, const std::string& message) const;
    std::string logMessageError(const std::string& level, const std::string& message) const;

    ServerConfig* getConfigForSocket(int socket);
    ServerConfig* getConfigForRequest(const std::string& hostHeader);

private:
    bool parseConfigFile(const std::string& configFile);
    // Helpers pour les sockets
    int createSocket();
    void configureSocket(int server_fd);
    void bindSocket(int server_fd, int port);
    void listenOnSocket(int server_fd);
    void addServerSocketToPoll(int server_fd);
    void cleanupSockets();
    void cleanup();

    // Gestion des connexions
    void handleNewConnection(int server_fd);
    void handleClientRequest(int clientIndex);
    void logResponseDetails(const std::string& response, const std::string& path);
    std::string readClientRequest(int client_fd, int clientIndex);
    void removeClient(int index);

    // Utilitaires HTTP
    std::string generateHttpResponse(const std::string& requestedPath);
    std::string extractRequestedPath(const std::string& buffer);

    // Vérification des sockets
    bool isServerSocket(int fd) const;

    // Variables membres
    bool running;
    std::vector<int> _server_fds;      // Liste des sockets serveurs
    std::vector<int> _ports;           // Liste des ports en écoute
    std::vector<sockaddr_in> _addresses; // Adresses des sockets
    std::vector<pollfd> _poll_fds;     // Liste des descripteurs pour poll
    std::vector<ServerConfig> _configs;
    std::map<int, ServerConfig*> _socketToConfig;

    // Variable statique pour gérer les signaux
    static volatile sig_atomic_t signal_received;
};

#endif // SERVER_HPP