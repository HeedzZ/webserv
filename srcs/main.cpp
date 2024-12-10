#include <iostream>
#include <csignal>
#include "Server.hpp"

Server* globalServerPointer = NULL;

void signalHandlerWrapper(int signal)
{
    if (signal == SIGINT && globalServerPointer != NULL)
    {
        std::cout << std::endl << "Interrupt signal received. Stopping the server..." << std::endl;
        globalServerPointer->stop();
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <configuration file>" << std::endl;
        return EXIT_FAILURE;
    }

    try
    {
        Server server(argv[1]);
        globalServerPointer = &server;

        std::signal(SIGINT, signalHandlerWrapper);

        server.run();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "Unhandled unknown error occurred!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}