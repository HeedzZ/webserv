#include "Server.hpp"
#include "HttpRequest.hpp"
#include "ServerConfig.hpp"

Server::Server(const std::string& configFile) : running(false)
{
    logMessage("INFO", "Initializing the server...");
    try
    {
        if (!parseConfigFile(configFile))
            throw std::runtime_error("Failed to parse configuration file: " + configFile);
        validateServerConfigurations();
        initSockets();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error during server initialization: " << e.what() << std::endl;
        cleanup();
        throw;
    }
}

Server::~Server()
{
    cleanupSockets();
}

void Server::cleanup()
{
    logMessage("INFO", "Cleaning up resources...");
    _configs.clear();
    cleanupSockets();
}

bool Server::parseConfigFile(const std::string& configFile)
{
    std::ifstream file(configFile.c_str());
    if (!file.is_open())
    {
        std::cerr << "Could not open the file: " << configFile << std::endl;
        return false;
    }

    try
    {
        while (file.peek() != EOF)
        {
            std::auto_ptr<ServerConfig> currentConfig(new ServerConfig());
            if (currentConfig->parseServerBlock(file))
                _configs.push_back(*currentConfig);
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cerr << "Unexpected error while parsing configuration." << std::endl;
        return false;
    }

    return !_configs.empty();
}

void Server::validateServerConfigurations()
{
    for (size_t i = 0; i < _configs.size(); ++i)
    {
        const std::string& host1 = _configs[i].getHost();
        const std::vector<int>& ports1 = _configs[i].getPorts();
        const std::string& serverName1 = _configs[i].getServerName();

        for (size_t j = i + 1; j < _configs.size(); ++j)
        {
            const std::string& host2 = _configs[j].getHost();
            const std::vector<int>& ports2 = _configs[j].getPorts();
            const std::string& serverName2 = _configs[j].getServerName();

            for (size_t p1 = 0; p1 < ports1.size(); ++p1)
            {
                for (size_t p2 = 0; p2 < ports2.size(); ++p2)
                {
                    if (ports1[p1] == ports2[p2] && host1 == host2)
                    {
                        if (serverName1.empty() && serverName2.empty())
                        {
                            throw std::runtime_error(
                                "Configuration error: Multiple servers on the same host (" +
                                host1 + ") and port (" + intToString(ports1[p1]) +
                                ") without server_name.");
                        }
                        if (!serverName1.empty() && !serverName2.empty() && serverName1 == serverName2)
                        {
                            throw std::runtime_error(
                                "Configuration error: Duplicate server_name (" + serverName1 +
                                ") on the same host (" + host1 + ") and port (" + intToString(ports1[p1]) + ").");
                        }
                    }
                }
            }
        }
    }
}

void Server::logMessage(const std::string& level, const std::string& message) const
{
    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);
    char timeBuffer[20];

    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);
    std::cout << "[" << level << "] " << timeBuffer << " - " << message << std::endl;
}

std::string Server::logMessageError(const std::string& level, const std::string& message) const
{
    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);

    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);

    std::ostringstream oss;
    oss << "[" << level << "] " << timeBuffer << " - " << message;

    return oss.str();
}

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

std::string Server::intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

void Server::displayConfigs(const std::vector<ServerConfig>& configs)
{
    std::cout << "==== Displaying Server Configurations ====" << std::endl;

    if (configs.empty())
    {
        std::cout << "No configurations found." << std::endl;
        return;
    }

    for (size_t i = 0; i < configs.size(); ++i)
    {
        std::cout << "Configuration #" << i + 1 << ":" << std::endl;
        std::cout << configs[i].toString() << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
    }
}
