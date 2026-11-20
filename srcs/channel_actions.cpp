/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channel_actions.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 22:14:39 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/19 21:22:19 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Channel.hpp"

// Mover a Channel.cpp
/**
 * @brief Indica si solo los operadores pueden modificar el topic del canal.
 * @return true si el modo de topic restringido está activo; false en caso
 * contrario.
 */
bool Channel::isTopicRestricted() const {return (_topicRestricted);}

/**
 * @brief Procesa la consulta o modificación del topic de un canal.
 * 
 * Con un único parámetro devuelve el topic actual. Con un segundo parámetro
 * intenta sustituirlo y, si procede, comunica el cambio a todos los miembros
 * conectados del canal.
 *
 * @param[in] client Cliente registrado que ejecutó el comando TOPIC.
 * @param[in] params Parámetros analizados: nombre del canal y, opcionalmente,
 * nuevo topic.
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
	if (consultedCh[0] != '#' && consultedCh[0] != '&')
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
 * @brief Registra a un cliente como invitado al canal.
 * @param[in] client Cliente que podrá acceder al canal; los punteros nulos se
 * ignoran y las invitaciones duplicadas no alteran el conjunto.
 */
void Channel::addInvite(Client *client)
{
    if (!client)
        return;
    _chInvited.insert(client);
}

/**
 * @brief Procesa una invitación de un cliente a un canal.
 *
 * Comprueba que el nick y el canal existan, que quien invita pertenezca al
 * canal, que el destinatario aún no sea miembro y, para canales de acceso
 * restringido, que quien invita sea operador. Si tiene éxito, registra la
 * invitación y notifica a ambos clientes.
 *
 * @param[in] client Cliente registrado que ejecutó el comando INVITE.
 * @param[in] params Parámetros analizados: nick del invitado y nombre del canal.
 */
void Server::handleInvite(Client *client, const std::vector<std::string> &params)
{
	const std::string srv = ":" + _serverName + " ";
	const std::string nick = client->getNick();   // ya está registrado (lo filtra el dispatcher)
	// Menos de dos parámetros o alguno vacío... Ignoramos si hay más de 2 parámetros?
	if (params.empty() || params.size() < 2 || params[0].empty() || params[1].empty())
	{
		client->queueMessage(srv + "461 " + nick + " INVITE :Not enough parameters\r\n");
		return;
	}
	// Si no existe el invitado
	std::string lowered(params[0]);
	for (size_t j = 0; j < lowered.size(); ++j)
		lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));
	Client *invitedNick = findByNick(lowered);
	if (!invitedNick)
	{
		client->queueMessage(srv + "401 " + nick + " " + params[0] + " :No such nick/channel\r\n");
		return;
	}
		const std::string &invitationCh = params[1];
	// Ver si es un canal y si el usuario pertenece a él
	Channel *channel;
	lowered = invitationCh;
	for (size_t j = 0; j < lowered.size(); ++j)
		lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));
	// Si no tiene sintaxis de canal, CHAOOOO
	if (invitationCh[0] != '#' && invitationCh[0] != '&')
	{
		client->queueMessage(srv + "403 " + nick + " " + invitationCh + " :No such channel\r\n");
		return;
	}
	else
	{
		channel = findChannel(lowered);
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
		client->queueMessage(srv + "442 " + nick + " " + channel->getChName() + " :You're not on that channel\r\n"); // No creo que este mensaje sea correcto
		return;
	}
	// Ver si el invitado pertenece ya al canal
	// Tiene que ir antes de comprobar si el usuario es operador según convención (no protocolo)
	if (channel->isMember(invitedNick) == true)
	{
		client->queueMessage(srv + "443 " + nick + " " + invitedNick->getNick() + " " + channel->getChName() + " :is already on channel\r\n"); // No creo que este mensaje sea correcto
		return;
	}
	// Ver si el usuario es operador del canal
	if (channel->isInviteOnly() == true && channel->isOperator(client) == false)
	{
		client->queueMessage(srv + "482 " + nick + " " + channel->getChName() + " :You're not channel operator\r\n"); // No creo que este mensaje sea correcto
		return;
	}
	// Esté ya invitado o no, llamamos a addInvite porque set guarda registros únicos
	channel->addInvite(invitedNick);
	client->queueMessage(srv + "341 " + nick + " " + invitedNick->getNick() + " " + invitationCh + "\r\n");
	invitedNick->queueMessage(":" + client->getPrefix() + " INVITE "+ invitedNick->getNick() + " :" + invitationCh + "\r\n");
}

