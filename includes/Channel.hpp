/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 19:27:38 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/19 02:42:56 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include <set>  
#include "Client.hpp"


class Channel
{
    private:
        std::string _chName;
        std::string _chTopic;
        //importante Client* para no pasar una copia y tener datos desincronizados o dobles close
        std::map<int, Client*> _chUsers;
        std::map<int, Client*> _chOperators; //listado de los operadores del canal 

        /**
         * alguien invita a X a un canal con invitación 
         * lo guardo en chInvited 
         * lo pongo set porque así no tengo que recorrer la lista puedo simplemente pregunta 
         * set no admite duplicados 
         * cuando X haga JOIN #canalConInvitacion se comprobará si está en esta lista
         * si está, le dejamos hacer el JOIN 
         * si no tiene invite => 473
         * 
         * IMPORTANTE: una invitación es de un solo uso asi que tengo que quitar a X de esta lista
         * por si se sale del canal o algo que no pueda volver a entrar sin invitación
         */
        std::set<Client*> _chInvited; // lista de invitados 
        // la primera vez que se crea un canal, no tiene clave
        //la hay cuando MODE +k 
        std::string _chKey; //ver si esto puede ir en claro o si estoy fumada
        bool _hasChKey; //esto igual por si me mandan una clave y el canan no tiene clave? 
        bool _chInviteOnly; 
        size_t _chUserLimit;
		/*CÉSAR*/bool _topicRestricted;
        

    public:
        //constructores
        Channel();
        Channel(const std::string &name);
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


        //funciones del canal
        bool addUser(Client *client);
        bool addOperator(Client *client); //para añadir un operador al listado de operadores 
        bool isOperator(Client *client) const; 
        bool isMember(Client *client) const;
        bool isInviteOnly() const; 
        bool hasUserLimit() const;
        size_t getUserLimit() const;
        bool isInvited(Client *client) const;
        void removeInvite(Client *client);
        void addInvite(Client *client);
		/*CËSAR*/ bool isTopicRestricted() const;


};


#endif