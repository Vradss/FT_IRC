/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kick.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 21:20:21 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:34:24 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"

static const size_t KICK_MAX_COMMENT = 300;

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


void Server::handleKick(Client *client, const std::vector<std::string> &params)
{

    const std::string srv = ":" + _serverName + " ";
    const std::string nick = client->getNick();  


    if (params.size() < 2 || params[0].empty() || params[1].empty())
    {
        client->queueMessage(srv + "461 " + nick + " KICK :Not enough parameters\r\n");
        return;
    }


    Channel *channel = findChannel(params[0]);
    if (!channel)
    {
        client->queueMessage(srv + "403 " + nick + " " + params[0] + " :No such channel\r\n");
        return;
    }


    if (!channel->isMember(client))
    {
        client->queueMessage(srv + "442 " + nick + " " + channel->getChName()
            + " :You're not on that channel\r\n");
        return;
    }

    
    if (!channel->isOperator(client))
    {
        client->queueMessage(srv + "482 " + nick + " " + channel->getChName()
            + " :You're not channel operator\r\n");
        return;
    }


    std::string comment = nick;
    if (params.size() > 2 && !params[2].empty())
    {
        comment = params[2];
        if (comment.size() > KICK_MAX_COMMENT)
            comment = comment.substr(0, KICK_MAX_COMMENT);
    }

    const std::string chName = channel->getChName();

    const std::vector<std::string> usersToKick = splitByComma(params[1]);
    std::set<std::string> usersKicked;

    for (size_t i = 0; i < usersToKick.size(); ++i)
    {
        if (usersToKick[i].empty())
            continue;
        if (!usersKicked.insert(toLower(usersToKick[i])).second)
            continue;


        Client *target = findByNick(usersToKick[i]);
        if (!target || !channel->isMember(target))
        {
            client->queueMessage(srv + "441 " + nick + " " + usersToKick[i] + " " + chName
                + " :They aren't on that channel\r\n");
            continue;
        }

        const std::string msg = ":" + client->getPrefix() + " KICK " + chName + " "
            + target->getNick() + " :" + comment + "\r\n";

        const std::map<int, Client *> &users = channel->getChUsers();
        for (std::map<int, Client *>::const_iterator userIt = users.begin(); userIt != users.end(); ++userIt)
            userIt->second->queueMessage(msg);

        leaveChannel(target, channel);

        channel = findChannel(chName);
        if (!channel || !channel->isMember(client))
            break;
    }
    
}