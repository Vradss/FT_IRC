/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   quit.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 22:31:32 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:41:05 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"


static const size_t QUIT_MAX_REASON = 300;

void Server::handleQuit(Client *client, const std::vector<std::string> &params)
{
    std::string reason = "Client Quit";

    if (!params.empty() && !params[0].empty())
    {
        std::string text = params[0];
        if (text.size() > QUIT_MAX_REASON)
            text = text.substr(0, QUIT_MAX_REASON);
        reason = "Quit: " + text;
    }

    client->setQuitReason(reason);
    disconnect(*client, "quit");
}

