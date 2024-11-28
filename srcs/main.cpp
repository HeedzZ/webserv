#include <iostream>
#include <csignal>
#include "Server.hpp"
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

Server* globalServerPointer = NULL;

void signalHandlerWrapper(int signal)
{
    (void) signal;
    if (globalServerPointer != NULL)
    {
        std::cout << std::endl;
        globalServerPointer->stop();
    }
}

/*void logMessage(const std::string& level, const std::string& message) const
{
    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);

    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);

    std::cout << "[" << level << "] " << timeBuffer << " - " << message << std::endl;
}*/

int main(int argc, char* argv[])
{
    (void) argc;
    try {
        Server server(argv[1]);
        globalServerPointer = &server;
        
        std::signal(SIGINT, signalHandlerWrapper);
        
        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
