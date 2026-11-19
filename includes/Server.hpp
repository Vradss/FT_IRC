#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <vector>
#include "Client.hpp"
#include "Channel.hpp"

/**
 * @file Server.hpp
 * @brief Servidor IRC: escucha, acepta clientes y mueve bytes con un unico poll().
 */

/**
 * @class Server
 * @brief Dueño de todos los sockets. Es la unica clase que llama a
 *        poll, accept, recv, send y close.
 *
 * Por que hay parte public y parte private:
 * - **public**: lo que main y los compañeros (B: comandos, C: canales) pueden
 *   pedir. Es un "contrato": si cambia, hay que avisarles.
 * - **private**: la maquinaria de red. Si B o C pudieran llamar a recv o close
 *   directamente, romperiamos las reglas del subject (un solo poll, cierre
 *   diferido). Al ser privado, el compilador lo impide.
 *
 * Flujo: run() -> updatePollEvents() -> poll() -> acceptClient() /
 *        handleRead() / handleWrite() -> removeClosedClients() -> otra vuelta.
 */
class Server
{
    public:

        /**
         * @brief Crea el socket de escucha (llama a setupSocket()).
         * @param port     Puerto TCP ya validado por main (1024-65535).
         * @param password Contraseña que pediran PASS.
         */
        Server(int port, const std::string &password);
        ~Server();
        void                run();

        //Esto tienen que hacerlo los usuarios , protocolos y redes
        void                sendTo(Client &client, const std::string &line);
        void                disconnect(Client &client, const std::string &reason);
        Client              *findByNick(const std::string &nick);
        const std::string   &getPassword() const;

        //aurora: 
        void dispatcher(Client *client, const std::string &command, const std::vector<std::string> &params);
        
        int joinChannel(Client *client, const std::string &chName, const std::string &key);
        void handleJoin(Client *client, const std::vector<std::string> &params);

        /*esta funcion es para PASS y básicamente 
            será al principio de que el cliente se conecte que para registrarse tendrá que poner primero la contraseña
            no podrá hacer nada sin la contraseña
        */
        void handlePass(Client *client, const std::vector<std::string> &params);
        void handleNick(Client *client, const std::vector<std::string> &params);
        void handleUser(Client *client, const std::vector<std::string> &params);
        void completeRegistration(Client *client);

        void privateMessage(Client *client, const std::vector<std::string> &params);
        Channel *findChannel(const std::string &name);

        void handlePart(Client *client, const std::vector<std::string> &params);
        void leaveChannel(Client *client, Channel *channel);

        void handleKick(Client *client, const std::vector<std::string> &params);

        void cleanupClient(Client *client);
        void handleQuit(Client *client, const std::vector<std::string> &params);

        void handlePing(Client *client, const std::vector<std::string> &params);
        void handlePong(Client *, const std::vector<std::string> &);
        void handleCap(Client *client, const std::vector<std::string> &params);

    private:
        //Estados, evaluar si necesitamo mas? 
        int                         _port;      ///< Puerto en el que escuchamos.
        std::string                 _password;  ///< Contraseña de ./ircserv <port> <password>.
        int                         _listenFd;  ///< Socket de escucha (solo sirve para accept).
        std::string                 _serverName; 
        //     struct pollfd {
        //     int   fd;       // qué socket
        //     short events;   // qué QUIERES vigilar (lo pones tú: POLLIN, POLLOUT)
        //     short revents;  // qué HA PASADO (lo rellena poll)
        // };

        
        std::vector<struct pollfd>  _fds;       ///< Array que recibe poll(). [0] = _listenFd.
        std::map<int, Client>       _clients;   ///< fd -> Client. El map no mueve sus elementos,
                                                ///< asi las referencias Client& siguen validas.
        std::map<std::string, Channel*> _channels; 


        //Funciones para runSocket                                        
        void    setupSocket();
        void    fail(const std::string &what);
        void    acceptClient();
        void    handleRead(int fd);
        void    handleWrite(int fd);
        void    handleLine(Client &client, const std::string &line);
        void    updatePollEvents();
        void    removeClosedClients();

        Server(const Server &);
        Server &operator=(const Server &);

};

#endif
