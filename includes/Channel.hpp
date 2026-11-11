/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 19:27:38 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/11 20:26:42 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include "Client.hpp"


class Channel
{
    private:
        std::string _chName;
        std::string _chTopic;
        //importante Client* para no pasar una copia y tener datos desincronizados o dobles close
        std::map<int, Client*> _chUsers;
        // la primera vez que se crea un canal, no tiene clave
        //la hay cuando MODE +k 
        std::string _chKey; //ver si esto puede ir en claro o si estoy fumada
        bool _hasChKey; //esto igual por si me mandan una clave y el canan no tiene clave? 

    public:
        //constructores
        Channel();
        //Channel::Channel(const std::string &name); no sé si lo usaré 
        Channel(const Channel &other);
        Channel& operator=(const Channel &other);
        ~Channel();

        //getters y setters de las propiedades
        const std::string &getChName() const;
        void setChName(const std::string &chName);
        const std::string &getChTopic() const;
        void setChTopic(const std::string &chTopic);
        const std::map<int, Client*> &getChUsers() const;
        void setChUsers(const std::map<int, Client*> &chUsers); //no deberia de usarse en principio
        const std::string &getChKey() const;
        void setChKey(const std::string &chKey);
        bool getHasChKey() const;
        void setHasChKey(const bool hasChKey); //no sé si lo usaré 

};


#endif