#include "Server.hpp"
#include "Parser.hpp"
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
#include <sstream>


/**
 * @file Server.cpp
 * @brief Parte de red: un UNICO poll() vigila todos los sockets y, cuando uno
 *        esta listo, leemos o escribimos sin bloquear.
 *
 * Vida de una conexion:
 *
 * @verbatim
 *  1. Arranque      Server() -> setupSocket()          abrir el puerto
 *  2. Bucle         run() -> updatePollEvents()        esperar en poll()
 *  3. Llega alguien acceptClient()                     nuevo Client en _clients
 *  4. Nos habla     handleRead() -> handleLine()       recv -> parser -> dispatcher
 *  5. Respondemos   sendTo() ... handleWrite()         encolar ahora, send despues
 *  6. Se va         disconnect() ... removeClosedClients()  marcar ahora, cerrar al final
 * @endverbatim
 *
 * Dos reglas que explican casi todo el fichero:
 *  - NUNCA llamamos a recv/send/accept sin que poll() diga que se puede.
 *    Asi ninguna llamada se queda esperando y el server no se congela.
 *  - NUNCA cerramos un socket en mitad de la vuelta (cierre diferido).
 *    Otro comando de la misma vuelta podria tener todavia un Client* a el.
 */

extern volatile sig_atomic_t g_running;     ///< Definido en main.cpp; Ctrl+C lo pone a 0.

// RFC: un mensaje IRC mide como maximo 512 bytes, contando el "\r\n" final.
// Si en el buffer de entrada se acumulan mas de 512 bytes SIN un '\n',
// el cliente esta mandando una linea sin fin: lo desconectamos.
static const size_t IRC_MAX_INPUT = 512;

/**
 * @brief Copia de s en minusculas: en IRC "Bob" y "bob" son el mismo nick.
 * static: solo existe dentro de este .cpp.
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
 * @details _listenFd empieza en -1 ("todavia no hay socket") para que fail()
 * sepa si hay algo que cerrar.
 */
Server::Server(int port, const std::string &password)
    : _port(port), _password(password), _listenFd(-1), _serverName("ircserv")
{
    setupSocket();
}

/**
 * @details Se ejecuta al salir de main() (Ctrl+C).
 * Los canales se crearon con new, asi que hay que liberarlos.
 * _fds contiene el fd de escucha ([0]) y todos los clientes, asi que un
 * solo bucle cierra todos los sockets.
 */
Server::~Server()
{
    for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;
    _channels.clear();
    
    for (size_t i = 0; i < _fds.size(); ++i)
        close(_fds[i].fd);
    std::cout << "Server closed" << std::endl;
}

/**
 * @details Si el constructor lanza una excepcion, el destructor NO se ejecuta:
 * por eso cerramos aqui _listenFd, o se quedaria abierto.
 * La excepcion la recoge el try/catch de main().
 */
void Server::fail(const std::string &what)
{
    if (_listenFd >= 0)
        close(_listenFd);
    throw std::runtime_error(what + " failed");
}

/**
 * @details Abre la "puerta" del servidor. El orden es obligatorio:
 *   socket -> setsockopt -> O_NONBLOCK -> bind -> listen.
 * Este socket NUNCA lee ni escribe datos de nadie: solo sirve para que
 * accept() reciba conexiones nuevas.
 */
void Server::setupSocket()
{
    // AF_INET = IPv4, SOCK_STREAM = TCP. Devuelve un fd, como un fichero.
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
        fail("socket");

    // Al cerrar el server, el sistema deja el puerto "ocupado" un rato
    // (TIME_WAIT). Sin SO_REUSEADDR, relanzarlo al momento daria "bind failed".
    int yes = 1;
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        fail("setsockopt");

    // No bloqueante: si accept() no tiene a nadie, devuelve -1 al momento
    // en vez de quedarse esperando y congelar el server.
    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
        fail("fcntl");

    // Direccion donde escuchamos: cualquier interfaz (localhost, wifi...) y
    // nuestro puerto. htonl/htons pasan los numeros al orden de bytes de la
    // red (big-endian), que puede ser distinto al del ordenador.
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(_port);

    // bind: "este socket es el del puerto X". Falla si el puerto ya esta en uso.
    if (bind(_listenFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
        fail("bind");
    // listen: empezar a aceptar llamadas. SOMAXCONN = tamaño maximo de la cola
    // de conexiones que esperan a que hagamos accept().
    if (listen(_listenFd, SOMAXCONN) < 0)
        fail("listen");

    // El fd de escucha va SIEMPRE en _fds[0]. Para el, POLLIN no significa
    // "hay datos" sino "hay alguien nuevo esperando a que le hagas accept()".
    struct pollfd p;
    p.fd = _listenFd;
    p.events = POLLIN;
    p.revents = 0;
    _fds.push_back(p);
}

/* ============================ Bucle principal ============================ */

/**
 * @details El corazon del servidor. Cada vuelta:
 *   1. decir a poll que vigilar        (updatePollEvents)
 *   2. dormir hasta que pase algo      (poll)
 *   3. atender a cada fd que se movio  (accept / read / write)
 *   4. cerrar los marcados             (removeClosedClients)
 * Sale cuando Ctrl+C pone g_running a 0.
 */
void Server::run()
{
    std::cout << "ircserv listening on port " << _port << std::endl;
    while (g_running)
    {
        updatePollEvents();
        // El UNICO poll del programa. Timeout -1 = dormir sin limite:
        // mientras nadie haga nada no hay trabajo, asi no gastamos CPU.
        // Ctrl+C interrumpe poll (devuelve -1) y salimos del bucle limpio.
        if (poll(&_fds[0], _fds.size(), -1) < 0)
            break;
        // Indice y no iterador: acceptClient() hace push_back y el vector
        // puede moverse en memoria (un iterador quedaria invalido).
        // Los fds recien añadidos tienen revents = 0 y se saltan solos.
        for (size_t i = 0; i < _fds.size(); ++i)
        {
            int     fd = _fds[i].fd;
            short   rev = _fds[i].revents;

            if (rev == 0)                   // a este fd no le ha pasado nada
                continue;
            if (fd == _listenFd)            // alguien nuevo quiere conectarse
            {
                acceptClient();
                continue;
            }
            // POLLERR = socket roto, POLLNVAL = fd no valido.
            // No tiene sentido leer ni escribir: se marca para cerrar.
            if (rev & (POLLERR | POLLNVAL))
            {
                std::map<int, Client>::iterator it = _clients.find(fd);
                if (it != _clients.end())
                    it->second.markToClose();
                continue;
            }
            // POLLIN = hay datos para leer. POLLHUP = el cliente colgo;
            // lo tratamos igual: recv devolvera 0 y handleRead lo desconecta.
            if (rev & (POLLIN | POLLHUP))
                handleRead(fd);
            // POLLOUT = ya se puede enviar sin bloquear.
            if (rev & POLLOUT)
                handleWrite(fd);
        }
        // Cierre diferido: SOLO aqui, con la vuelta terminada, se cierran
        // los clientes marcados. Nadie esta usando ya sus punteros.
        removeClosedClients();
    }
}

/**
 * @details Pedimos POLLOUT solo a quien tiene algo pendiente de enviar.
 * Un socket casi siempre "se puede escribir": si pidieramos POLLOUT a todos,
 * poll volveria al instante en cada vuelta y el server iria al 100% de CPU.
 * Empieza en i = 1 porque [0] es el fd de escucha (solo necesita POLLIN).
 * revents se limpia para no arrastrar lo que paso en la vuelta anterior.
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
 * @details accept() devuelve un fd NUEVO, solo para este cliente. El de
 * escucha sigue libre para el siguiente. Ese fd nuevo se mete en _fds (para
 * que poll lo vigile) y en _clients (su ficha con buffers y datos).
 * Si algo falla, se ignora: un cliente fallido no puede tumbar el servidor.
 */
void Server::acceptClient()
{
    struct sockaddr_in  addr;       // accept la rellena con la IP del cliente
    socklen_t           len = sizeof(addr);
    int fd = accept(_listenFd, reinterpret_cast<struct sockaddr *>(&addr), &len);
    if (fd < 0)
        return;
    // Sin O_NONBLOCK, un recv o send a este cliente podria quedarse
    // esperando y congelar el server para TODOS los demas.
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
    Client client(fd);
    client.setHostname(inet_ntoa(addr.sin_addr));

    _clients.insert(std::make_pair(fd, Client(fd)));
    std::cout << "[fd " << fd << "] connected from "
              << inet_ntoa(addr.sin_addr) << std::endl;
}

/**
 * @details Un solo recv por vuelta: poll ya dijo que hay datos, asi que no
 * bloquea. Si llego mas de 1024 bytes, el resto se lee en la siguiente vuelta.
 *
 * TCP NO envia "mensajes", envia un chorro de bytes. Un recv puede traer
 * medio comando, uno entero o varios juntos. Por eso todo se guarda en el
 * buffer del cliente y solo se procesan las lineas que ya estan completas.
 */
void Server::handleRead(int fd)
{
    // Buscamos la ficha del cliente. Si ya esta marcado para cerrar,
    // no leemos nada mas de el.
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end() || it->second.shouldClose())
        return;
    Client &client = it->second;

    char    buf[1024];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    // n == 0 -> el cliente cerro la conexion (Ctrl+C en nc, /quit...).
    // n <  0 -> error de red. El subject no deja mirar errno: desconectamos.
    if (n <= 0)
    {
        disconnect(client, n == 0 ? "connection closed" : "recv error");
        return;
    }

    // Guardamos los bytes en el buffer de entrada. Lo que no forme todavia
    // una linea completa se queda ahi esperando al siguiente recv.
    client.appendInput(buf, static_cast<size_t>(n));

    // Puede haber 0, 1 o varias lineas completas: las sacamos de una en una.
    // Si un comando desconecta al cliente (QUIT), paramos de procesar.
    std::string line;
    while (!client.shouldClose() && client.extractLine(line))
    {
        // Maximo IRC = 512 bytes con "\r\n" -> 510 de texto. Una linea mas
        // larga no se descarta: se recorta y se procesa igual.
        if (line.size() > 510)
            line.erase(510);
        if (!line.empty())
            handleLine(client, line);
    }

    // Lo que queda en el buffer NO tiene '\n'. Si ya pasa de 512 bytes,
    // nunca sera una linea valida: el cliente manda basura sin fin.
    if (client.inputSize() > IRC_MAX_INPUT)
        disconnect(client, "input too long");
}

/**
 * @details Un solo send por vuelta con todo lo pendiente. poll dijo POLLOUT,
 * asi que no bloquea. send puede enviar MENOS bytes de los pedidos (si el
 * cliente va lento): devuelve cuantos salieron, y solo esos se borran del
 * buffer. El resto se queda en _outBuffer y se envia en otra vuelta.
 */
void Server::handleWrite(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end() || !it->second.hasOutput())
        return;
    Client &client = it->second;

    const std::string &out = client.getOutput();
    ssize_t n = send(fd, out.c_str(), out.size(), 0);
    if (n < 0)                      // error de red (p. ej. el cliente ya cerro)
    {
        disconnect(client, "send error");
        return;
    }
    client.consumeOutput(static_cast<size_t>(n));   // borra solo lo que salio
}

/**
 * @details La PUERTA entre la red y los comandos. Recibe UNA linea limpia
 * (sin "\r\n"), la trocea con el parser y se la pasa al dispatcher, que
 * llama al handler del comando (NICK, JOIN, PRIVMSG...).
 * Se llama una vez por cada linea completa desde handleRead().
 */
void Server::handleLine(Client &client, const std::string &line)
{
    //DEBUG
    std::cout << "[fd " << client.getFd() << "] line: \"" << line << "\"" << std::endl;

	Instruction instruction ;
	std::string err_msg;

    if (!Parser::parse(line, instruction, err_msg))
        return;

    // ver cómo queda troceada la línea DEBUG
    std::cout << "    command: [" << instruction.command << "]" << std::endl;
    for (size_t i = 0; i < instruction.parameters.size(); ++i)
        std::cout << "    param[" << i << "]: [" << instruction.parameters[i] << "]" << std::endl;

    dispatcher(&client, instruction.command, instruction.parameters);
}

/* ========================== Cierre diferido ========================== */

/**
 * @details Cierre diferido: aqui, y SOLO aqui, desaparecen los clientes
 * marcados con disconnect(). Se llama al FINAL de cada vuelta de run().
 *
 * Orden para cada cliente marcado:
 *   1. cleanupClient: avisar a sus canales y sacarle de ellos
 *      (antes de borrarle, mientras sus datos aun existen)
 *   2. close(fd):     cerrar el socket
 *   3. borrarle de _clients y de _fds
 *
 * Se recorre de atras hacia delante: al hacer erase de _fds[i], los
 * elementos de detras se desplazan, pero esos ya los hemos mirado.
 * Nunca toca [0], que es el fd de escucha.
 */
void Server::removeClosedClients()
{
    for (size_t i = _fds.size(); i-- > 1; )         // de atras a delante, sin tocar [0]
    {
        int fd = _fds[i].fd;
        std::map<int, Client>::iterator it = _clients.find(fd);
        if (it == _clients.end() || !it->second.shouldClose())
            continue;
        cleanupClient(&it->second); 
        close(fd);
        _clients.erase(it);
        _fds.erase(_fds.begin() + i);
    }
}

/* =================== API de red para los comandos =================== */

/**
 * @details NO envia nada: solo guarda la linea (con "\r\n") en el buffer de
 * salida del cliente. Al tener salida pendiente, updatePollEvents() pedira
 * POLLOUT y el send real lo hara handleWrite() cuando poll lo permita.
 */
void Server::sendTo(Client &client, const std::string &line)
{
    client.queueOutput(line + "\r\n");
}

/**
 * @details NO cierra nada: solo marca al cliente. El cierre real lo hace
 * removeClosedClients() al final de la vuelta (cierre diferido), para que
 * nadie se quede con un Client* que apunte a algo ya borrado.
 * Si ya estaba marcado, no hace nada (evita repetir el log).
 * Ojo: lo que quede en su _outBuffer NO llega a enviarse, porque el socket
 * se cierra en esta misma vuelta, antes del siguiente POLLOUT.
 */
void Server::disconnect(Client &client, const std::string &reason)
{
    if (client.shouldClose())
        return;
    std::cout << "[fd " << client.getFd() << "] disconnect: " << reason << std::endl;
    client.markToClose();
}

/**
 * @details Recorre todos los clientes (con pocos clientes es suficiente).
 * Compara en minusculas porque en IRC "Bob" y "bob" son el mismo nick.
 * Ignora a los marcados para cerrar: para los comandos ya no existen.
 * Devuelve NULL si nadie tiene ese nick.
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

void Server::cleanupClient(Client *client)
{
    // Motivo: el de QUIT si lo hubo; si no (Ctrl+C, error de red...), genérico
    std::string reason = client->getQuitReason();
    if (reason.empty())
        reason = "Connection closed";

    // COPIA de los nombres de canal: leaveChannel modifica _clientChannels
    // (y puede liberar canales), así que no se puede recorrer directamente.
    // De paso se juntan los destinatarios del aviso (set = sin repetidos).
    std::vector<std::string> names;
    std::set<Client *> recipients;

    const std::map<std::string, Channel *> &channels = client->getChannels();
    for (std::map<std::string, Channel *>::const_iterator cit = channels.begin();
         cit != channels.end(); ++cit)
    {
        names.push_back(cit->first);

        const std::map<int, Client *> &users = cit->second->getChUsers();
        for (std::map<int, Client *>::const_iterator uit = users.begin();
             uit != users.end(); ++uit)
        {
            if (uit->second == client)          // sin aviso a quien se va
                continue;
            if (uit->second->shouldClose())     // ya se está cerrando
                continue;
            recipients.insert(uit->second);
        }
    }

    // 1. AVISO (antes de limpiar: después ya no hay de dónde sacar los datos).
    // Si no estaba en ningún canal (p. ej. sin registrar), no hay a quién avisar.
    if (!recipients.empty())
    {
        const std::string msg = ":" + client->getPrefix() + " QUIT :" + reason + "\r\n";
        for (std::set<Client *>::iterator rit = recipients.begin(); rit != recipients.end(); ++rit)
            (*rit)->queueMessage(msg);
    }

    // 2. SALIR DE SUS CANALES (los vacíos se borran dentro de leaveChannel)
    for (size_t i = 0; i < names.size(); ++i)
    {
        Channel *channel = findChannel(names[i]);
        if (channel)
            leaveChannel(client, channel);
    }

    // 3. INVITACIONES: puede estar invitado a canales donde NO entró, y esos
    // canales guardan un puntero a él. Hay que mirar TODOS los canales.
    for (std::map<std::string, Channel *>::iterator it = _channels.begin();
         it != _channels.end(); ++it)
        it->second->removeInvite(client);
}