#include <iostream>
#include <csignal>
#include "Server.hpp"

// Pointeur global vers le serveur pour le gestionnaire de signal
Server* globalServerPointer = NULL;

void signalHandlerWrapper(int signal) {
    if (globalServerPointer != NULL) {
        std::cout << "Signal (" << signal << ") reçu. Fermeture du serveur en cours..." << std::endl;
        globalServerPointer->stop(); // Appel de la méthode stop du serveur
    }
}

int main(int argc, char* argv[]) {
    (void) argc;
    try {
        Server server(argv[1]);
        globalServerPointer = &server; // Assigner le pointeur global
        
        // Associer le gestionnaire de signal
        std::signal(SIGINT, signalHandlerWrapper);
        
        // Initialisation et démarrage du serveur
        server.run();  // Méthode pour démarrer le serveur
    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
    }
    return 0;
}
