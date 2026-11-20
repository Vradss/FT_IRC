/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 20:25:49 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/20 02:17:01 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

Channel::Channel() : _chName(""), _chTopic(""), _chUsers(), _chOperators(), _chKey(""), _hasChKey(false),
      _chInviteOnly(false), _chUserLimit(0), _topicRestricted(false) {}


Channel::Channel(const std::string &name) :
_chName(name), _chTopic(""), _chUsers(), _chKey(""), _hasChKey(false),
      _chInviteOnly(false), _chUserLimit(0), _topicRestricted(false)
{
}

Channel::Channel(const Channel &other)
    : _chName(other._chName),
      _chTopic(other._chTopic),
      _chUsers(other._chUsers),
      _chOperators(other._chOperators),
      _chInvited(other._chInvited),
      _chKey(other._chKey),
      _hasChKey(other._hasChKey),
      _chInviteOnly(other._chInviteOnly),
      _chUserLimit(other._chUserLimit),
      _chJoinOrder(other._chJoinOrder),
      _topicRestricted(other._topicRestricted){}

Channel &Channel::operator=(const Channel &other)
{
    if (this != &other)
    {
        _chName = other._chName;
        _chTopic = other._chTopic;
        _chUsers = other._chUsers;
        _chOperators = other._chOperators;
        _chKey = other._chKey;
        _hasChKey = other._hasChKey;
        _chInvited = other._chInvited;
        _chInviteOnly = other._chInviteOnly;
        _chUserLimit = other._chUserLimit;
        _chJoinOrder = other._chJoinOrder;
        _topicRestricted = other._topicRestricted;
    }
    return *this;
}


Channel::~Channel(){}

const std::string &Channel::getChName() const
{
    return _chName;
}

void Channel::setChName(const std::string &chName)
{
    _chName = chName;
}

const std::string &Channel::getChTopic() const
{
    return _chTopic;
}

void Channel::setChTopic(const std::string &chTopic)
{
    _chTopic = chTopic;
}

const std::map<int, Client *> &Channel::getChUsers() const
{
    return _chUsers;
}

void Channel::setChUsers(const std::map<int, Client *> &chUsers)
{
    _chUsers = chUsers;
}

const std::string &Channel::getChKey() const
{
    return _chKey;
}

void Channel::setChKey(const std::string &chKey)
{
    _chKey = chKey;
}

bool Channel::getHasChKey() const
{
    return _hasChKey;
}

void Channel::setHasChKey(bool hasChKey)
{
    _hasChKey = hasChKey;
}

bool Channel::addUser(Client *client)
{
    /*
    if (!client)
        return false;
    // insert() no sobrescribe si la clave (el fd) ya existe
    // .second dice si se insertó, buscamos el value para luego devolverlo y ver si se ha insertado o no 
    // https://stackoverflow.com/questions/60766336/map-c-using-itr-first-second-vs-itr-second
    return _chUsers.insert(std::make_pair(client->getFd(), client)).second;*/
    if (!client)
        return false;
    // insert() no sobrescribe si la clave ya existe; .second dice si se insertó
    bool inserted = _chUsers.insert(std::make_pair(client->getFd(), client)).second;
    if (inserted)
        _chJoinOrder.push_back(client);     // al final = el más reciente
    return inserted;

}

//para saber si el cliente es operador del canal 
bool Channel::isOperator(Client *client) const
{
    if (!client)
        return false;
    return _chOperators.find(client->getFd()) != _chOperators.end();
}


// Da privilegios de operador a un cliente que YA es miembro del canal.
// Devuelve true si se añadió, false si no es miembro, ya era operador
// o el puntero es NULL.
bool Channel::addOperator(Client *client)
{
    if (!client)
        return false;
    // Un operador siempre tiene que ser miembro.
    //si no encontramos al usuario false 
    if (_chUsers.find(client->getFd()) == _chUsers.end())
        return false;
    return _chOperators.insert(std::make_pair(client->getFd(), client)).second;
}

// para saber si un user es miembro del canal 
bool Channel::isMember(Client *client) const
{
    if (!client)
        return false;
    return _chUsers.find(client->getFd()) != _chUsers.end();
}

// para saber si un canal solo va por invitación 
bool Channel::isInviteOnly() const
{
    return _chInviteOnly;
}

// ¿Tiene el cliente una invitación pendiente a este canal? NO ENTIENDO DEL TODO 
bool Channel::isInvited(Client *client) const
{
    if (!client)
        return false;
    return _chInvited.count(client) > 0;
}

//borra la invitación pendiente del cliente (si la tenía)
//si no estaba en la lista no hace nada
void Channel::removeInvite(Client *client)
{
    if (!client)
        return;
    _chInvited.erase(client);
}

// para saber si el canal tiene límite de usuarios 
bool Channel::hasUserLimit() const
{
    return _chUserLimit > 0; // si el limite del usuario es mayor que 0 devuelve true y por tanto tiene user limit
    //esto lo hago así porque no me apetece tener muchos booleanos 
}

// para saber el número máximo de usuarios 
size_t Channel::getUserLimit() const
{
    return _chUserLimit;
}


void Channel::removeUser(Client *client)
{
    /*
    if (!client)
        return;
    _chUsers.erase(client->getFd());*/
    if (!client)
        return;
    _chUsers.erase(client->getFd());

    for (std::vector<Client *>::iterator it = _chJoinOrder.begin();
         it != _chJoinOrder.end(); ++it)
    {
        if (*it == client)
        {
            _chJoinOrder.erase(it);
            break;
        }
    }
}

// Quita al cliente de la lista de operadores
void Channel::removeOperator(Client *client)
{
    if (!client)
        return;
    _chOperators.erase(client->getFd());
}

// si el canal está vacio, esto es para sabe si borro o no el canal 
bool Channel::isEmpty() const
{
    return _chUsers.empty();
}


bool Channel::hasOperators() const
{
    return !_chOperators.empty();
}



// para promocionar el miembro mas antiguo a operador
Client *Channel::getOldestMember() const
{
    for (size_t i = 0; i < _chJoinOrder.size(); ++i)
        if (!_chJoinOrder[i]->shouldClose())
            return _chJoinOrder[i];
    return NULL;
}