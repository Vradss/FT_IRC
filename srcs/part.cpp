/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   part.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 19:53:20 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/17 22:09:57 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include <cctype>

static const size_t PART_MAX_REASON = 300;

//TO-DO: ponerla en un fichero de utilidades y borrar esto de aqui y de join_utils.cpp
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


void Server::leaveChannel(Client *client, Channel *channel)
{
    //buscamos el canal 
    const std::string chKey = toLower(channel->getChName());

    channel->removeUser(client);
    channel->removeOperator(client);
    channel->removeInvite(client);
    client->removeChannel(chKey);

    if (channel->isEmpty())
    {
        _channels.erase(chKey); // primero la quitamos del mapa 
        delete channel; // y luego se libera la memoria, no usar mas
    }
}

//comprobamos que vienen los parametros 
//minimo un canal, si no no funciona 
//tenemos que ver si queda más gente en el canal
//si no queda más nadie, ¿borramos el canal? 
//si queda más gente y quien se va es operador hay que borrarlo de la lista de operadores
// si no quedan mas operadores pues ya está pero en ese canal no se podrá ya hacer "nada"
//le quitamos de la lista de invitados? 
//CUIDARSE DE LOS LEAKS !!!! ESTAMOS BORRANDO COSAS 
//mandamos los mensajes a los canales ANTES DE QUITAR AL CLIENTE DEL CANAL y el mensaje lo mandamos aunque no quede nadie en el canal
//lanzar errores adecuados 
//puede venir una lista de canales 
//importante el canal será el que tengamos guardado no el que haya introducido el user
//quitar el canal de la lista del cliente 
void Server::handlePart(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";
    const std::string nick = client->getNick();

    //no hay canal
    if (params.empty() || params[0].empty())
    {
        client->queueMessage(srv + "461 " + nick + " PART :Not enough parameters\r\n");
        return;
    }

    //motivo (es opcional) sin los dos : 
    std::string reason;
    //comprobamos si viene mas de un parametro, si sí es que viene el motivo
    if (params.size() > 1)
    {
        reason = params[1];
        if (reason.size() > PART_MAX_REASON) //esto es el límite de caracteres 
            reason = reason.substr(0, PART_MAX_REASON);
    }

    // pillamos los canales: 
    const std::vector<std::string> chNames = splitByComma(params[0]);
    std::set<std::string> chNamesHandled; //para ignorar los repetidos, por eso usamos set

    //recorrer la lista 
    for (size_t i = 0; i < chNames.size(); ++i)
    {
        if (chNames[i].empty()) //si viene vacio lo ignoramos 
            continue;
        if (!chNamesHandled.insert(toLower(chNames[i])).second) //comprobamos que no está en handled porque si está es que ya lo procesó
            continue;

        //buscamos el canal
        Channel *channel = findChannel(chNames[i]);
        //si no existe => fallo 
        if (!channel) 
        {
            client->queueMessage(srv + "403 " + nick + " " + chNames[i] + " :No such channel\r\n");
            continue;
        }
        //miramos si el user no es miembro o sí 
        if (!channel->isMember(client))
        {
            client->queueMessage(srv + "442 " + nick + " " + channel->getChName()
                + " :You're not on that channel\r\n");
            continue;
        }
        //ya ponemos el aviso, hay que hacerlo antes de que el miembro se vaya del canal porque si el miembro no recibe el mensaje
        //además que puede haber problemas de memoria 
        std::string msg = ":" + client->getPrefix() + " PART " + channel->getChName();
        if (!reason.empty())
            msg += " :" + reason;
        msg += "\r\n";

        const std::map<int, Client *> &users = channel->getChUsers();
        for (std::map<int, Client *>::const_iterator uit = users.begin(); uit != users.end(); ++uit)
            uit->second->queueMessage(msg);

        //lo quitamos del canal 
        leaveChannel(client, channel);
    }
    
}