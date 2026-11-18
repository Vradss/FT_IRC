/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ping.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/18 21:34:12 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/18 22:35:34 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../includes/Server.hpp"

static const size_t PING_MAX_TEXT = 400;

void Server::handlePing(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";

    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    if (params.empty() || params[0].empty())
    {
        client->queueMessage(srv + "409 " + nick + " :No origin specified\r\n");
        return;
    }

    // texto que tenemos que mandar para que lo devuelva pong
    // el segundo parámetro, es el "destino" del RFC antiguo lo ignoramos
    std::string pingText = params[0];
    if (pingText.size() > PING_MAX_TEXT)
        pingText = pingText.substr(0, PING_MAX_TEXT);

    // respuesta solo a quien mandó el PING, no se manda a canales
    client->queueMessage(srv + "PONG " + _serverName + " :" + pingText + "\r\n");
}