/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vrads <vrads@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 12:48:31 by vrads             #+#    #+#             */
/*   Updated: 2026/11/21 12:50:38 by vrads            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "Client.hpp"

/*
 * Client: only handles strings and buffers, no network calls.
 *
 *  Server::handleRead()          -> appendInput() -> extractLine() / inputSize()
 *  Server::sendTo()              -> queueOutput()
 *  Server::updatePollEvents()    -> hasOutput()
 *  Server::handleWrite()         -> getOutput() -> consumeOutput()
 *  Server::disconnect()          -> markToClose()
 *  Server::removeClosedClients() -> shouldClose()
 */

/* Every bool must start at false: an uninitialized bool holds garbage. */
Client::Client(int fd)
    : _fd(fd), _toClose(false), _markedForDisconnect(false),
      _authenticated(false), _welcomed(false)
{}

int Client::getFd() const { return _fd; }

const std::string &Client::getNick() const { return _nick; }
void Client::setNick(const std::string &nick) { _nick = nick; }

/* ============================= Input buffer ============================= */

/* Uses len because the recv() buffer is not null-terminated. */
void Client::appendInput(const char *data, size_t len)
{
    _inBuffer.append(data, len);
}

/*
 * Takes one line out of the buffer, up to the first '\n'.
 * Strips a trailing '\r', so both "\n" (nc) and "\r\n" (IRC clients) work.
 * Returns false when there is no full line yet.
 */
bool Client::extractLine(std::string &line)
{
    std::string::size_type pos = _inBuffer.find('\n');
    if (pos == std::string::npos)
        return false;
    line = _inBuffer.substr(0, pos);
    _inBuffer.erase(0, pos + 1);
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
    return true;
}

size_t Client::inputSize() const { return _inBuffer.size(); }

/* ============================= Output buffer ============================ */

void Client::queueOutput(const std::string &data) { _outBuffer += data; }
bool Client::hasOutput() const { return !_outBuffer.empty(); }
const std::string &Client::getOutput() const { return _outBuffer; }
void Client::consumeOutput(size_t n) { _outBuffer.erase(0, n); }

/* Adds a message to the output buffer. The server sends it on POLLOUT. */
void Client::queueMessage(const std::string &msg)
{
    queueOutput(msg);
}

/* ============================== Disconnect ============================== */

void Client::markToClose() { _toClose = true; }
bool Client::shouldClose() const { return _toClose; }


bool Client::isMarkedForDisconnect() const
{
    return _markedForDisconnect;
}

void Client::markForDisconnect()
{
    _markedForDisconnect = true;
}

void Client::setQuitReason(const std::string &reason)
{
    _quitReason = reason;
}

const std::string &Client::getQuitReason() const
{
    return _quitReason;
}

/* ============================= Registration ============================= */

/* Computed instead of stored, so it can never get out of sync. */
bool Client::isRegistered() const
{
    return _authenticated && !_nick.empty() && !_username.empty();
}

/* True once the client sent the correct PASS. */
bool Client::isAuthenticated() const
{
    return _authenticated;
}

void Client::setAuthenticated(bool value)
{
    _authenticated = value;
}

/* True once the 001 welcome was sent. */
bool Client::hasWelcomed() const
{
    return _welcomed;
}

void Client::setWelcomed(bool value)
{
    _welcomed = value;
}

/* =============================== Identity =============================== */

/* IRC prefix: nick!user@host */
std::string Client::getPrefix() const
{
    return _nick + "!" + _username + "@" + _hostname;
}

const std::string &Client::getUsername() const
{
    return _username;
}

void Client::setUsername(const std::string &username)
{
    _username = username;
}

void Client::setRealname(const std::string &realname)
{
    _realname = realname;
}

void Client::setHostname(const std::string &hostname)
{
    _hostname = hostname;
}

/* =============================== Channels =============================== */

void Client::addChannel(const std::string &name, Channel *channel)
{
    if (!channel)
        return;
    _clientChannels[name] = channel;
}

void Client::removeChannel(const std::string &name)
{
    _clientChannels.erase(name);
}

/* Read-only. Used by NICK and QUIT to know who to notify. */
const std::map<std::string, Channel *> &Client::getChannels() const
{
    return _clientChannels;
}
