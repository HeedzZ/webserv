#include <iostream>
#include <csignal>
#include "Server.hpp"

Server* globalServerPointer = NULL;

void signalHandlerWrapper(int signal)
{
    if (globalServerPointer != NULL)
    {
        std::cout << "Signal (" << signal << ") reçu. Fermeture du serveur en cours..." << std::endl;
        globalServerPointer->stop();
    }
}

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
        std::cerr << "Erreur: " << e.what() << std::endl;
    }
    return 0;
}
