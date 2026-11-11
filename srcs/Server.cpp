#include "Server.hpp"
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

/**
 * @file Server.cpp
 * @brief Implementacion de Server. Mapa de quien llama a quien:
 *
 * @verbatim
 *  main()
 *   ├─ Server()  ── setupSocket() ── fail() si algo falla
 *   └─ run()  (bucle hasta Ctrl+C)
 *       ├─ updatePollEvents()           pide POLLOUT a quien tenga salida
 *       ├─ poll()                       el UNICO del programa
 *       ├─ acceptClient()               fd de escucha listo
 *       ├─ handleRead()                 POLLIN
 *       │    ├─ Client::appendInput()
 *       │    ├─ Client::extractLine()   en bucle
 *       │    ├─ handleLine()  ── sendTo()        (mañana: CommandHandler de B)
 *       │    └─ disconnect()            recv <= 0 o linea demasiado larga
 *       ├─ handleWrite()                POLLOUT
 *       │    └─ Client::consumeOutput() / disconnect()
 *       └─ removeClosedClients()        cierre diferido
 *
 *  B y C desde sus comandos: sendTo(), disconnect(), findByNick(), getPassword()
 * @endverbatim
 */

extern volatile sig_atomic_t g_running;     ///< Definido en main.cpp; Ctrl+C lo pone a 0.

static const size_t IRC_MAX_INPUT = 1024;   ///< Proteccion contra lineas sin fin.

/**
 * @brief Copia de s en minusculas (los nicks IRC no distinguen mayusculas).
 *
 * static: solo existe dentro de este .cpp. Llamada por: findByNick().
 */
static std::string toLower(const std::string &s)
{
    std::string out(s);
    for (std::string::size_type i = 0; i < out.size(); ++i)
        out[i] = std::tolower(static_cast<unsigned char>(out[i]));
    return out;
}

/* ===================== Construccion / destruccion ===================== */

/**
 * @details _listenFd empieza en -1 para que fail() sepa si hay algo que cerrar.
 * Llamada por: main().  Llama a: setupSocket().
 */
Server::Server(int port, const std::string &password)
    : _port(port), _password(password), _listenFd(-1)
{
    setupSocket();
}

/**
 * @details _fds contiene el fd de escucha ([0]) y todos los clientes,
 * asi que un solo bucle cierra todo. Se ejecuta al salir de main().
 */
Server::~Server()
{
    for (size_t i = 0; i < _fds.size(); ++i)
        close(_fds[i].fd);
    std::cout << "Server closed" << std::endl;
}

/**
 * @details Si el constructor lanza, el destructor NO se ejecuta: por eso
 * cerramos aqui _listenFd, o se quedaria abierto.
 * Llamada por: setupSocket().  La excepcion la captura main().
 */
void Server::fail(const std::string &what)
{
    if (_listenFd >= 0)
        close(_listenFd);
    throw std::runtime_error(what + " failed");
}

/**
 * @details Orden obligatorio: socket -> setsockopt -> O_NONBLOCK -> bind -> listen.
 * Al final mete _listenFd en _fds[0] para que poll() avise de conexiones nuevas.
 * Llamada por: Server().  Llama a: fail() si algo falla.
 */
void Server::setupSocket()
{
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);            // TCP sobre IPv4
    if (_listenFd < 0)
        fail("socket");
    int yes = 1;                                            // relanzar sin esperar
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        fail("setsockopt");
    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
        fail("fcntl");

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);               // todas las interfaces
    addr.sin_port = htons(_port);                           // orden de red
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

/* ============================ Bucle principal ============================ */

/**
 * @details Una vuelta = preparar eventos, esperar en poll(), atender a cada
 * fd listo y, al final, cerrar los marcados.
 * Llamada por: main().
 * Llama a: updatePollEvents(), acceptClient(), handleRead(), handleWrite(),
 *          removeClosedClients().
 */
void Server::run()
{
    std::cout << "ircserv listening on port " << _port << std::endl;
    while (g_running)
    {
        updatePollEvents();
        // El UNICO poll del programa. -1 = esperar sin limite.
        // Devuelve <0 si llega Ctrl+C (o error): salimos limpio.
        //deberiamos esperiri sin limite? o como 
        if (poll(&_fds[0], _fds.size(), -1) < 0)
            break;
        // Indice y no iterador: acceptClient() hace push_back y puede
        // reubicar el vector. Los nuevos tienen revents = 0 y se saltan.
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
            if (rev & (POLLIN | POLLHUP))   // POLLHUP: recv devolvera 0
                handleRead(fd);
            if (rev & POLLOUT)
                handleWrite(fd);
        }
        removeClosedClients();              // cierre diferido
    }
}

/**
 * @details Pedimos POLLOUT solo a quien tiene algo que enviar.
 * Si lo pidieramos siempre, poll volveria al instante: 100% CPU.
 * Empieza en i = 1 porque [0] es el fd de escucha (solo POLLIN).
 * Llamada por: run(), al principio de cada vuelta.  Llama a: Client::hasOutput().
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

/* ================================ Eventos ================================ */

/**
 * @details El fd nuevo se pone en O_NONBLOCK y se añade a _fds y _clients.
 * Si accept falla, no pasa nada: se ignora (el servidor no puede caerse).
 * Llamada por: run(), cuando el fd de escucha tiene POLLIN.
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
    _clients.insert(std::make_pair(fd, Client(fd)));
    std::cout << "[fd " << fd << "] connected from "
              << inet_ntoa(addr.sin_addr) << std::endl;
}

/**
 * @details Un recv por vuelta (poll ya dijo que hay datos, no bloquea).
 * Llamada por: run(), con POLLIN o POLLHUP.
 * Llama a: Client::appendInput(), Client::extractLine(), handleLine(),
 *          disconnect().
 */
void Server::handleRead(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd); //esta es la ficha del cliente
    if (it == _clients.end() || it->second.shouldClose())
        return;
    Client &client = it->second;

    char    buf[1024];
    ssize_t n = recv(fd, buf, sizeof(buf), 0); //los bytes del mesnsaje OJITO
    if (n <= 0)                             // 0 = el cliente cerro; <0 = error. Sin errno.
    {
        disconnect(client, n == 0 ? "connection closed" : "recv error");
        return;
    }

    //gaaaaa
    //handleLine(client, std::string(buf, n));
    //pruebassss para ver que pasa 


    client.appendInput(buf, static_cast<size_t>(n)); // los mete al inbuffer para que no se pierde

    // Puede haber 0, 1 o varias lineas completas en el buffer.
    std::string line;
    while (!client.shouldClose() && client.extractLine(line))
        if (!line.empty())
            handleLine(client, line);

    if (client.inputSize() > IRC_MAX_INPUT)
        disconnect(client, "input too long");
}

/**
 * @details Un send por vuelta con todo lo pendiente; send puede enviar menos.
 * Llamada por: run(), con POLLOUT (pedido en updatePollEvents()).
 * Llama a: Client::getOutput(), Client::consumeOutput(), disconnect().
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
    client.consumeOutput(static_cast<size_t>(n));   // si fue parcial, el resto espera
}

/**
 * @details TEMPORAL hasta que B tenga CommandHandler: solo hace eco para
 * probar la red. Cuando exista, esta funcion sera solo:
 *     CommandHandler::execute(*this, client, line);
 * Es la PUERTA entre la red (A) y los comandos (B).
 * Llamada por: handleRead(), una vez por linea.  Llama a: sendTo().
 */
void Server::handleLine(Client &client, const std::string &line)
{
    std::cout << "[fd " << client.getFd() << "] line: \"" << line << "\"" << std::endl;
    sendTo(client, ":ircserv recibiendo señal prroos: " + line);
}

/* ========================== Cierre diferido ========================== */

/**
 * @details Recorre de atras hacia delante: al borrar _fds[i] los de detras
 * se mueven, pero esos ya los hemos visto. Nunca toca [0] (escucha).
 * Llamada por: run(), al FINAL de cada vuelta (cierre diferido).
 * Llama a: Client::shouldClose(), close().
 */
void Server::removeClosedClients()
{
    for (size_t i = _fds.size(); i-- > 1; )         // atras -> delante, sin tocar [0]
    {
        int fd = _fds[i].fd;
        std::map<int, Client>::iterator it = _clients.find(fd);
        if (it == _clients.end() || !it->second.shouldClose())
            continue;
        // TODO (C): antes de borrar, sacar al cliente de todos sus canales.
        close(fd);
        _clients.erase(it);
        _fds.erase(_fds.begin() + i);
    }
}

/* ========================= Contrato para B y C ========================= */

/**
 * @details Solo encola. El send real lo hara handleWrite() en otra vuelta.
 * Llamada por: handleLine() y, mas adelante, los comandos de B y C.
 * Llama a: Client::queueOutput().
 */
void Server::sendTo(Client &client, const std::string &line)
{
    client.queueOutput(line + "\r\n");
}

/**
 * @details Solo marca. Si ya estaba marcado, no repite el log.
 * Ojo: lo que haya en _outBuffer se perdera (se cierra en esta misma vuelta).
 * Llamada por: handleRead(), handleWrite() y los comandos de B (QUIT...).
 * Llama a: Client::markToClose().
 */
void Server::disconnect(Client &client, const std::string &reason)
{
    if (client.shouldClose())
        return;
    std::cout << "[fd " << client.getFd() << "] disconnect: " << reason << std::endl;
    client.markToClose();
}

/**
 * @details Busqueda lineal en _clients: con pocos clientes es suficiente.
 * Ignora a los ya marcados para cerrar (para B, ya no existen).
 * Llamada por: comandos de B y C (NICK, PRIVMSG, INVITE, KICK...).
 * Llama a: toLower(), Client::getNick(), Client::shouldClose().
 */
Client *Server::findByNick(const std::string &nick)
{
    std::string wanted = toLower(nick);
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        if (!it->second.shouldClose() && toLower(it->second.getNick()) == wanted)
            return &it->second;
    return NULL;
}

const std::string &Server::getPassword() const { return _password; }


//aurora aqui abajito para no molestar: 

