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
    ServerConfig config;
    
    if (!config.parseConfigFile("Bad.conf"))
    {
        std::cout << "Failed to parse configuration file." << std::endl;
        return 1;
    }
    config.display();
    Server server(config.getPort()); // faire en sorte que lorsqu'on met
                                    //localhost:8081/Yann alors il cherche le /Yann
                                    //dans le vecteur de ServeurLocation pour adapter le root du serv

    server.initSocket();
    server.bindSocket();
    server.listenSocket();

    server.run(config);

    return 0;
}
