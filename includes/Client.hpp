#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <cstddef>
#include <map>

class Channel;

/**
 * @file Client.hpp
 * @brief One accepted TCP connection: its buffers and its user data.
 */

/**
 * @class Client
 * @brief A connected client. It does NO I/O: it only stores data.
 *
 * Server does recv/send; Client only keeps what arrived and what has to be
 * sent. That way all the network code lives in one place (Server::run).
 *
 * Unlike Server, Client CAN be copied: std::map copies it on insert().
 * That is safe because Client never closes its fd (Server does).
 */
class Client
{
    public:

        explicit Client(int fd);

        /* =================== Socket and buffers ================ */
        int                 getFd() const;
        void                appendInput(const char *data, size_t len);
        bool                extractLine(std::string &line);
        size_t              inputSize() const;
        void                queueOutput(const std::string &data);
        void                queueMessage(const std::string &msg);
        bool                hasOutput() const;
        const std::string   &getOutput() const;
        void                consumeOutput(size_t n);

        /* ============================ Deferred close ============================ */
        void                markToClose();
        bool                shouldClose() const;
        bool                isMarkedForDisconnect() const;
        void                markForDisconnect();

        /* =================== Identity: nick, user, realname, host =============== */
        const std::string   &getNick() const;
        void                setNick(const std::string &nick);
        const std::string   &getUsername() const;
        void                setUsername(const std::string &username);
        void                setRealname(const std::string &realname);
        void                setHostname(const std::string &hostname);
        std::string         getPrefix() const;

        /* ========================== Registration state ========================== */
        bool                isAuthenticated() const;
        void                setAuthenticated(bool value);
        bool                isRegistered() const;
        bool                hasWelcomed() const;
        void                setWelcomed(bool value);

        /* =============================== Channels =============================== */
        void                addChannel(const std::string &name, Channel *channel);
        void                removeChannel(const std::string &name);
        const std::map<std::string, Channel *> &getChannels() const;

        /* ================================= QUIT ================================= */
        void                setQuitReason(const std::string &reason);
        const std::string   &getQuitReason() const;

    private:
        /* =========================== Socket and buffers ========================= */
        int             _fd; 
        std::string     _inBuffer; 
        std::string     _outBuffer;    


        /* ============================ Deferred close ============================ */
        bool            _toClose;           
        bool            _markedForDisconnect; 

        /* =============================== Identity =============================== */
        std::string     _nick;         
        std::string     _username;
        std::string     _realname;  
        std::string     _hostname;   

        /* ========================== Registration state ========================== */
        bool            _authenticated;
        bool            _welcomed;   

        /* =============================== Channels =============================== */
        std::map<std::string, Channel*> _clientChannels; 

        std::string     _quitReason;    

};

#endif
