/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vrads <vrads@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 12:27:24 by vrads             #+#    #+#             */
/*   Updated: 2026/11/21 12:40:27 by vrads            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Parser.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <cctype>
#include <stdexcept>
#include <iostream>
#include <sstream>

/**
 * @file Server.cpp
 * @brief Network part of the server: one poll() watches all sockets.
 *
 * @verbatim
 *  Server() -> setupSocket()                  open the port
 *  run()    -> updatePollEvents() -> poll()   wait for something to happen
 *           -> acceptClient()                 new client
 *           -> handleRead() -> handleLine()   client sent a line -> command
 *           -> handleWrite()                  send pending replies
 *           -> removeClosedClients()          close clients that must leave
 * @endverbatim
 */

/** @brief Set to 0 by Ctrl+C (main.cpp) to stop run(). */
extern volatile sig_atomic_t g_running;

/** @brief Max IRC line size, "\r\n" included. */
static const size_t IRC_MAX_INPUT = 512;

/** @brief Lowercase copy, so "Bob" and "bob" are the same nick. */
static std::string toLower(const std::string &s)
{
    std::string out(s);
    for (std::string::size_type i = 0; i < out.size(); ++i)
        out[i] = std::tolower(static_cast<unsigned char>(out[i]));
    return out;
}

/* ===================== Construction / destruction ===================== */

/** @brief Saves port and password and opens the listening socket. */
Server::Server(int port, const std::string &password)
    : _port(port), _password(password), _listenFd(-1), _serverName("ircserv")
{
    setupSocket();
}

/** @brief Frees all channels and closes all sockets. */
Server::~Server()
{
    for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;
    _channels.clear();

    for (size_t i = 0; i < _fds.size(); ++i)
        close(_fds[i].fd);
    std::cout << "Server closed" << std::endl;
}

/**
 * @brief Closes the listening socket and throws an error.
 * Used by setupSocket(). The error is caught in main().
 */
void Server::fail(const std::string &what)
{
    if (_listenFd >= 0)
        close(_listenFd);
    throw std::runtime_error(what + " failed");
}

/**
 * @brief Opens the port: socket -> setsockopt -> fcntl -> bind -> listen.
 * The listening socket is stored in _fds[0]; it only accepts new clients.
 * Called by the constructor.
 */
void Server::setupSocket()
{
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
        fail("socket");

    int yes = 1;
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        fail("setsockopt");

    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
        fail("fcntl");

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(_port);

    if (bind(_listenFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
        fail("bind");
    if (listen(_listenFd, SOMAXCONN) < 0)
        fail("listen");

    struct pollfd p;
    p.fd = _listenFd;
    p.events = POLLIN;
    p.revents = 0;
    _fds.push_back(p);
}

/* ============================== Main loop ============================== */

/**
 * @brief Main loop: waits in poll() and handles every socket that is ready.
 * New client -> acceptClient(). Data -> handleRead(). Can send -> handleWrite().
 * At the end of each turn, removeClosedClients(). Stops on Ctrl+C.
 */
void Server::run()
{
    std::cout << "ircserv listening on port " << _port << std::endl;
    while (g_running)
    {
        updatePollEvents();
        if (poll(&_fds[0], _fds.size(), -1) < 0)
            break;
        for (size_t i = 0; i < _fds.size(); ++i)
        {
            int     fd = _fds[i].fd;
            short   rev = _fds[i].revents;

            if (rev == 0)
                continue;
            if (fd == _listenFd)
            {
                acceptClient();
                continue;
            }
            if (rev & (POLLERR | POLLNVAL))
            {
                std::map<int, Client>::iterator it = _clients.find(fd);
                if (it != _clients.end())
                    it->second.markToClose();
                continue;
            }
            if (rev & (POLLIN | POLLHUP))
                handleRead(fd);
            if (rev & POLLOUT)
                handleWrite(fd);
        }
        removeClosedClients();
    }
}

/**
 * @brief Tells poll() what to watch: always POLLIN, and POLLOUT only for
 * clients with something to send. Called by run() before each poll().
 */
void Server::updatePollEvents()
{
    for (size_t i = 1; i < _fds.size(); ++i)
    {
        std::map<int, Client>::iterator it = _clients.find(_fds[i].fd);
        _fds[i].events = POLLIN;
        if (it != _clients.end() && it->second.hasOutput())
            _fds[i].events |= POLLOUT;
        _fds[i].revents = 0;
    }
}

/* ================================ Events ================================ */

/**
 * @brief Accepts a new client, saves its IP as hostname and adds it to
 * _fds and _clients. Called by run() when the listening socket is ready.
 */
void Server::acceptClient()
{
    struct sockaddr_in  addr;
    socklen_t           len = sizeof(addr);
    int fd = accept(_listenFd, reinterpret_cast<struct sockaddr *>(&addr), &len);
    if (fd < 0)
        return;
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        close(fd);
        return;
    }
    struct pollfd p;
    p.fd = fd;
    p.events = POLLIN;
    p.revents = 0;
    _fds.push_back(p);
    Client client(fd);
    client.setHostname(inet_ntoa(addr.sin_addr));
    _clients.insert(std::make_pair(fd, client));
    std::cout << "[fd " << fd << "] connected from "
              << inet_ntoa(addr.sin_addr) << std::endl;
}

/**
 * @brief Reads what the client sent and passes each full line to handleLine().
 * Incomplete lines wait in the buffer. Disconnects on close, error or a
 * line that is too long. Called by run().
 */
void Server::handleRead(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end() || it->second.shouldClose())
        return;
    Client &client = it->second;

    char    buf[1024];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n <= 0)
    {
        disconnect(client, n == 0 ? "connection closed" : "recv error");
        return;
    }

    client.appendInput(buf, static_cast<size_t>(n));

    std::string line;
    while (!client.shouldClose() && client.extractLine(line))
    {
        if (line.size() > 510)
            line.erase(510);
        if (!line.empty())
            handleLine(client, line);
    }

    if (client.inputSize() > IRC_MAX_INPUT)
        disconnect(client, "input too long");
}

/**
 * @brief Sends the client's pending output. If only part is sent, the rest
 * is sent next turn. Called by run() when poll() says POLLOUT.
 */
void Server::handleWrite(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end() || !it->second.hasOutput())
        return;
    Client &client = it->second;

    const std::string &out = client.getOutput();
    ssize_t n = send(fd, out.c_str(), out.size(), 0);
    if (n < 0)
    {
        disconnect(client, "send error");
        return;
    }
    client.consumeOutput(static_cast<size_t>(n));
}

/**
 * @brief Parses one line and sends the command to dispatcher().
 * Bad lines are ignored. Called by handleRead(). The prints are for debug.
 */
void Server::handleLine(Client &client, const std::string &line)
{
    std::cout << "[fd " << client.getFd() << "] line: \"" << line << "\"" << std::endl;

    Instruction instruction;
    std::string err_msg;

    if (!Parser::parse(line, instruction, err_msg))
        return;

    std::cout << "    command: [" << instruction.command << "]" << std::endl;
    for (size_t i = 0; i < instruction.parameters.size(); ++i)
        std::cout << "    param[" << i << "]: [" << instruction.parameters[i] << "]" << std::endl;

    dispatcher(&client, instruction.command, instruction.parameters);
}

/* ============================ Deferred close ============================ */

/**
 * @brief Closes and deletes every client marked by disconnect().
 * Calls cleanupClient() first. Called by run() at the end of each turn.
 */
void Server::removeClosedClients()
{
    for (size_t i = _fds.size(); i-- > 1; )
    {
        int fd = _fds[i].fd;
        std::map<int, Client>::iterator it = _clients.find(fd);
        if (it == _clients.end() || !it->second.shouldClose())
            continue;
        cleanupClient(&it->second);
        close(fd);
        _clients.erase(it);
        _fds.erase(_fds.begin() + i);
    }
}

/* ======================== Network API for commands ======================= */

/** @brief Queues a line for the client. handleWrite() sends it later. */
void Server::sendTo(Client &client, const std::string &line)
{
    client.queueOutput(line + "\r\n");
}

/**
 * @brief Marks the client to leave. removeClosedClients() closes it at the
 * end of the turn. Used by QUIT, wrong PASS and network errors.
 */
void Server::disconnect(Client &client, const std::string &reason)
{
    if (client.shouldClose())
        return;
    std::cout << "[fd " << client.getFd() << "] disconnect: " << reason << std::endl;
    client.markToClose();
}

/** @brief Finds a client by nick (case-insensitive). NULL if not found. */
Client *Server::findByNick(const std::string &nick)
{
    std::string wanted = toLower(nick);
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        if (!it->second.shouldClose() && toLower(it->second.getNick()) == wanted)
            return &it->second;
    return NULL;
}

/** @brief Server password, checked by handlePass(). */
const std::string &Server::getPassword() const { return _password; }

/**
 * @brief Before closing a client: sends QUIT to its channels, removes it
 * from them (leaveChannel) and deletes its invites.
 * Called by removeClosedClients().
 */
void Server::cleanupClient(Client *client)
{
    std::string reason = client->getQuitReason();
    if (reason.empty())
        reason = "Connection closed";

    std::vector<std::string> names;
    std::set<Client *> recipients;

    const std::map<std::string, Channel *> &channels = client->getChannels();
    for (std::map<std::string, Channel *>::const_iterator cit = channels.begin();
         cit != channels.end(); ++cit)
    {
        names.push_back(cit->first);

        const std::map<int, Client *> &users = cit->second->getChUsers();
        for (std::map<int, Client *>::const_iterator uit = users.begin();
             uit != users.end(); ++uit)
        {
            if (uit->second == client)
                continue;
            if (uit->second->shouldClose())
                continue;
            recipients.insert(uit->second);
        }
    }

    if (!recipients.empty())
    {
        const std::string msg = ":" + client->getPrefix() + " QUIT :" + reason + "\r\n";
        for (std::set<Client *>::iterator rit = recipients.begin(); rit != recipients.end(); ++rit)
            (*rit)->queueMessage(msg);
    }

    for (size_t i = 0; i < names.size(); ++i)
    {
        Channel *channel = findChannel(names[i]);
        if (channel)
            leaveChannel(client, channel);
    }

    for (std::map<std::string, Channel *>::iterator it = _channels.begin();
         it != _channels.end(); ++it)
        it->second->removeInvite(client);
}
