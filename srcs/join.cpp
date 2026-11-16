/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   join.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 21:59:48 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/16 23:44:17 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../includes/Server.hpp"
#include <cctype>
#include <new>

static std::string toLowerStr(const std::string &s)
{
    std::string result(s);
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    return result;
}

// Empieza por '#', sin espacios, comas ni caracteres de control.
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


/*
 * Códigos de error: 
 *    0 -> todo correcto
 *   -1 -> algo falló pero es rollo mi culpa 
 *  403 -> nombre de canal inválido
 *  471 -> canal lleno (+l)
 *  473 -> canal solo por invitación (+i)
 *  475 -> key incorrecta o ausente (+k)
 *
 * Los errores los encola el handler que llama a esta función.
 * Las respuestas de okay (JOIN, topic, names) se encolan aquí dentro.
 */
int Server::joinChannel(Client *client, const std::string &chName, const std::string &key)
{
    //comprobamos que el nombre es correct (ver con césar)
    if (!isValidChannelName(chName))
        return 403;

    const std::string mapKey = toLowerStr(chName); //voy a usar el nombre de canal como key 
    Channel *channel = NULL;

    //comprobar que el canal existe
    //NOTA: la primera vez que se crea, lo hace sin pass 
    //si llegara una pass, simplemente la ignoramos 
    std::map<std::string, Channel *>::iterator it = _channels.find(mapKey);
    if (it == _channels.end())
    {
        //setearemos al Cliente en cocreto como operator de ese canal
        try
        {
            channel = new Channel(chName); //cuando creamos el channel solo le damos el nombre 
            _channels[mapKey] = channel;
        }
        catch (std::bad_alloc &)
        {
            delete channel; // si falló al insertar en el mapa, no se pierde
            return -1;
        }

        //podemos añadir el usuario
        channel->addUser(client);
        //lo añadimos a la lista de operadores del channel 
        channel->addOperator(client);
    }
    else
    {
        //SI EL CANAL YA EXISTE 
        channel = it->second;

        //comprobamos que el cliente no está ya en el canal 
        if (channel->isMember(client))
            return 0;

        // duda: una invitación se salta +i, +k y +l? NO SE LA SALTA 
        // Tiene que comportarse igual en INVITE
        //en teoria aqui es:
        //si el cliente NO ha sido invitado al canal 
        // +k y +l se comprueban siempre. Tiene que comportarse igual en INVITE.
        if (channel->isInviteOnly() && !channel->isInvited(client))
            return 473;
        if (channel->getHasChKey() && channel->getChKey() != key)
            return 475;
        if (channel->hasUserLimit() && channel->getChUsers().size() >= channel->getUserLimit())
            return 471;
        
        channel->removeInvite(client); //invitación de un solo uso 
        channel->addUser(client);
   
    }

    //añadimos el canal a la lista de canales del cliente 
    client->addChannel(mapKey, channel);


    const std::string &name = channel->getChName();
    const std::string &nick = client->getNick();
    const std::string srv = ":" + _serverName + " "; //TO-DO: HACER GETTERS Y SETTERS 
    const std::map<int, Client *> &users = channel->getChUsers();

    // JOIN a todos los miembros, incluido el que entra
    //esto para luego pintar: :nick!user@host 
    // :ana!ana@host JOIN #canal\r\n 
    // esto es como lo pinta tal cual el protocolo IRC
    const std::string joinMsg = ":" + client->getPrefix() + " JOIN " + name + "\r\n"; //creamos el mensaje
    for (std::map<int, Client *>::const_iterator userIt = users.begin(); userIt != users.end(); ++userIt)
        userIt->second->queueMessage(joinMsg); // vamos recorriendo y encolano el mensaje que vamos a mandar por cada cliente 

    // el topic solo se lo mostramos al que entra
    if (channel->getChTopic().empty())
        client->queueMessage(srv + "331 " + nick + " " + name + " :No topic is set\r\n"); //si no hay topic aun 
    else
        client->queueMessage(srv + "332 " + nick + " " + name + " :" + channel->getChTopic() + "\r\n");

    // pintamos la Lista de nombres (353), partida en varias líneas si se pasa de 512 bytes
    //debemos incluir al nuevo miembro 
    const std::string namesPrefix = srv + "353 " + nick + " = " + name + " :";
    std::string line = namesPrefix;
    bool lineHasNames = false;

    for (std::map<int, Client *>::const_iterator userIt = users.begin(); userIt != users.end(); ++userIt)
    {
        std::string entry;
        if (channel->isOperator(userIt->second))
            entry = "@";
        entry += userIt->second->getNick();

        // +1 por el espacio separador, +2 por el \r\n final
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

    // Fin de la lista
    client->queueMessage(srv + "366 " + nick + " " + name + " :End of /NAMES list.\r\n");

    return 0;
}