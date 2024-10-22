/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ymostows <ymostows@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/22 16:50:10 by ymostows          #+#    #+#             */
/*   Updated: 2024/10/22 16:50:10 by ymostows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "ServerConfig.hpp"

int main(void)
{
    // Crée une instance du serveur sur le port 8080
<<<<<<< HEAD
    ServerConfig config("server.conf");
    config.parseConfigFile();
    Server server(config.getPort());
=======
    Server server(8080);
>>>>>>> refs/remotes/origin/main

    // Initialise le socket, le lie à l'adresse et le met en écoute
    server.initSocket();
    server.bindSocket();
    server.listenSocket();

    // Démarre la boucle principale pour gérer les connexions
    server.run();

    return 0;
}
