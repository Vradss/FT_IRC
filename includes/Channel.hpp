/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 19:27:38 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 15:04:01 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include <set>
#include <vector>
#include "Client.hpp"

class Channel
{
    public:

        /* ============================= Constructors ============================= */
        Channel();
        Channel(const std::string &name);
        Channel(const Channel &other);
        Channel &operator=(const Channel &other);
        ~Channel();

        /* ======================== Identity: name, topic ========================= */
        const std::string               &getChName() const;
        void                            setChName(const std::string &chName);
        const std::string               &getChTopic() const;
        void                            setChTopic(const std::string &chTopic);

        /* =============================== Members ================================ */
        bool                            addUser(Client *client);
        void                            removeUser(Client *client);
        bool                            isMember(Client *client) const;
        bool                            isEmpty() const;
        Client                          *getOldestMember() const;
        const std::map<int, Client*>    &getChUsers() const;
        void                            setChUsers(const std::map<int, Client*> &chUsers);

        /* ============================== Operators =============================== */
        bool                            addOperator(Client *client);
        void                            removeOperator(Client *client);
        bool                            isOperator(Client *client) const;
        bool                            hasOperators() const;

        /* =============================== Invites ================================ */
        void                            addInvite(Client *client);
        void                            removeInvite(Client *client);
        bool                            isInvited(Client *client) const;

        /* ========================== Modes: +i +t +k +l ========================== */
        bool                            isInviteOnly() const;
		void                            changeInviteOnly(bool chInviteOnly); 
        bool                            isTopicRestricted() const;
		void                            changeTopicRestricted(bool topicRestricted); 
        const std::string               &getChKey() const;
        void                            setChKey(const std::string &chKey);
        bool                            getHasChKey() const;
        void                            setHasChKey(const bool hasChKey);
        bool                            hasUserLimit() const;
        size_t                          getUserLimit() const;
		void                            setUserLimit(size_t chUserLimit);
		std::string                     getChModes() const;


    private:

        /* ============================ Channel state ============================= */
        std::string                     _chName;
        std::string                     _chTopic;

        std::map<int, Client*>          _chUsers;
        std::map<int, Client*>          _chOperators;
        std::set<Client*>               _chInvited;

        std::string                     _chKey;
        bool                            _hasChKey;
        bool                            _chInviteOnly;
        size_t                          _chUserLimit;

        std::vector<Client *>           _chJoinOrder;
        bool                            _topicRestricted;
};

#endif
