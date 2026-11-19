/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kick.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 21:20:21 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/19 21:31:51 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"

static const size_t KICK_MAX_COMMENT = 300;

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


/*
KICK #Finnish John :Speaking English 

   Only a channel operator may kick another user out of a  channel.
   Each  server that  receives  a KICK message checks that it is valid
   (ie the sender is actually a  channel  operator)  before  removing
   the  victim  from  the channel.
    SOLO LO DESCONECTA NO LO SACA DEL CANAL COMO TAL, realmente el expulsado puede volver a entrar cuando quiera
*/

    //esto no deja de ser un PART forzado, provocado por otro 
    // la persona que lo hace, solo puede ser operadora 
    //el resto es más o menos igual, lo único que tendré que comprobar que quien hace el KICK es operador
    // el resto: comprobar que el canal existe, que el user es miembro + operador del canal 
    // lanzamos mensaje
    // le quitamos del canal con la función leaveChannel que hice antes
    // control de errores
    // si me expulso a mi mismo, y el canal queda vacio por ejemplo pues tengo que borrar el canal (duda de si permito expulsarme o no)
    // comprobar si hay alguna invitación por ahi pendiente por si la hay borrarla y que no pueda volver a acceder (creo que esto no es neceario)

void Server::handleKick(Client *client, const std::vector<std::string> &params)
{

    const std::string srv = ":" + _serverName + " ";
    const std::string nick = client->getNick();  

    // hacen falta canal y nick 
    if (params.size() < 2 || params[0].empty() || params[1].empty())
    {
        client->queueMessage(srv + "461 " + nick + " KICK :Not enough parameters\r\n");
        return;
    }

    //  El canal no existe
    Channel *channel = findChannel(params[0]);
    if (!channel)
    {
        client->queueMessage(srv + "403 " + nick + " " + params[0] + " :No such channel\r\n");
        return;
    }

    // Quien expulsa está dentro: 442 
    //(esto lo hacemos antes de ver si es operador porq me da igual que sea operador si no es de ese canal)
    //tmb para no revelar a una persona de fuera si x está en el canal 
    if (!channel->isMember(client))
    {
        client->queueMessage(srv + "442 " + nick + " " + channel->getChName()
            + " :You're not on that channel\r\n");
        return;
    }

    // miramos si es operado (ANTES de mirar al user a echar,
    // para no revelar a un no-operador quién está en el canal)
    
    if (!channel->isOperator(client))
    {
        client->queueMessage(srv + "482 " + nick + " " + channel->getChName()
            + " :You're not channel operator\r\n");
        return;
    }

    // motivo opcional
    std::string comment = nick;
    if (params.size() > 2 && !params[2].empty())
    {
        comment = params[2];
        if (comment.size() > KICK_MAX_COMMENT)
            comment = comment.substr(0, KICK_MAX_COMMENT);
    }

    // coger el nombre del canal antes de hacer nada con él por si luego se borra o lo que sea 
    const std::string chName = channel->getChName();

    const std::vector<std::string> usersToKick = splitByComma(params[1]);
    std::set<std::string> usersKicked; //para los duplicados

    for (size_t i = 0; i < usersToKick.size(); ++i)
    {
        if (usersToKick[i].empty())
            continue;
        if (!usersKicked.insert(toLower(usersToKick[i])).second) //si estan en la lista los añadimos como ya gestionados
            continue;

        // checkar si el user existe y está en el canal 
        Client *target = findByNick(usersToKick[i]);
        if (!target || !channel->isMember(target))
        {
            client->queueMessage(srv + "441 " + nick + " " + usersToKick[i] + " " + chName
                + " :They aren't on that channel\r\n");
            continue;
        }

        //hacemos el aviso, hay que hacerlo antes de expuslar al cliente pq si no luego no lo recibe
        // Prefijo = quien expulsa; nick = el REAL del expulsado.
        const std::string msg = ":" + client->getPrefix() + " KICK " + chName + " "
            + target->getNick() + " :" + comment + "\r\n";
        //encolar mensajes
        const std::map<int, Client *> &users = channel->getChUsers();
        for (std::map<int, Client *>::const_iterator userIt = users.begin(); userIt != users.end(); ++userIt)
            userIt->second->queueMessage(msg);

        // limpiamos con el expulsado!!! NO CON EL CLIENT el client es quien echa
        // KICK no desconecta a nadie: solo lo saca del canal.
        leaveChannel(target, channel);

        // channel puede estar liberado: se vuelve a buscar. 
        //Si ya no existe, o quien expulsa ya no está dentro (se expulsó a sí mismo)
        // no tiene sentido seguir con el resto de la lista asi que break 
        channel = findChannel(chName);
        if (!channel || !channel->isMember(client))
            break;
    }
    
}