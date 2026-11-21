/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 19:27:38 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/21 11:56:15 by aumoreno         ###   ########.fr       */
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
    private:
        std::string _chName;
        std::string _chTopic;
        std::map<int, Client*> _chUsers;
        std::map<int, Client*> _chOperators;
        std::set<Client*> _chInvited;
        std::string _chKey;
        bool _hasChKey;
        bool _chInviteOnly; 
        size_t _chUserLimit;
        std::vector<Client *> _chJoinOrder; 
        bool _topicRestricted; // MODE +t (CÉSAR)
        

    public:
        Channel();
        Channel(const std::string &name);
        Channel(const Channel &other);
        Channel& operator=(const Channel &other);
        ~Channel();

        const std::string &getChName() const;
        void setChName(const std::string &chName);
        const std::string &getChTopic() const;
        void setChTopic(const std::string &chTopic);
        const std::map<int, Client*> &getChUsers() const;
        void setChUsers(const std::map<int, Client*> &chUsers);
        const std::string &getChKey() const;
        void setChKey(const std::string &chKey);
        bool getHasChKey() const;
        void setHasChKey(const bool hasChKey);



        bool addUser(Client *client);
        bool addOperator(Client *client);
        bool isOperator(Client *client) const; 
        bool isMember(Client *client) const;
        bool isInviteOnly() const; 
        bool hasUserLimit() const;
        size_t getUserLimit() const;
        bool isInvited(Client *client) const;
        void removeInvite(Client *client);

        void removeUser(Client *client);
        void removeOperator(Client *client);
        bool isEmpty() const;
        bool hasOperators() const;
        Client *getOldestMember() const;
        void addInvite(Client *client);
        bool isTopicRestricted() const;


};


#endif