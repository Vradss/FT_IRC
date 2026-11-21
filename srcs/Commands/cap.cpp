/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cap.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/18 23:03:15 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:29:49 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../includes/Server.hpp"

static const size_t CAP_MAX_REQ = 400;

std::string toUpperStr(const std::string &s)
{
    std::string result(s);
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
    return result;
}

void Server::handleCap(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";

    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    const std::string target = client->isRegistered() ? client->getNick() : "*";

    if (params.empty() || params[0].empty())
    {
        client->queueMessage(srv + "461 " + nick + " CAP :Not enough parameters\r\n");
        return;
    }

    const std::string sub = toUpperStr(params[0]);

    if (sub == "LS")
    {
        client->queueMessage(srv + "CAP " + target + " LS :\r\n");
    }
    else if (sub == "LIST")
    {
        client->queueMessage(srv + "CAP " + target + " LIST :\r\n");
    }
    else if (sub == "REQ")
    {
        if (params.size() < 2)
        {
            client->queueMessage(srv + "461 " + nick + " CAP :Not enough parameters\r\n");
            return;
        }
        std::string requested = params[1];
        if (requested.size() > CAP_MAX_REQ)
            requested = requested.substr(0, CAP_MAX_REQ);
        client->queueMessage(srv + "CAP " + target + " NAK :" + requested + "\r\n");
    }
    else if (sub == "END")
    {
    }
    else
    {
        client->queueMessage(srv + "410 " + nick + " " + params[0] + " :Invalid CAP command\r\n");
    }
}