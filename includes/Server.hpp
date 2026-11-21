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
 * @brief Dueño de todos los sockets. Es la UNICA clase que llama a
 *        poll, accept, recv, send y close.
 *
 * Por que hay parte public y parte private:
 * - **public**: lo que usan main() y los handlers de comandos (sendTo,
 *   disconnect, findByNick...). Si cambia una firma, hay que avisar al equipo.
 * - **private**: la maquinaria de red. Si un comando pudiera llamar a recv o
 *   close directamente, romperia las reglas del subject (un solo poll, nunca
 *   bloquear, cierre diferido). Al ser privado, el compilador lo impide.
 *
 * Una vuelta del bucle:
 *   run() -> updatePollEvents() -> poll() -> acceptClient() / handleRead() /
 *   handleWrite() -> removeClosedClients() -> otra vuelta.
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

        // ---- API de red para los comandos ----
        // sendTo:     encola una linea (añade "\r\n"); se envia cuando poll lo permita.
        // disconnect: marca al cliente; se cierra al FINAL de la vuelta, no ahora.
        // findByNick: busca sin distinguir mayusculas; NULL si no existe.
        void                sendTo(Client &client, const std::string &line);
        void                disconnect(Client &client, const std::string &reason);
        Client              *findByNick(const std::string &nick);
        const std::string   &getPassword() const;

        //aurora: 
        void dispatcher(Client *client, const std::string &command, const std::vector<std::string> &params);
        
        int joinChannel(Client *client, const std::string &chName, const std::string &key);
        void handleJoin(Client *client, const std::vector<std::string> &params);
        void handlePass(Client *client, const std::vector<std::string> &params);
        void handleNick(Client *client, const std::vector<std::string> &params);
        void handleUser(Client *client, const std::vector<std::string> &params);
        void completeRegistration(Client *client);
		/*CÉSAR		COMMAND: TOPIC*/ void handleTopic(Client *client, const std::vector<std::string>& params);
		/*CÉSAR		COMMAND: TOPIC*/ void handleInvite(Client *client, const std::vector<std::string> &params);

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
        // ---- Estado del servidor ----
        int                         _port;     ///< Puerto en el que escuchamos.
        std::string                 _password;  ///< Contraseña de ./ircserv <port> <password>.
        int                         _listenFd;  ///< Socket de escucha (solo sirve para accept).
        std::string                 _serverName; 
        // Cada elemento de _fds es un struct pollfd con 3 campos:
        //   fd      -> que socket vigilar
        //   events  -> que QUEREMOS saber (lo ponemos nosotros: POLLIN, POLLOUT)
        //   revents -> que HA PASADO (lo rellena poll al volver)
        std::vector<struct pollfd>  _fds;       ///< Array que recibe poll(). [0] = _listenFd.
        std::map<int, Client>       _clients;   ///< fd -> Client. El map no mueve sus elementos
                                                ///< al insertar, asi los Client& siguen validos.
        std::map<std::string, Channel*> _channels; 


        // ---- Maquinaria de red (en el orden en que actua) ----
        void    setupSocket();
        void    fail(const std::string &what);
        void    acceptClient();
        void    handleRead(int fd);
        void    handleWrite(int fd);
        void    handleLine(Client &client, const std::string &line);
        void    updatePollEvents();
        void    removeClosedClients();

        // Prohibido copiar: Server es dueño de los fds. Dos copias cerrarian
        // los mismos sockets dos veces en sus destructores.
        Server(const Server &);
        Server &operator=(const Server &);

};

#endif
