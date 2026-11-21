/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 20:25:49 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:45:13 by aumoreno         ###   ########.fr       */
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

    if (!client)
        return false;
    bool inserted = _chUsers.insert(std::make_pair(client->getFd(), client)).second;
    if (inserted)
        _chJoinOrder.push_back(client);
    return inserted;

}


bool Channel::isOperator(Client *client) const
{
    if (!client)
        return false;
    return _chOperators.find(client->getFd()) != _chOperators.end();
}



bool Channel::addOperator(Client *client)
{
    if (!client)
        return false;

    if (_chUsers.find(client->getFd()) == _chUsers.end())
        return false;
    return _chOperators.insert(std::make_pair(client->getFd(), client)).second;
}

bool Channel::isMember(Client *client) const
{
    if (!client)
        return false;
    return _chUsers.find(client->getFd()) != _chUsers.end();
}

bool Channel::isInviteOnly() const
{
    return _chInviteOnly;
}


bool Channel::isInvited(Client *client) const
{
    if (!client)
        return false;
    return _chInvited.count(client) > 0;
}

void Channel::removeInvite(Client *client)
{
    if (!client)
        return;
    _chInvited.erase(client);
}

bool Channel::hasUserLimit() const
{
    return _chUserLimit > 0;
}

size_t Channel::getUserLimit() const
{
    return _chUserLimit;
}


void Channel::removeUser(Client *client)
{

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

void Channel::removeOperator(Client *client)
{
    if (!client)
        return;
    _chOperators.erase(client->getFd());
}

bool Channel::isEmpty() const
{
    return _chUsers.empty();
}


bool Channel::hasOperators() const
{
    return !_chOperators.empty();
}


Client *Channel::getOldestMember() const
{
    for (size_t i = 0; i < _chJoinOrder.size(); ++i)
        if (!_chJoinOrder[i]->shouldClose())
            return _chJoinOrder[i];
    return NULL;
}