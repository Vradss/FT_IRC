/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   join_utils.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/14 11:47:50 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:31:50 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include <vector>
#include <string>

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


void Server::handleJoin(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";


    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    if (params.empty())
    {
        client->queueMessage(srv + "461 " + nick + " JOIN :Not enough parameters\r\n");
        return;
    }

    if (params[0] == "0")
    {
        std::string list;
        const std::map<std::string, Channel *> &mine = client->getChannels();
        for (std::map<std::string, Channel *>::const_iterator it = mine.begin();
             it != mine.end(); ++it)
        {
            if (!list.empty())
                list += ",";
            list += it->second->getChName(); 
        }

        if (list.empty())  
            return;

        std::vector<std::string> partParams;
        partParams.push_back(list);
        handlePart(client, partParams);
        return;
    }

    const std::vector<std::string> channels = splitByComma(params[0]);
    std::vector<std::string> keys;
    if (params.size() > 1)  
        keys = splitByComma(params[1]);

    for (size_t i = 0; i < channels.size(); ++i)
    {
      
        if (channels[i].empty())
            continue;

        std::string key;
        if (i < keys.size())
            key = keys[i];

        int result = joinChannel(client, channels[i], key);


        switch (result)
        {
            case 403:
                client->queueMessage(srv + "403 " + nick + " " + channels[i] + " :No such channel\r\n");
                break;
            case 471:
                client->queueMessage(srv + "471 " + nick + " " + channels[i] + " :Cannot join channel (+l)\r\n");
                break;
            case 473:
                client->queueMessage(srv + "473 " + nick + " " + channels[i] + " :Cannot join channel (+i)\r\n");
                break;
            case 475:
                client->queueMessage(srv + "475 " + nick + " " + channels[i] + " :Cannot join channel (+k)\r\n");
                break;
            default:
                break;
        }
    }
}

