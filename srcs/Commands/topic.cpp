/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   topic.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 22:14:39 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/21 16:31:32 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "Channel.hpp"

void Server::handleTopic(Client *client, const std::vector<std::string> &params)
{
	const std::string srv = ":" + _serverName + " ";
	const std::string nick = client->getNick(); 

	if (params.empty() || params[0].empty())
	{
		client->queueMessage(srv + "461 " + nick + " TOPIC :Not enough parameters\r\n");
		return;
	}
	const std::string &consultedCh = params[0];

	Channel *channel;
	std::string lowered(consultedCh);
	for (size_t j = 0; j < lowered.size(); ++j)
		lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));

	if (consultedCh[0] != '#' && consultedCh[0] != '&')
	{
		client->queueMessage(srv + "403 " + nick + " " + consultedCh + " :No such channel\r\n");
		return;
	}
	else
	{
		channel = findChannel(lowered);

		if (!channel)
		{
			client->queueMessage(srv + "403 " + nick + " " + consultedCh + " :No such channel\r\n");
			return;
		}
	}

	if (channel->isMember(client) == false)
	{
		client->queueMessage(srv + "442 " + nick + " " + channel->getChName() + " :You're not on that channel\r\n");
		return;
	}

	if (params.size() == 1)
	{

		if (channel->getChTopic().empty())
			client->queueMessage(srv + "331 " + nick + " " + channel->getChName() + " :No topic is set\r\n");

		else
			client->queueMessage(srv + "332 " + nick + " " + channel->getChName() + " :" + channel->getChTopic() + "\r\n");
		return;
	}


	const std::string &newTopic = params[1];

	if (channel->isTopicRestricted() == true && channel->isOperator(client) == false)
	{
		client->queueMessage(srv + "482 " + nick + " " + channel->getChName() + " :You're not channel operator\r\n");
		return;
	}

	channel->setChTopic(newTopic);

	const std::string msg = ":" + client->getPrefix() + " TOPIC " + channel->getChName() + " :" + newTopic + "\r\n";

	const std::map<int, Client *> &users = channel->getChUsers();
	for (std::map<int, Client *>::const_iterator userIterator = users.begin(); userIterator != users.end(); ++userIterator)
	{
		if (userIterator->second->shouldClose())   
			continue;
		userIterator->second->queueMessage(msg);
	}
}
