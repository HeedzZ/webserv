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

// Initialize static variable
volatile sig_atomic_t Server::signal_received = 0;

// Constructor
Server::Server(const std::string& configFile) : running(false)
{
    logMessage("INFO", "Initializing the server...");
    if (!_config.parseConfigFile(configFile))
        throw std::runtime_error(logMessageError("ERROR", "Failed to parse configuration file."));
    logMessage("INFO", "Configuration file parsed successfully.");
    _ports = _config.getPorts();
    //_config.display();
    initSockets();
}

// Destructor
Server::~Server()
{
    cleanupSockets();
}

// Log a message
void Server::logMessage(const std::string& level, const std::string& message) const
{
    // Get the current time
    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);

    // Format the time manually
    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);

    // Print the formatted log message
    std::cout << "[" << level << "] " << timeBuffer << " - " << message << std::endl;
}

// Generate a log message as a string
std::string Server::logMessageError(const std::string& level, const std::string& message) const
{
    // Get the current time
    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);

    // Format the time manually
    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);

    // Construct the log message
    std::ostringstream oss;
    oss << "[" << level << "] " << timeBuffer << " - " << message;

    // Return the log message
    return oss.str();
}

// Convert an integer to a string
std::string Server::intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Initialize sockets
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

// Create a socket
int Server::createSocket()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        throw std::runtime_error(logMessageError("ERROR", "Failed to create socket."));
    logMessage("INFO", "Socket created successfully.");
    return server_fd;
}

// Configure socket options
void Server::configureSocket(int server_fd)
{
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(server_fd);
        throw std::runtime_error(logMessageError("ERROR", "Failed to configure socket options (SO_REUSEADDR)."));
    }
    logMessage("INFO", "Socket configured with SO_REUSEADDR.");
}

// Bind a socket to an address and port
void Server::bindSocket(int server_fd, int port)
{
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0)
    {
        close(server_fd);
        throw std::runtime_error(logMessageError("ERROR", "Failed to bind socket."));
    }
    logMessage("INFO", "Socket bound to port " + intToString(port) + ".");
    _addresses.push_back(address);
}

// Set a socket to listen for connections
void Server::listenOnSocket(int server_fd)
{
    if (listen(server_fd, 10) < 0)
    {
        close(server_fd);
        throw std::runtime_error(logMessageError("ERROR", "Failed to set socket to listen."));
    }
    logMessage("INFO", "Socket is now listening.");
}

// Add a server socket to the poll array
void Server::addServerSocketToPoll(int server_fd)
{
    struct pollfd server_poll_fd = {};
    server_poll_fd.fd = server_fd;
    server_poll_fd.events = POLLIN;
    _poll_fds.push_back(server_poll_fd);
    _server_fds.push_back(server_fd);

    logMessage("INFO", "Server is listening on port " + intToString(ntohs(_addresses.back().sin_port)) + ".");
}

// Clean up all sockets
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
    logMessage("INFO", "All sockets have been cleaned up.");
}

// Main server loop
void Server::run()
{
    logMessage("INFO", "Server is running...");
    running = true;
    while (running)
    {
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), -1);
        if (poll_count < 0)
        {
            if (!running) break; // Server stopped
            logMessage("ERROR", "Poll failed.");
            continue;
        }

        for (size_t i = 0; i < _poll_fds.size(); ++i)
        {
            if (_poll_fds[i].revents & POLLIN)
            {
                if (isServerSocket(_poll_fds[i].fd))
                {
                    logMessage("INFO", "New connection detected.");
                    handleNewConnection(_poll_fds[i].fd);
                }
                else
                {
                    logMessage("INFO", "Client request detected.");
                    handleClientRequest(i);
                }
            }
        }
    }
    logMessage("INFO", "Server stopped.");
}

// Check if a file descriptor is a server socket
bool Server::isServerSocket(int fd) const
{
    return std::find(_server_fds.begin(), _server_fds.end(), fd) != _server_fds.end();
}

// Handle a new connection
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
    logMessage("INFO", "New connection accepted.");
}

// Handle a client request
void Server::handleClientRequest(int clientIndex)
{
    int client_fd = _poll_fds[clientIndex].fd;

    std::string buffer = readClientRequest(client_fd, clientIndex);
    if (buffer.empty()) return;

    HttpRequest request(buffer);
    std::string response = request.handleRequest(_config);

    logResponseDetails(response, request.getPath());
    send(client_fd, response.c_str(), response.size(), 0);
}

// Log details of the response
void Server::logResponseDetails(const std::string& response, const std::string& path)
{
    size_t statusLineEnd = response.find("\r\n");
    if (statusLineEnd == std::string::npos)
    {
        logMessage("ERROR", "Malformed response for path: " + path);
        return;
    }

    std::string statusLine = response.substr(0, statusLineEnd);
    size_t statusCodeStart = statusLine.find(" ") + 1;
    size_t statusCodeEnd = statusLine.find(" ", statusCodeStart);
    std::string statusCode = statusLine.substr(statusCodeStart, statusCodeEnd - statusCodeStart);

    size_t contentTypeStart = response.find("Content-Type: ");
    std::string contentType = "Unknown";
    if (contentTypeStart != std::string::npos)
    {
        size_t contentTypeEnd = response.find("\r\n", contentTypeStart);
        contentType = response.substr(contentTypeStart + 14, contentTypeEnd - (contentTypeStart + 14));
    }

    logMessage("INFO", "Response sent: " + statusCode + " for " + path + " with Content-Type: " + contentType);
}

// Read a client request
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
        logMessage("WARNING", "Client connection closed or read error occurred.");
        removeClient(clientIndex);
        return "";
    }

    logMessage("INFO", "Complete request received (" + intToString(buffer.size()) + " bytes).");
    return buffer;
}

// Remove a client from the poll array
void Server::removeClient(int index)
{
    if (_poll_fds[index].fd != -1)
    {
        logMessage("INFO", "Disconnecting client with descriptor " + intToString(_poll_fds[index].fd) + ".");
        close(_poll_fds[index].fd);
        _poll_fds[index].fd = -1;
    }
    _poll_fds.erase(_poll_fds.begin() + index);
}

// Stop the server
void Server::stop()
{
    logMessage("INFO", "Stopping the server...");
    running = false;
    cleanupSockets();
    logMessage("INFO", "Server stopped successfully.");
}

// Signal handler
void Server::signalHandler(int signal)
{
    if (signal == SIGINT)
        signal_received = 1;
}
