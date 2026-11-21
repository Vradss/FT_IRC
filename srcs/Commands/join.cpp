/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   join.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 21:59:48 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:33:21 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../includes/Server.hpp"
#include <cctype>
#include <new>

static std::string toLowerStr(const std::string &s)
{
    std::string result(s);
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    return result;
}


static bool isValidChannelName(const std::string &name)
{
    if (name.size() < 2 || name.size() > 50)
        return false;
    if (name[0] != '#' && name[0] != '&')
        return false;
    for (size_t i = 1; i < name.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if (c <= 32 || c == ',' || c == 127)
            return false;
    }
    return true;
}



int Server::joinChannel(Client *client, const std::string &chName, const std::string &key)
{

    if (!isValidChannelName(chName))
        return 403;

    const std::string mapKey = toLowerStr(chName);
    Channel *channel = NULL;


    std::map<std::string, Channel *>::iterator it = _channels.find(mapKey);
    if (it == _channels.end())
    {
  
        try
        {
            channel = new Channel(chName);
            _channels[mapKey] = channel;
        }
        catch (std::bad_alloc &)
        {
            delete channel;
            return -1;
        }

        channel->addUser(client);
        channel->addOperator(client);
    }
    else
    {
        channel = it->second;

        if (channel->isMember(client))
            return 0;

        if (channel->isInviteOnly() && !channel->isInvited(client))
            return 473;
        if (channel->getHasChKey() && channel->getChKey() != key)
            return 475;
        if (channel->hasUserLimit() && channel->getChUsers().size() >= channel->getUserLimit())
            return 471;
        
        channel->removeInvite(client);
        channel->addUser(client);
   
    }

    client->addChannel(mapKey, channel);


    const std::string &name = channel->getChName();
    const std::string &nick = client->getNick();
    const std::string srv = ":" + _serverName + " ";  
    const std::map<int, Client *> &users = channel->getChUsers();


    const std::string joinMsg = ":" + client->getPrefix() + " JOIN " + name + "\r\n"; 
    for (std::map<int, Client *>::const_iterator userIt = users.begin(); userIt != users.end(); ++userIt)
        userIt->second->queueMessage(joinMsg); 

    if (channel->getChTopic().empty())
        client->queueMessage(srv + "331 " + nick + " " + name + " :No topic is set\r\n");
    else
        client->queueMessage(srv + "332 " + nick + " " + name + " :" + channel->getChTopic() + "\r\n");


    const std::string namesPrefix = srv + "353 " + nick + " = " + name + " :";
    std::string line = namesPrefix;
    bool lineHasNames = false;

    for (std::map<int, Client *>::const_iterator userIt = users.begin(); userIt != users.end(); ++userIt)
    {
        std::string entry;
        if (channel->isOperator(userIt->second))
            entry = "@";
        entry += userIt->second->getNick();

        if (lineHasNames && line.size() + 1 + entry.size() + 2 > 510)
        {
            client->queueMessage(line + "\r\n");
            line = namesPrefix;
            lineHasNames = false;
        }
        if (lineHasNames)
            line += " ";
        line += entry;
        lineHasNames = true;
    }
    if (lineHasNames)
        client->queueMessage(line + "\r\n");

    client->queueMessage(srv + "366 " + nick + " " + name + " :End of /NAMES list.\r\n");

    return 0;
}