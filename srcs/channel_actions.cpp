/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channel_actions.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 22:14:39 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/19 17:09:43 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Channel.hpp"

// Mover a Channel.cpp
/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool Channel::isTopicRestricted() const {return (_topicRestricted);}

/**
 * @brief 
 * 
 * @param client 
 * @param params 
 */

void Server::handleTopic(Client *client, const std::vector<std::string> &params)
{
	const std::string srv = ":" + _serverName + " ";
	const std::string nick = client->getNick();   // ya está registrado (lo filtra el dispatcher)
	// Sin parámetros o primero vacío
	if (params.empty() || params[0].empty())
	{
		client->queueMessage(srv + "461 " + nick + " TOPIC :Not enough parameters\r\n");
		return;
	}
	const std::string &consultedCh = params[0];
	// Ver si es un canal y si el usuario pertenece a él
	Channel *channel;
	std::string lowered(consultedCh);
	for (size_t j = 0; j < lowered.size(); ++j)
		lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));
	// Si no tiene sintaxis de canal, CHAOOOO
	if (consultedCh[0] != '#')
	{
		client->queueMessage(srv + "403 " + nick + " " + consultedCh + " :No such channel\r\n");
		return;
	}
	else
	{
		channel = findChannel(lowered);
		// Si no es un canal, CHAOOOO
		if (!channel)
		{
			client->queueMessage(srv + "403 " + nick + " " + consultedCh + " :No such channel\r\n");
			return;
		}
	}
	// Si el usuario no pertenece al canal, CHAOOOO
	if (channel->isMember(client) == false)
	{
		client->queueMessage(srv + "442 " + nick + " " + channel->getChName() + " :You're not on that channel\r\n"); // No creo que este mensaje sea correcto
		return;
		}
	// Si solo hay 1 parámetro, entonces es consulta del topic
	if (params.size() == 1)
	{
		// Si topic está vacío
		if (channel->getChTopic().empty())
			client->queueMessage(srv + "331 " + nick + " " + channel->getChName() + " :No topic is set\r\n");
		// Si hay topic, mandárselo al usuario
		else
			client->queueMessage(srv + "332 " + nick + " " + channel->getChName() + " :" + channel->getChTopic() + "\r\n");
		return;
	}
	// Si hay más de 1, entonces es modificación con el segundo
	// Pensaba que si hay más de 2 parámetros, tiene que dar error, pero simplemente se omiten los demás
	/*if (params.size() > 2)
	{
		client->queueMessage(srv + "CODE " + nick + " :MESSAGE\r\n");
		return;
	}*/
	const std::string &newTopic = params[1];
	// Ver si el modo topic está bloqueado y el usuario no es operador para dar error y salir
	if (channel->isTopicRestricted() == true && channel->isOperator(client) == false)
	{
		client->queueMessage(srv + "482 " + nick + " " + channel->getChName() + " :You're not channel operator\r\n");
		return;
	}
	// Reemplazar topic
	channel->setChTopic(newTopic);
	// Preparamos mensaje para los usuarios del canal
	const std::string msg = ":" + client->getPrefix() + " TOPIC " + channel->getChName() + " :" + newTopic + "\r\n";
	// Enviamos mensaje a los usuarios del canal
	const std::map<int, Client *> &users = channel->getChUsers();
	for (std::map<int, Client *>::const_iterator userIterator = users.begin(); userIterator != users.end(); ++userIterator)
	{
		if (userIterator->second->shouldClose())     // Se está cerrando
			continue;
		userIterator->second->queueMessage(msg);
	}
}

// Mover a Channel.cpp
/**
 * @brief 
 * 
 * @param client 
 */
void Channel::addInvite(Client *client)
{
    if (!client)
        return;
    _chInvited.insert(client);
}

void Server::handleInvite(Client *client, const std::vector<std::string> &params)
{
	const std::string srv = ":" + _serverName + " ";
	const std::string nick = client->getNick();   // ya está registrado (lo filtra el dispatcher)
	// Sin parámetros o primero vacío
	if (params.empty() || params[0].empty())
	{
		client->queueMessage(srv + "461 " + nick + " INVITE :Not enough parameters\r\n");
		return;
	}
	// Si no existe el invitado
	const std::string &invitedNickName = params[0];
	Client *invitedNick = findByNick(invitedNickName);
	if (!invitedNick)
	{
		client->queueMessage(srv + "401 " + nick + " " + invitedNickName + " :No such nick/channel\r\n");
		return;
	}
	// Parámetros de más o channel vacío
	if (params.size() != 2 || params[1].empty())
	{
		client->queueMessage(srv + "CODE " + nick + " :MESSAGE\r\n");
		return;
	}
		const std::string &invitationCh = params[1];
	// Ver si es un canal y si el usuario pertenece a él
	Channel *channel;
	std::string lowered(invitationCh);
	for (size_t j = 0; j < lowered.size(); ++j)
		lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));
	// Si no tiene sintaxis de canal, CHAOOOO
	if (invitationCh[1] != '#')
	{
		client->queueMessage(srv + "403 " + nick + " " + invitationCh + " :No such channel\r\n");
		return;
	}
	else
	{
		channel = findChannel(invitationCh);
		// Si no es un canal, CHAOOOO
		if (!channel)
		{
			client->queueMessage(srv + "403 " + nick + " " + invitationCh + " :No such channel\r\n");
			return;
		}
	}
	// Si el usuario no pertenece al canal, CHAOOOO
	if (channel->isMember(client) == false)
	{
		client->queueMessage(srv + "402 " + nick + " " + channel->getChName() + " :You're not on that channel\r\n"); // No creo que este mensaje sea correcto
		return;
	}
	// Ver si el usuario es operador del canal
	if (channel->isInviteOnly() == true && channel->isOperator(client) == false)
	{
		client->queueMessage(srv + "482 " + nick + " " + channel->getChName() + " :You're not channel operator\r\n"); // No creo que este mensaje sea correcto
		return;
	}
	// Ver si el invitado pertenece ya al canal o ya estaba invitado no hacer nada
	if (channel->isMember(invitedNick)) // Revisar qué hacer cuando channel->isInvited(invitedNick) == true
	{
		client->queueMessage(srv + "442 " + nick + " " + channel->getChName() + " :is already on channel\r\n"); // No creo que este mensaje sea correcto
		return;
	}
	channel->addInvite(invitedNick);
	client->queueMessage(srv + "CODE " + nick + " :MESSAGE\r\n");
	invitedNick->queueMessage(srv + "CODE " + nick + " :MESSAGE\r\n");
}
