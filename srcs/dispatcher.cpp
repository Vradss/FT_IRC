/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dispatcher.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/15 12:02:18 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 14:51:16 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Client.hpp"

/*
 * TODO (ver con César porq esto no lo estoy haciendo) 
 * El dispatcher procesa una línea ya parseada y no sabe nada del buffer
 * Quien lee del socket debe:
 *   - Acumular lo recibido en el buffer de entrada del cliente (TCP es un
 *     flujo de bytes, no de mensajes: un recv puede traer medio comando o varios).
 *   - Mientras haya líneas COMPLETAS (hasta \r\n): parsear y llamar a dispatcher.
 *   - Dejar en el buffer lo que sobre sin terminar para el siguiente recv.
 *   - Dejar de procesar líneas si el cliente queda marcado "a desconectar".
 *   - Rodear cada llamada a dispatcher con try/catch de std::bad_alloc
 *     (una sola captura aquí, no una en cada handler).
 *   - Borrar a los clientes marcados al terminar de procesar el buffer,
 *     nunca dentro de un handler.
 */

enum CommandId
{
    CMD_UNKNOWN,
    CMD_JOIN,
    CMD_PASS,
    CMD_NICK,
    CMD_USER,
    CMD_INVITE,
    CMD_TOPIC,
    CMD_KICK,
    CMD_PRIVMSG,
    CMD_PART,
    CMD_QUIT,
    CMD_PING,
    CMD_PONG,
    CMD_CAP,
	CMD_MODE
};

struct CommandEntry
{
    const char *name;
    CommandId   id;
};


static const CommandEntry g_commands[] =
{
    { "JOIN",    CMD_JOIN },
    { "PASS",    CMD_PASS },
    { "NICK",    CMD_NICK },
    { "USER",    CMD_USER },
    { "INVITE",  CMD_INVITE },
    { "TOPIC",   CMD_TOPIC },
    { "KICK",    CMD_KICK },
    { "PRIVMSG", CMD_PRIVMSG },
    { "PART",    CMD_PART },
    { "QUIT",    CMD_QUIT },
    { "PING",    CMD_PING },
    { "PONG",    CMD_PONG },
    { "CAP",     CMD_CAP },
	{ "MODE",    CMD_MODE }
};

static CommandId findCommand(const std::string &cmd)
{
    const size_t count = sizeof(g_commands) / sizeof(g_commands[0]);
    for (size_t i = 0; i < count; ++i)
    {
        if (cmd == g_commands[i].name)
            return g_commands[i].id;
    }
    return CMD_UNKNOWN;
}

static bool isPreRegistrationCommand(const std::string &cmd)
{
    return cmd == "PASS" || cmd == "NICK" || cmd == "USER"
        || cmd == "CAP" || cmd == "PING" || cmd == "PONG" || cmd == "QUIT";
}



void Server::dispatcher(Client *client, const std::string &command, const std::vector<std::string> &params)
{

    if (!client)
        return;


    if (client->isMarkedForDisconnect())
        return;


    if (command.empty())
        return;


    const std::string &cmd = command;


    const std::string srv = ":" + _serverName + " ";
    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    
    if (!client->isRegistered() && !isPreRegistrationCommand(cmd))
    {
        client->queueMessage(srv + "451 " + nick + " :You have not registered\r\n");
        return;
    }

    switch (findCommand(cmd))
    {
        case CMD_JOIN:
            handleJoin(client, params);
            break;
        case CMD_PASS:
            handlePass(client, params);
            break;
        case CMD_NICK:
            handleNick(client, params);
            break;
        case CMD_USER:
            handleUser(client, params);
            break;
        case CMD_INVITE:
            handleInvite(client, params);
            break;
        case CMD_TOPIC:
            handleTopic(client, params);
            break;
        case CMD_KICK:
            handleKick(client, params);
            break;
        case CMD_PRIVMSG:
            privateMessage(client, params);
            break;
        case CMD_PART:
            handlePart(client, params);
            break;
        case CMD_QUIT:
            handleQuit(client, params);
            break;
        case CMD_PING:
            handlePing(client, params);
            break;
        case CMD_PONG:
            handlePong(client, params);
            break;
        case CMD_CAP:
            handleCap(client, params);
            break;
        case CMD_MODE:
            handleMode(client, params);
            break;
        case CMD_UNKNOWN:
        default:
            client->queueMessage(srv + "421 " + nick + " " + command + " :Unknown command\r\n");
            break;
}

}