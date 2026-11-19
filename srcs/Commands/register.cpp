/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   register.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/15 18:20:51 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/19 21:32:15 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
    En este clase voy a tener los handle de pass, nick y user 
*/

#include "../../includes/Server.hpp"
#include "../../includes/Channel.hpp"

// máximo de caracteres del username. Si es más largo los truncamos TO-DO HACER ESTO (irssi usa
// por defecto el nombre de usuario del sistema, que puede ser largo)
static const size_t USERNAME_MAX_LEN = 30;
static const size_t NICK_MAX_LEN = 9;

//checkeamos el usrename para que no haya caracteres que rompan nick!user@host 
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


//más o menos las mismas comprobaciones que con NICK quitando lo propio de este comando
//los usernames no son únicos y no se pueden editar 
//luego hacer lo mismo de la bienvenida si procede 
//el user tiene parámetros: USER <user> <mode> <unused> <realname> que habrá que gestionarlos 

//si está todo correcto, lo guardamos
/*LANZAR LOS ERRORES */

void Server::handleUser(Client *client, const std::vector<std::string> &params)
{

    const std::string srv = ":" + _serverName + " ";

    // USER <MODO> <> :realname
    // USER puede llegar antes que NICK: el nick puede estar vacío -> "*"
    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    //Sin contraseña correcta, esto igual se puede omitir pero bueno TO-DO 
    if (!client->isAuthenticated())
    {
        client->queueMessage(srv + "451 " + nick + " :You have not registered\r\n");
        return;
    }

    // comprobamos que están todos los parámetros 
    if (params.size() < 4)
    {
        client->queueMessage(srv + "461 " + nick + " USER :Not enough parameters\r\n");
        return;
    }

    // miramos si ya antes se habia puesto un username porque una vez puesto, no se puede cambiar
    // además no son únicos o sea esto me parece que tiene 0 sentido pero no me voy a quejar 
    if (!client->getUsername().empty())
    {
        client->queueMessage(srv + "462 " + nick + " :You may not reregister\r\n");
        return;
    }

    // validamos el username 
    std::string username = params[0];
    if (!isValidUsername(username))
    {
        client->queueMessage(srv + "461 " + nick + " USER :Invalid username\r\n");
        return;
    }
    if (username.size() > USERNAME_MAX_LEN)
        username = username.substr(0, USERNAME_MAX_LEN);

    // params[1] (modo) y params[2] (sin uso) se ignoran.
    // el realname se guarda tal cual, sin los ':' del protocolo TO-DO: checkear esto con césar 
    client->setUsername(username);
    client->setRealname(params[3]);

    // si con este USER se completa el registro, bienvenida.
    // Si NICK aún no ha llegado, no hace nada: lo hará NICK cuando llegue.
    completeRegistration(client);
    
}


//si no está el usuario autenticado, no podemos cambiar el nick, será lo primero a mirar entiendo

//mirar si viene vacio tal tal 
//miramos formato
// NOTA: hay que tener en cuenta al usuario registrado si cambia el nick 
//si no existe entiendo que debe de cumplir con lo siguiente de acuerdo al IRC:
/*
Each user is distinguished from other users by a unique nickname
having a maximum length of nine (9) characters.  See the protocol
grammar rules (section 2.3.1) for what may and may not be used in a
nickname.
*/
//miramos si el nickname ya existe

// si todo correcto, guardamos el nick
    //hay que almacenarlo tal y como lo guardó el usuario
    //TO-DO: MIRAR SI CASE SENSITIVE

//tendré que avisar la primera vez cuando se haya unido el usuario, darle la bienvenida SOLO SI SE HA COMPLETADO EL REGISTRO
//el resto del tiempo, si solo se cambia nick madnar el mensaje con: :viejo!user@host NICK nuevo 
    //en este caso habrá que avisar a TODOS LOS CANALES en los que se comparta ese nick de que se cambió 
    // IMPORTANTE: montar el prefijo del nick viejo antes de montar el nuevo  
/*LANZAR LOS ERRORES ADECUADOS*/


// Primer carácter: letra o especial. Resto: letra, dígito, especial o '-'
// Quedan fuera espacios, ',', ':', '#', '&', '@', '!' (romperían el prefijo
// nick!user@host o se confundirían con canales)


static bool isNickSpecial(char c)
{
    return c == '[' || c == ']' || c == '\\' || c == '`'
        || c == '_' || c == '^' || c == '{' || c == '|' || c == '}';
}

//TO-DO: esto comprobar que luego lo paso a lower case que no se me olvide 
static bool isNickLetter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool isValidNick(const std::string &nick)
{
    if (nick.empty() || nick.size() > NICK_MAX_LEN) //definir esto a 9 
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

/*
    funcion que simplemente lo que hace es ver si se ha completado el registro
    es decir si el user, pass y nick estan 
*/
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
    // lo de siempre: esto para los mensajitos igual me acabaré haciendo una función jaja
    const std::string srv = ":" + _serverName + " ";
    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    // sin contraseña correcta no se acepta NICK (no acepto ni el primer nick ni si lo cambia pero en teoria esto no deberia de pasar)
    // LO MISMO PARA USER
    //TO-DO: VER SI LO QUITO O LO DEJO 
    if (!client->isAuthenticated())
    {
        client->queueMessage(srv + "451 " + nick + " :You have not registered\r\n");
        return;
    }

    // sin parámetro: 431 (no es un 461)
    if (params.empty() || params[0].empty())
    {
        client->queueMessage(srv + "431 " + nick + " :No nickname given\r\n");
        return;
    }
    const std::string &newNick = params[0]; // este el nick nuevo o el primer noic

    //miramos si el formato es válido de acuerdo a las normas del IRC 
    if (!isValidNick(newNick))
    {
        client->queueMessage(srv + "432 " + nick + " " + newNick + " :Erroneus nickname\r\n");
        return;
    }

    // comprobamos que el nickname no está ya en uso 
    // "ana" -> "Ana" es case insenstive
    Client *other = findByNick(newNick);
    if (other && other != client)
    {
        client->queueMessage(srv + "433 " + nick + " " + newNick + " :Nickname is already in use\r\n");
        return;
    }

    // si el nick introducido es el mismo que el anterior, no hacemos nada 
    // ver si esta comprobación no sería mejor hacerla antes ? si ya es el mismo no? no voy a hacer todo para nada 
    if (newNick == client->getNick())
        return;

    // MONTAR EL PREFIJO ANTES DE CAMBIARLO!!!
    const bool isOldNickRegistered = client->isRegistered(); //hacemos la comprobacion con su antigo nick de si estaba registrado
    //hay que hacer la comprobación antes porque si la hacemos despues nos va a salir que no existe obvio 
    //comprobar el método isRegistered de si hace bien las comprobaciones DUDA
    const std::string oldPrefix = client->getPrefix();
    client->setNick(newNick);

    // si ya estaba registrado, avisar al propio cliente y a todos los que
    // compartan algún canal con él, UNA sola vez a cada uno (el set evita
    // duplicados si comparten varios canales)
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

    // Si con este NICK se completa el registro => bienvenido
    // Si ya estaba bienvenido no hace nada
    completeRegistration(client);

}

void Server::handlePass(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";

    //aun no tiene nick porque el usuario no lo ha seteado pero lo necesitamos para mandar el mensaje de error
    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    if (params.empty())
    {
        client->queueMessage(srv + "461 " + nick + " PASS :Not enough parameters\r\n");
        return;
    }

    // 2. Ya registrado: se rechaza SIN comparar la contraseña, para que un
    // PASS erróneo posterior no pueda tumbar una sesión ya establecida.

    //compropbamos que el usuario ya está registrado 
    //si lo está ya mandamos 462 y return porque ya no seguimos con los demás obvio
    if (client->isRegistered())
    {
        client->queueMessage(srv + "462 " + nick + " :You may not reregister\r\n");
        return;
    }

    //si la contraseña es incorrecta, lo indicamos y tenemos que tumbar la sesión de acuerdo al protocolo
    if (params[0] != _password)
    {
        client->queueMessage(srv + "464 " + nick + " :Password incorrect\r\n");
        disconnect(*client, "wrong password");
        return;
    }

    // si la contraseña es correcta lo marcamos como que se ha podido autenticar bien 
    client->setAuthenticated(true);
}