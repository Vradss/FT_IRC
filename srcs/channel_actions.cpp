/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channel_actions.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 22:14:39 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/18 20:28:47 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */



#include "Server.hpp"
#include "Channel.hpp"

void Server::handleTopic(Client *client, std::vector<std::string> &params)
{
	const std::string srv = ":" + _serverName + " ";
	const std::string nick = client->getNick();   // ya está registrado (lo filtra el dispatcher)
	const std::string &target = params[0];
	// Sin parámetros o primero vacío
	if (params.empty() || target.empty())
	{
		client->queueMessage(srv + "CODE " + nick + " :MESSAGE\r\n");
		return;
	}
	// Parámetros de más
	if (params.size() > 2)
	{
		client->queueMessage(srv + "CODE " + nick + " :MESSAGE\r\n");
		return;
	}
	// Ver si es un canal
	std::string lowered(target);
	for (size_t j = 0; j < lowered.size(); ++j)
		lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));
	if (target[0] == '#')
	{
		Channel *channel = findChannel(target);
		// Ver si es un canal
		if (!channel)
		{
			client->queueMessage(srv + "403 " + nick + " " + target + " :No such channel\r\n");
			return;
		}
		// Ver si el usuario pertenece al canal 
		if (!channel->isMember(client))
		{
			client->queueMessage(srv + "404 " + nick + " " + channel->getChName() + " :Cannot send to channel\r\n");
			return;
		}
	}
	else
	{
		client->queueMessage(srv + "CODE " + nick + " :MESSAGE\r\n");
		return;
	}
		// Si solo hay 1, entonces es consulta del topic
	if (params.size() == 1)
	{
		
	}
			// Comprobar si tiene topic. Si no, error	:ircserv 331 Pepe #canal :No topic is set\r\n
			// Mostrar topic	:ircserv 332 Pepe #canal :Bienvenidos a nuestro canal\r\n
		// Si hay 2, entonces es modificación
			// Ver si el modo topic está desbloqueado
				// Si está bloqueado, consultar si es operador
					// Reemplazar topic
}