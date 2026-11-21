/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mode.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 22:14:39 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/21 16:30:32 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "Channel.hpp"
#include <sstream>


std::string Channel::getChModes() const
{
	std::string modes = "+";
	if (isInviteOnly() == true)
		modes += "i";
	if (isTopicRestricted() == true)
		modes += "t";

	if (getHasChKey() == true)
		modes += "k";
	if (hasUserLimit() == true)
		modes += "l";

	if (getHasChKey() == true)
		modes += " " + getChKey();
	if (hasUserLimit() == true)
	{
		// Pasar el parámetro str a size_t
		std::ostringstream oss;
		oss << getUserLimit();
		modes += " " + oss.str();
	}
	if (modes.size() == 1)
		return ("");
	return (modes);
}


static std::vector<std::string> getActionModes(const std::string &modeString)
{
	bool activeMode = true;
	std::vector<std::string> actionModes;
	std::string::size_type modeStringSize = modeString.size();
	for (std::string::size_type i = 0; i < modeStringSize; ++i)
	{
		switch (modeString[i])
		{
		case '+':
			activeMode = true;
			break;
		case '-':
			activeMode = false;
			break;
		case 'i':
			if (activeMode == true)
				actionModes.push_back("+i");
			else
				actionModes.push_back("-i");
			break;
		case 't':
			if (activeMode == true)
				actionModes.push_back("+t");
			else
				actionModes.push_back("-t");
			break;
		case 'k':
			if (activeMode == true)
				actionModes.push_back("+k");
			else
				actionModes.push_back("-k");
			break;
		case 'l':
			if (activeMode == true)
				actionModes.push_back("+l");
			else
				actionModes.push_back("-l");
			break;
		case 'o':
			if (activeMode == true)
				actionModes.push_back("+o");
			else
				actionModes.push_back("-o");
			break;
		default:
			std::string unknown(1, modeString[i]);
			actionModes.push_back(unknown);
			break;
		}
	}
	return (actionModes);
}

static void notifyModeChange(Channel *channel, Client *source, const std::string &mode, const std::string &parameter)
{
	std::string msg = ":" + source->getPrefix() + " MODE "
		+ channel->getChName() + " " + mode;
	if (parameter.empty() == false)
		msg += " " + parameter;
	msg += "\r\n";

	const std::map<int, Client *> &users = channel->getChUsers();
	for (std::map<int, Client *>::const_iterator it = users.begin(); it != users.end(); ++it)
	{
		if (it->second->shouldClose() == false)
			it->second->queueMessage(msg);
	}
}

static void changeChInviteOnly(Channel *channel, std::string inviteMode, Client *source)
{
	if (inviteMode[0] == '+')
		channel->changeInviteOnly(true);
	else
		channel->changeInviteOnly(false);
	notifyModeChange(channel, source, inviteMode, "");
}

static void changeChTopicRestriction(Channel *channel, std::string inviteMode, Client *source)
{
	if (inviteMode[0] == '+')
		channel->changeTopicRestricted(true);
	else
		channel->changeTopicRestricted(false);
	notifyModeChange(channel, source, inviteMode, "");
}

static void changeChKeywordRequired(Channel *channel, std::string inviteMode,
	std::string modeParameter, Client *source)
{
	if (inviteMode[0] == '+')
	{
		if (modeParameter.empty() == true)
			return; //LANZAR ERROR
		channel->setHasChKey(true);
		channel->setChKey(modeParameter);
	}
	else
	{
		channel->setHasChKey(false);
		channel->setChKey("");
	}
	notifyModeChange(channel, source, inviteMode, modeParameter);
}

static void changeChUserLimit(Channel *channel, std::string inviteMode,
	std::string modeParameter, Client *source, const std::string &srv)
{
	if (inviteMode[0] == '+')
	{
		if (modeParameter.empty() == true)
			return;
		const std::string invalidModeParam = srv + "696 " + source->getNick()
			+ " " + channel->getChName() + " l " + modeParameter
			+ " :Invalid mode parameter\r\n";

		size_t chUserLimit;
		std::string::size_type modeParameterSize = modeParameter.size();
		for (std::string::size_type i = 0; i < modeParameterSize; ++i)
		{
			if (modeParameter[i] < '0' || modeParameter[i] > '9')
			{
				source->queueMessage(invalidModeParam);
				return;
			}
		}

		std::istringstream iss(modeParameter);
		if (!(iss >> chUserLimit) || chUserLimit < 1)
		{
			source->queueMessage(invalidModeParam);
			return;
		}
		channel->setUserLimit(chUserLimit);
	}
	else
		channel->setUserLimit(0);
	notifyModeChange(channel, source, inviteMode, modeParameter);
}

static void changeChOperatorStatus(Channel *channel, std::string inviteMode,
	Client *nominatedNick, Client *source)
{
	if (inviteMode[0] == '+')
	{
		if (channel->addOperator(nominatedNick) == false)
			return;
	}
	else
	{
		if (channel->isOperator(nominatedNick) == false)
			return; // LANZAR ERROR
		channel->removeOperator(nominatedNick);
	}
	notifyModeChange(channel, source, inviteMode, nominatedNick->getNick());
}


void Server::handleMode(Client *client, const std::vector<std::string> &params)
{
	const std::string srv = ":" + _serverName + " ";
	const std::string nick = client->getNick();

	std::string::size_type idx = 0;
	if (params.empty() || params[idx].empty())
	{
		client->queueMessage(srv + "461 " + nick + " MODE :Not enough parameters\r\n");
		return;
	}
	const std::string &consultedCh = params[idx++];

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

	std::vector<std::string>::size_type paramsNum = params.size();
	if (paramsNum == 1)
	{
		client->queueMessage(srv + "324 " + nick + " " + channel->getChName() + " " + channel->getChModes() + "\r\n");
		return;
	}

	if (channel->isMember(client) == false)
	{
		client->queueMessage(srv + "442 " + nick + " " + channel->getChName() + " :You're not on that channel\r\n");
		return;
	}
	if (channel->isOperator(client) == false)
	{
		client->queueMessage(srv + "482 " + nick + " " + channel->getChName() + " :You're not channel operator\r\n");
		return;
	}
	if (paramsNum == 2 && params[idx].empty() == true)
	{
		client->queueMessage(srv + "461 " + nick + " MODE :Not enough parameters\r\n");
		return;
	}
	std::vector<std::string> actionModes = getActionModes(params[idx]);
	std::string modeParameter;
	std::vector<std::string>::const_iterator it = actionModes.begin();
	if (actionModes.empty() == true || it == actionModes.end())
	{
		client->queueMessage(srv + "461 " + nick + " MODE :Not enough parameters\r\n");
		return;
	}
	for (; it != actionModes.end(); ++it)
	{
		if ((*it).empty())
			continue;
		else if (*it == "+i" || *it == "-i")
			changeChInviteOnly(channel, *it, client);
		else if (*it == "+t" || *it == "-t")
			changeChTopicRestriction(channel, *it, client);
		else if (*it == "-k")
			changeChKeywordRequired(channel, *it, "", client);
		else if (*it == "-l")
			changeChUserLimit(channel, *it, "", client, srv);
		else if (*it == "+k" || *it == "+l" || *it == "+o" || *it == "-o")
			++idx;
		else
		{
			client->queueMessage(srv + "472 " + nick + " " + *it + " :is unknown mode char to me\r\n");
			continue;
		}
			
		if (idx < paramsNum)
			modeParameter = params[idx];
		else
			modeParameter = "";	
		if ((*it == "+k" || *it == "+l" || *it == "+o" || *it == "-o")
			&& modeParameter.empty() == true)
		{
			client->queueMessage(srv + "461 " + nick + " MODE :Not enough parameters\r\n");
			return;
		}
		
		if (*it == "+k")
			changeChKeywordRequired(channel, *it, modeParameter, client);
		if (*it == "+l")
			changeChUserLimit(channel, *it, modeParameter, client, srv);
		if (*it == "+o" || *it == "-o")
		{
			std::string lowered(modeParameter);
			for (size_t j = 0; j < lowered.size(); ++j)
				lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));
			Client *nominatedNick = findByNick(lowered);
			if (!nominatedNick)
			{
				client->queueMessage(srv + "401 " + nick + " " + modeParameter + " :No such nick/channel\r\n");
				return;
			}

			if (channel->isMember(nominatedNick) == false)
			{
				client->queueMessage(srv + "441 " + nick + " " + modeParameter + " " + channel->getChName() + " :They aren't on that channel\r\n");
				return;
			}
			changeChOperatorStatus(channel, *it, nominatedNick, client);
		}
	}
}
