/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   invite.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 22:14:39 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/21 16:29:23 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "Channel.hpp"


void Server::handleInvite(Client *client, const std::vector<std::string> &params)
{
	const std::string srv = ":" + _serverName + " ";
	const std::string nick = client->getNick();

	if (params.empty() || params.size() < 2 || params[0].empty() || params[1].empty())
	{
		client->queueMessage(srv + "461 " + nick + " INVITE :Not enough parameters\r\n");
		return;
	}

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
	Channel *channel;
	lowered = invitationCh;
	for (size_t j = 0; j < lowered.size(); ++j)
		lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));

	if (invitationCh[0] != '#' && invitationCh[0] != '&')
	{
		client->queueMessage(srv + "403 " + nick + " " + invitationCh + " :No such channel\r\n");
		return;
	}
	else
	{
		channel = findChannel(lowered);
		if (!channel)
		{
			client->queueMessage(srv + "403 " + nick + " " + invitationCh + " :No such channel\r\n");
			return;
		}
	}

	if (channel->isMember(client) == false)
	{
		client->queueMessage(srv + "442 " + nick + " " + channel->getChName() + " :You're not on that channel\r\n");
		return;
	}

	if (channel->isMember(invitedNick) == true)
	{
		client->queueMessage(srv + "443 " + nick + " " + invitedNick->getNick() + " " + channel->getChName() + " :is already on channel\r\n");
		return;
	}

	if (channel->isInviteOnly() == true && channel->isOperator(client) == false)
	{
		client->queueMessage(srv + "482 " + nick + " " + channel->getChName() + " :You're not channel operator\r\n");
		return;
	}

	channel->addInvite(invitedNick);
	client->queueMessage(srv + "341 " + nick + " " + invitedNick->getNick() + " " + invitationCh + "\r\n");
	invitedNick->queueMessage(":" + client->getPrefix() + " INVITE "+ invitedNick->getNick() + " :" + invitationCh + "\r\n");
}
