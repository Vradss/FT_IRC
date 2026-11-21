/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   privmsg.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 22:06:28 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:40:45 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Channel.hpp"


static const size_t PRIVMSG_MAX_TEXT = 370;

Channel *Server::findChannel(const std::string &name)
{
    std::string key(name);
    for (size_t i = 0; i < key.size(); ++i)
        key[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(key[i])));

    std::map<std::string, Channel *>::iterator it = _channels.find(key);
    if (it == _channels.end())
        return NULL;
    return it->second;
}



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


void Server::privateMessage(Client *client, const std::vector<std::string> &params)
{
 
    const std::string srv = ":" + _serverName + " ";
    const std::string nick = client->getNick();

    if (params.empty() || params[0].empty())
    {
        client->queueMessage(srv + "411 " + nick + " :No recipient given (PRIVMSG)\r\n");
        return;
    }

    if (params.size() < 2 || params[1].empty())
    {
        client->queueMessage(srv + "412 " + nick + " :No text to send\r\n");
        return;
    }


    std::string text = params[1];
    if (text.size() > PRIVMSG_MAX_TEXT)
        text = text.substr(0, PRIVMSG_MAX_TEXT);

    const std::vector<std::string> targets = splitByComma(params[0]);
    std::set<std::string> sent;

    
    for (size_t i = 0; i < targets.size(); ++i)
    {
        const std::string &target = targets[i];
        if (target.empty())
            continue;

        std::string lowered(target);
        for (size_t j = 0; j < lowered.size(); ++j)
            lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));
        
        if (!sent.insert(lowered).second) 
            continue;

        if (target[0] == '#' || target[0] == '&')
        {
            Channel *channel = findChannel(target);

            if (!channel)
            {
                client->queueMessage(srv + "403 " + nick + " " + target + " :No such channel\r\n");
                continue;
            }
            if (!channel->isMember(client))
            {
                client->queueMessage(srv + "404 " + nick + " " + channel->getChName() + " :Cannot send to channel\r\n");
                continue;
            }

            const std::string msg = ":" + client->getPrefix() + " PRIVMSG "
                + channel->getChName() + " :" + text + "\r\n";

            const std::map<int, Client *> &users = channel->getChUsers();
            for (std::map<int, Client *>::const_iterator userIterator = users.begin(); userIterator != users.end(); ++userIterator)
            {
                if (userIterator->second == client) 
                    continue;
                if (userIterator->second->shouldClose()) 
                    continue;
                userIterator->second->queueMessage(msg);
            }
        }
        else
        {
            Client *receiver = findByNick(target);
            if (!receiver)
            {
                client->queueMessage(srv + "401 " + nick + " " + target + " :No such nick/channel\r\n");
                continue;
            }

            receiver->queueMessage(":" + client->getPrefix() + " PRIVMSG "
                + receiver->getNick() + " :" + text + "\r\n");
        }
    }

}