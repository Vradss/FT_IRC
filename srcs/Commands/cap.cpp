/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cap.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/18 23:03:15 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/19 21:31:33 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../includes/Server.hpp"

static const size_t CAP_MAX_REQ = 400;

std::string toUpperStr(const std::string &s)
{
    std::string result(s);
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
    return result;
}

/*
    este método sirve para las capacidades de un servidor
    las lista y las podrias pedir en el caso de que nuestro servidor tuviera
    pero no las tiene entonces cuando nos piden de listar ponemos que no hay y ya
    es mas administrativo que otra cosa 
    
*/
void Server::handleCap(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";

    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    const std::string target = client->isRegistered() ? client->getNick() : "*";

    // sin comando (args del cap)
    if (params.empty() || params[0].empty())
    {
        client->queueMessage(srv + "461 " + nick + " CAP :Not enough parameters\r\n");
        return;
    }

    // subcomando no distinque mayus (Ver esto con César )
    const std::string sub = toUpperStr(params[0]);

    if (sub == "LS")
    {
        // Capacidades disponibles: ninguna, sladrá vacia
        client->queueMessage(srv + "CAP " + target + " LS :\r\n");
    }
    else if (sub == "LIST")
    {
        // Capacidades activas en esta conexión: lisyta vacia
        client->queueMessage(srv + "CAP " + target + " LIST :\r\n");
    }
    else if (sub == "REQ")
    {
        //TO-DO: considerar quitar porq total no hace nada 
        // El cliente pide activar capacidades: se rechaza todo, devolviendo
        // la lista tal como la pidió.
        if (params.size() < 2)
        {
            client->queueMessage(srv + "461 " + nick + " CAP :Not enough parameters\r\n");
            return;
        }
        std::string requested = params[1];
        if (requested.size() > CAP_MAX_REQ)
            requested = requested.substr(0, CAP_MAX_REQ);
        client->queueMessage(srv + "CAP " + target + " NAK :" + requested + "\r\n");
    }
    else if (sub == "END")
    {
        // Fin de la negociación: no hacesmos  ni responde nada.

    }
    else
    {
        // args comando desconocido
        client->queueMessage(srv + "410 " + nick + " " + params[0] + " :Invalid CAP command\r\n");
    }
}