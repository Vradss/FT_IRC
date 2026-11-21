/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   register.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/15 18:20:51 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:43:25 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../includes/Server.hpp"
#include "../../includes/Channel.hpp"


static const size_t USERNAME_MAX_LEN = 30;
static const size_t NICK_MAX_LEN = 9;

static bool isValidUsername(const std::string &username)
{
    if (username.empty())
        return false;
    for (size_t i = 0; i < username.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(username[i]);
        if (c <= 32 || c == 127 || c == '@' || c == '!')
            return false;
    }
    return true;
}

void Server::handleUser(Client *client, const std::vector<std::string> &params)
{

    const std::string srv = ":" + _serverName + " ";

    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    if (!client->isAuthenticated())
    {
        client->queueMessage(srv + "451 " + nick + " :You have not registered\r\n");
        return;
    }

    if (params.size() < 4)
    {
        client->queueMessage(srv + "461 " + nick + " USER :Not enough parameters\r\n");
        return;
    }

    if (!client->getUsername().empty())
    {
        client->queueMessage(srv + "462 " + nick + " :You may not reregister\r\n");
        return;
    }

    std::string username = params[0];
    if (!isValidUsername(username))
    {
        client->queueMessage(srv + "461 " + nick + " USER :Invalid username\r\n");
        return;
    }
    if (username.size() > USERNAME_MAX_LEN)
        username = username.substr(0, USERNAME_MAX_LEN);


    client->setUsername(username);
    client->setRealname(params[3]);


    completeRegistration(client);
    
}


static bool isNickSpecial(char c)
{
    return c == '[' || c == ']' || c == '\\' || c == '`'
        || c == '_' || c == '^' || c == '{' || c == '|' || c == '}';
}

static bool isNickLetter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool isValidNick(const std::string &nick)
{
    if (nick.empty() || nick.size() > NICK_MAX_LEN)
        return false;
    if (!isNickLetter(nick[0]) && !isNickSpecial(nick[0]))
        return false;
    for (size_t i = 1; i < nick.size(); ++i)
    {
        char c = nick[i];
        if (!isNickLetter(c) && !isNickSpecial(c)
            && !(c >= '0' && c <= '9') && c != '-')
            return false;
    }
    return true;
}


void Server::completeRegistration(Client *client)
{
    if (!client->isRegistered() || client->hasWelcomed())
        return;
    client->setWelcomed(true);

    const std::string srv = ":" + _serverName + " ";
    const std::string nick = client->getNick();

    client->queueMessage(srv + "001 " + nick + " :Welcome to the Internet Relay Network "
        + client->getPrefix() + "\r\n");
    client->queueMessage(srv + "002 " + nick + " :Your host is " + _serverName
        + ", running version 1.0\r\n");
    client->queueMessage(srv + "003 " + nick + " :This server was created recently\r\n");
    client->queueMessage(srv + "004 " + nick + " " + _serverName + " 1.0 o itkol\r\n");
}


void Server::handleNick(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";
    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    if (!client->isAuthenticated())
    {
        client->queueMessage(srv + "451 " + nick + " :You have not registered\r\n");
        return;
    }

    if (params.empty() || params[0].empty())
    {
        client->queueMessage(srv + "431 " + nick + " :No nickname given\r\n");
        return;
    }
    const std::string &newNick = params[0];

    if (!isValidNick(newNick))
    {
        client->queueMessage(srv + "432 " + nick + " " + newNick + " :Erroneus nickname\r\n");
        return;
    }

    Client *other = findByNick(newNick);
    if (other && other != client)
    {
        client->queueMessage(srv + "433 " + nick + " " + newNick + " :Nickname is already in use\r\n");
        return;
    }

    if (newNick == client->getNick())
        return;

    const bool isOldNickRegistered = client->isRegistered();
    const std::string oldPrefix = client->getPrefix();
    client->setNick(newNick);


    if (isOldNickRegistered)
    {
        const std::string msg = ":" + oldPrefix + " NICK " + newNick + "\r\n";

        std::set<Client *> recipients;
        recipients.insert(client);

        const std::map<std::string, Channel *> &channels = client->getChannels();
        for (std::map<std::string, Channel *>::const_iterator cit = channels.begin();
             cit != channels.end(); ++cit)
        {
            const std::map<int, Client *> &users = cit->second->getChUsers();
            for (std::map<int, Client *>::const_iterator userIterator = users.begin();
                 userIterator != users.end(); ++userIterator)
                recipients.insert(userIterator->second);
        }

        for (std::set<Client *>::iterator recipientIterator = recipients.begin(); recipientIterator != recipients.end(); ++recipientIterator)
            (*recipientIterator)->queueMessage(msg);
    }

    completeRegistration(client);

}

void Server::handlePass(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";

    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    if (params.empty())
    {
        client->queueMessage(srv + "461 " + nick + " PASS :Not enough parameters\r\n");
        return;
    }

    if (client->isRegistered())
    {
        client->queueMessage(srv + "462 " + nick + " :You may not reregister\r\n");
        return;
    }

    if (params[0] != _password)
    {
        client->queueMessage(srv + "464 " + nick + " :Password incorrect\r\n");
        disconnect(*client, "wrong password");
        return;
    }

    client->setAuthenticated(true);
}