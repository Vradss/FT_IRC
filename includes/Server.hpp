/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vrads <vrads@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 12:43:52 by vrads             #+#    #+#             */
/*   Updated: 2026/11/21 12:44:04 by vrads            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include "Client.hpp"
#include "Channel.hpp"

class Server
{
    public:

        Server(int port, const std::string &password);
        ~Server();
        
        /* ============================== Main loop =============================== */
        void                run();
    
        /* ======================== Network API for commands ====================== */
        void                sendTo(Client &client, const std::string &line);
        void                disconnect(Client &client, const std::string &reason);
        Client              *findByNick(const std::string &nick);
        Channel             *findChannel(const std::string &name);
        const std::string   &getPassword() const;

    
        /* ============================ Command routing =========================== */
    
        void    dispatcher(Client *client, const std::string &command,
                           const std::vector<std::string> &params);
    
        /* ======================= Registration: PASS NICK USER ==================== */
        void    handlePass(Client *client, const std::vector<std::string> &params);
        void    handleNick(Client *client, const std::vector<std::string> &params);
        void    handleUser(Client *client, const std::vector<std::string> &params);
        void    handleCap(Client *client, const std::vector<std::string> &params);
        void    completeRegistration(Client *client);
    
        /* ======================= Connection: PING PONG QUIT ===================== */
        void    handlePing(Client *client, const std::vector<std::string> &params);
        void    handlePong(Client *, const std::vector<std::string> &);
        void    handleQuit(Client *client, const std::vector<std::string> &params);
        void    cleanupClient(Client *client);
    
        /* ============ Channels: JOIN PART KICK TOPIC INVITE ============ */
        void    handleJoin(Client *client, const std::vector<std::string> &params);
        int     joinChannel(Client *client, const std::string &chName, const std::string &key);
        void    handlePart(Client *client, const std::vector<std::string> &params);
        void    leaveChannel(Client *client, Channel *channel);
        void    handleKick(Client *client, const std::vector<std::string> &params);
        void    handleTopic(Client *client, const std::vector<std::string> &params);
        void    handleInvite(Client *client, const std::vector<std::string> &params);
    
        /* =========================== Messages: PRIVMSG ========================== */
        void    privateMessage(Client *client, const std::vector<std::string> &params);


    private:

        Server(const Server &);
        Server &operator=(const Server &);

        
        /* ============================= Server state ============================= */
        int                             _port;  
        std::string                     _password; 
        int                             _listenFd;  
        std::string                     _serverName;

        std::vector<struct pollfd>      _fds;
        std::map<int, Client>           _clients;
        std::map<std::string, Channel*> _channels;

        /* =============== Network machinery  =============== */
        void    setupSocket();
        void    fail(const std::string &what);
        void    acceptClient();
        void    handleRead(int fd);
        void    handleWrite(int fd);
        void    handleLine(Client &client, const std::string &line);
        void    updatePollEvents();
        void    removeClosedClients();
};

#endif
