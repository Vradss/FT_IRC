/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   part.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 19:53:20 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:36:01 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Channel.hpp"
#include <cctype>

static const size_t PART_MAX_REASON = 300;

static std::vector<std::string> splitByComma(const std::string &s)
{
    std::vector<std::string> result;
    std::size_t start = 0;

    while (true)
    {
        std::size_t pos = s.find(',', start);
        if (pos == std::string::npos)
        {
            result.push_back(s.substr(start));
            break;        }
        result.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return result;
}


static std::string toLower(const std::string &s)
{
    std::string result(s);
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    return result;
}


void Server::leaveChannel(Client *client, Channel *channel)
{
    const std::string chKey = toLower(channel->getChName());

    const bool wasOperator = channel->isOperator(client);

    channel->removeUser(client);
    channel->removeOperator(client);
    channel->removeInvite(client);
    client->removeChannel(chKey);

    if (channel->isEmpty())
    {
        _channels.erase(chKey);
        delete channel;
        return;
    }

    if (wasOperator && !channel->hasOperators())
    {
        Client *next = channel->getOldestMember();
        if (next && channel->addOperator(next))
        {
            const std::string msg = ":" + _serverName + " MODE " + channel->getChName()
                + " +o " + next->getNick() + "\r\n";

            const std::map<int, Client *> &users = channel->getChUsers();
            for (std::map<int, Client *>::const_iterator uit = users.begin();
                 uit != users.end(); ++uit)
            {
                if (uit->second->shouldClose())
                    continue;
                uit->second->queueMessage(msg);
            }
        }
    }
}


void Server::handlePart(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";
    const std::string nick = client->getNick();

    if (params.empty() || params[0].empty())
    {
        client->queueMessage(srv + "461 " + nick + " PART :Not enough parameters\r\n");
        return;
    }

    std::string reason;
    if (params.size() > 1)
    {
        reason = params[1];
        if (reason.size() > PART_MAX_REASON) 
            reason = reason.substr(0, PART_MAX_REASON);
    }

    const std::vector<std::string> chNames = splitByComma(params[0]);
    std::set<std::string> chNamesHandled;

    for (size_t i = 0; i < chNames.size(); ++i)
    {
        if (chNames[i].empty()) 
            continue;
        if (!chNamesHandled.insert(toLower(chNames[i])).second)
            continue;

        Channel *channel = findChannel(chNames[i]);
        if (!channel) 
        {
            client->queueMessage(srv + "403 " + nick + " " + chNames[i] + " :No such channel\r\n");
            continue;
        }

        if (!channel->isMember(client))
        {
            client->queueMessage(srv + "442 " + nick + " " + channel->getChName()
                + " :You're not on that channel\r\n");
            continue;
        }

        std::string msg = ":" + client->getPrefix() + " PART " + channel->getChName();
        if (!reason.empty())
            msg += " :" + reason;
        msg += "\r\n";

        const std::map<int, Client *> &users = channel->getChUsers();
        for (std::map<int, Client *>::const_iterator uit = users.begin(); uit != users.end(); ++uit)
            uit->second->queueMessage(msg);

        leaveChannel(client, channel);
    }
    
}