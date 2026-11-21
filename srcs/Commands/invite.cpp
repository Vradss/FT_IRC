/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   invite.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 22:14:39 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/20 03:01:52 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "Channel.hpp"

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
