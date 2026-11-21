#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <cstddef>
#include <map>
class Channel;

/**
 * @file Client.hpp
 * @brief Una conexion TCP aceptada y sus buffers.
 */

/**
 * @class Client
 * @brief Un cliente conectado. NO hace I/O: solo guarda datos.
 *
 * Quien hace recv/send es Server; Client solo guarda lo que llega y lo que
 * hay que enviar. Asi toda la red queda en un unico sitio (Server::run).
 *
 * Reparto:
 * - Persona A: fd, buffers y cierre diferido.
 * - Persona B: datos del usuario (nick, user, registro). Añadir aqui sin
 *   tocar los campos de A.
 *
 * Por que public / private:
 * - Los atributos son **private**: nadie puede escribir en _outBuffer sin
 *   pasar por queueOutput(). Si mañana cambiamos como se guarda la salida,
 *   solo cambia esta clase.
 * - Las funciones son **public** porque Server (y B) las necesitan.
 *
 * A diferencia de Server, Client SI se puede copiar: std::map lo copia al
 * hacer insert(). Es seguro porque Client no cierra su fd (lo hace Server).
 */
class Client
{
    public:
        /**
         * @brief Crea el cliente para un fd ya aceptado y en O_NONBLOCK.
         * @param fd Socket devuelto por accept().
         *
         * explicit: evita que un int se convierta solo en Client por error.
         */
        explicit Client(int fd);
        int                 getFd() const;

        //Aquí tienen que completar lo de los usuarios, NICK USER PASS, etc GAAA
        const std::string   &getNick() const;
        void                setNick(const std::string &nick);

        void                appendInput(const char *data, size_t len);
        bool                extractLine(std::string &line);
        size_t              inputSize() const;
        void                queueOutput(const std::string &data);
        bool                hasOutput() const;
        const std::string   &getOutput() const;
        void                consumeOutput(size_t n);
        void                markToClose();
        bool                shouldClose() const;

        //AURORA:
        void addChannel(const std::string &name, Channel *channel);
        std::string getPrefix() const; 
        void queueMessage(const std::string &msg); 
        bool isRegistered() const;
        bool isMarkedForDisconnect() const; // esto para ver el estado   
        void markForDisconnect(); //esto para marcarlo como desconectado

    private:
        int         _fd;         ///< Socket TCP del cliente. Lo cierra Server, no Client.
        std::string _nick;       ///< (B) Nick actual; "" hasta que haga NICK. MODIFICAR OJITO
        std::string _inBuffer;   ///< Bytes recibidos sin '\n' todavia del cliente
        std::string _outBuffer;  ///< Bytes pendientes de send().
        bool        _toClose;    ///< true = cerrar en removeClosedClients().

        //aurora 
        std::map<std::string, Channel*> _clientChannels; //la lista de canales a la que pertenece el cliente
        std::string _username; 
        std::string _hostname; 
        bool _authenticated; // esto ponerlo a true cuando el user se autentique con la PASS del server si no puede pasar por logado y no 
        bool _markedForDisconnect;  //para si un comando es QUIT y ya el cliente lo marcó como desconectau para que no se ejecute nada más del buffer
    
};

#endif
