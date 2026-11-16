/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   privmsg.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 22:06:28 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/16 23:57:57 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Channel.hpp"


static const size_t PRIVMSG_MAX_TEXT = 370; // TO-DO checkear movidas

// para encontrar el canal al que tenemos que mandar el correo 
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

//comprobamos que llegan todos los parámetros que hacen falta
//vemos si se trata de un canal o de un user
//si es un canal:
    //ver si existe el canal
    //ver si somos miembros de ese canal 
//si es un user => comprobar que existe ese user
//ir encolando los mensajes accordingly 
//lanzar los errores adecuados
//en la docu del IRC dice esto:
// puede admitir listas de varios destanatoris
// no mandamos copia al emisor del mensaje en el canal
void Server::privateMessage(Client *client, const std::vector<std::string> &params)
{
 
    const std::string srv = ":" + _serverName + " ";
    const std::string nick = client->getNick();   // ya está registrado (lo filtra el dispatcher)

    // sin destinatario: 411
    if (params.empty() || params[0].empty())
    {
        client->queueMessage(srv + "411 " + nick + " :No recipient given (PRIVMSG)\r\n");
        return;
    }

    // sin texto 412
    if (params.size() < 2 || params[1].empty())
    {
        client->queueMessage(srv + "412 " + nick + " :No text to send\r\n");
        return;
    }

    // El texto se reenvía TAL CUAL (también los mensajes CTCP con \x01) TO-DO duda jorge
    // recorto el mensaje si es largo
    std::string text = params[1];
    if (text.size() > PRIVMSG_MAX_TEXT)
        text = text.substr(0, PRIVMSG_MAX_TEXT);

    const std::vector<std::string> targets = splitByComma(params[0]); //pillamos los destinarios 
    std::set<std::string> sent; // para ir viendo a quién le hemos mandado el mensaje y a quien no 

    
    for (size_t i = 0; i < targets.size(); ++i)
    {
        const std::string &target = targets[i];
        if (target.empty())
            continue;

        // destinatario repetido (sin distinguir mayúsculas): se ignora
        std::string lowered(target);
        //aqui lo paso a miniusculas porque es case insentive y que esté todo igual 
        for (size_t j = 0; j < lowered.size(); ++j)
            lowered[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[j])));
        
        if (!sent.insert(lowered).second) //si ya lo he gestionado, mandado el mensaje, seguimos 
            continue;


        //si es un canal: 
        //TO-DO: añadir las máscaras con & done 
        if (target[0] == '#' || target[0] == '&')
        {
            Channel *channel = findChannel(target);
            //vemos si existe el canal
            if (!channel)
            {
                client->queueMessage(srv + "403 " + nick + " " + target + " :No such channel\r\n");
                continue;
            }
            //vemos si es usuario de miembro del canal 
            if (!channel->isMember(client))
            {
                client->queueMessage(srv + "404 " + nick + " " + channel->getChName() + " :Cannot send to channel\r\n");
                continue;
            }

            // Se usa el nombre GUARDADO en el canal, no el que tecleó el emisor
            // esto es porque los diferentes clientes pueden tener el nombre del canal mostrado de diferentes formas
            //y a la hora de pintarlo, si uso el tecleado por usuario y no es el mismo que el del otro cliente, se verá raro 
            const std::string msg = ":" + client->getPrefix() + " PRIVMSG "
                + channel->getChName() + " :" + text + "\r\n";

            // mandamos los mensajes en cuestion
            const std::map<int, Client *> &users = channel->getChUsers();
            for (std::map<int, Client *>::const_iterator userIterator = users.begin(); userIterator != users.end(); ++userIterator)
            {
                if (userIterator->second == client)          // sin copia al emisor
                    continue;
                if (userIterator->second->shouldClose())     // ya se está cerrando
                    continue;
                userIterator->second->queueMessage(msg);
            }
        }
        else
        {
            // si es un usuario 
            Client *receiver = findByNick(target);
            if (!receiver)
            {
                client->queueMessage(srv + "401 " + nick + " " + target + " :No such nick/channel\r\n");
                continue;
            }

            // Se usa el nick del receptor, no el que tecleó el emisor
            receiver->queueMessage(":" + client->getPrefix() + " PRIVMSG "
                + receiver->getNick() + " :" + text + "\r\n");
        }
    }

}