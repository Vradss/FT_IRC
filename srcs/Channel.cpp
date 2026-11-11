/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 20:25:49 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/11 20:26:20 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

/* ************************************************************************** */
/*                          Constructores y destructor                        */
/* ************************************************************************** */

Channel::Channel()
    : _chName(""), _chTopic(""), _chUsers(), _chKey(""), _hasChKey(false)
{
}


/*
Channel::Channel(const std::string &name)
    : _chName(name), _chTopic(""), _chUsers(), _chKey(""), _hasChKey(false)
{
}
*/


Channel::Channel(const Channel &other)
    : _chName(other._chName),
      _chTopic(other._chTopic),
      _chUsers(other._chUsers),
      _chKey(other._chKey),
      _hasChKey(other._hasChKey)
{
}

Channel &Channel::operator=(const Channel &other)
{
    if (this != &other)
    {
        _chName = other._chName;
        _chTopic = other._chTopic;
        _chUsers = other._chUsers;
        _chKey = other._chKey;
        _hasChKey = other._hasChKey;
    }
    return *this;
}


Channel::~Channel()
{
}

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