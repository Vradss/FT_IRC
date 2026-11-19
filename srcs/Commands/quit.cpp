/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   quit.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 22:31:32 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/19 21:32:09 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"


static const size_t QUIT_MAX_REASON = 300;

/*
    there is no numeric replies y el motivo es opcional
    básicamente desconecta a un usuario 
    hay dos formas en las que se desconecta el user
        *QUIT
        *Ctrl+C, este no pasa por ningun handler y no hace limpieza y entonces hay punteros suelos 
            to-do: removeClosedClients !!!
    necesitaremso un campo en la clase que sea quitMotive ya que el mensaje con el motivo se va 
    dar una vez esté todo borrado, no en el propio método
*/
void Server::handleQuit(Client *client, const std::vector<std::string> &params)
{
    std::string reason = "Client Quit"; //el motivo por defecto en caso de que el client no haya puesto motivo
    //en este método solo voy a checar que me vienen los parámetros que necesito y a pintar el mensaje
    //al final del método desconecto 
    if (!params.empty() && !params[0].empty())
    {
        std::string text = params[0];
        if (text.size() > QUIT_MAX_REASON)
            text = text.substr(0, QUIT_MAX_REASON);
        reason = "Quit: " + text;
    }

    client->setQuitReason(reason);
    disconnect(*client, "quit");
}

