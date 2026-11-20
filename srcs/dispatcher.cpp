/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dispatcher.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/15 12:02:18 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/20 16:16:19 by aumoreno         ###   ########.fr       */
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
    CMD_CAP
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
    { "CAP",     CMD_CAP }
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
        || cmd == "CAP" || cmd == "PING" || cmd == "QUIT";
}



//check esto con César para ver qué me llega 
//esto habrá que llamarlo en el código que lee el socket 
// se acumula en el buffer de entrada 
// en teoria: hay una linea completa, se parsea y se despacha
// repetimos este proceso mientras queden lineas COMPLETAS en el buffer 
// esto es porque en un solo uso de recv pueden vernir varios comandos a la vez 
// esto es porque TCP no tiene un concepto de mensajes o cmds, es un flujo continuo de bytes 
// tonces lo que sobra se queda en el buffer 
/*
    el "problema" con esto es que el cliente puede dejar de existir despues del handler
    Es decir, al venir varios comandos a la vez si primero viene un QUIT o hay un error en alguno de los comandos que 
    pueda borrar al cliente, hay que asegurarse que ni el dispatcher ni el bucle de lectura usen el puntero sin comprobar que sigue vivo
    Ejemplo: si el primer comando desconecta al cliente, los demás no pueden ejecutarse
*/
/*
    para la memoria ver bad_alloc             
*/
void Server::dispatcher(Client *client, const std::string &command, const std::vector<std::string> &params)
{
    //en teoria el nombre del comando ya vendría normalizado
    //ignorar las lineas vacías? 
    //si el cliente no está registrado solo admitimos los comadnos de registro
    //en teoria esto lo checka join pero igual es mejor hacerlo aqui para no tener que hacerlo por cada comando?
    //buscamos ya el comando en concreto y llamamos a su handler correspondiente 
    /*LANZAR LOS ERRORES ADECUADOS*/
    
    //si no hay cliente F 
    if (!client)
        return;

    // Si un comando anterior (QUIT, error grave, KICK...) ya marcó al client como desconectado
    //no se ejecuta nada más de lo que quede en su buffer 
    if (client->isMarkedForDisconnect())
        return;

    // Línea vacía => seguimos 
    if (command.empty())
        return;

    /*ESTO VERLO CON CÉSAR QUE IGUAL LO HACE ÉL */
    //si, ya lo hace cesar en parser.cpp
    //const std::string cmd = toUpperStr(command);
    const std::string &cmd = command;

    //aqui pongo bien el formto y tal para si el user no está registrado 
    // TO-DO: tengo que quitar del joinChannel esto 
    const std::string srv = ":" + _serverName + " ";
    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    // comprubeo aqui si el user no está registrado por 
    //y si no es un comando admitido, rollo intenta hacer JOIN y no hizo NICK 
    // lo hago aqui una vez y no en cada handler

    
    if (!client->isRegistered() && !isPreRegistrationCommand(cmd))
    {
        client->queueMessage(srv + "451 " + nick + " :You have not registered\r\n");
        return;
    }
    
   
    //toda esta parte no sé si la meteré mejor en un switch que se vea más mono 
    // TO-DO: VER SI SE ME HA OLVIDADO ALGUN COMANDO
    // TO-DO: PRIVMESSAGE Y PART
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
        case CMD_UNKNOWN:
        default:
            client->queueMessage(srv + "421 " + nick + " " + command + " :Unknown command\r\n");
            break;
}

}