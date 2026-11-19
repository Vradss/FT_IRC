#include "Client.hpp"

/**
 * @file Client.cpp
 * @brief Implementacion de Client: solo manipula strings, ninguna llamada de red.
 *
 * @verbatim
 *  Server::handleRead()   -> appendInput() -> extractLine() (en bucle) / inputSize()
 *  Server::sendTo()       -> queueOutput()
 *  Server::updatePollEvents() -> hasOutput()
 *  Server::handleWrite()  -> getOutput() -> consumeOutput()
 *  Server::disconnect()   -> markToClose()
 *  Server::removeClosedClients() -> shouldClose()
 * @endverbatim
 */

/** @details Llamada por: Server::acceptClient(). _toClose empieza en false. */
/*
 * [INTEGRACION] Faltaba inicializar los bools que añadió Aurora (Persona B).
 * En C++ un bool sin inicializar tiene BASURA: a veces true, a veces false.
 * Sintomas que daba:
 *   - _welcomed = true  -> nunca se mandaba el 001 (irssi no conecta)
 *   - _markedForDisconnect = true -> el dispatcher ignoraba TODOS los comandos
 *   - _authenticated = true -> se podia registrar sin PASS
 */
Client::Client(int fd)
    : _fd(fd), _toClose(false),
      _authenticated(false), _markedForDisconnect(false), _welcomed(false)  // [INTEGRACION]
{}

int Client::getFd() const { return _fd; }

const std::string &Client::getNick() const { return _nick; }
void Client::setNick(const std::string &nick) { _nick = nick; }

/**
 * @details append(data, len) y no append(data): el buffer de recv NO acaba
 * en '\0', asi que hay que decir cuantos bytes copiar.
 * Llamada por: Server::handleRead().
 */
void Client::appendInput(const char *data, size_t len)
{
    _inBuffer.append(data, len);
}

/**
 * @details Busca el primer '\n', copia lo anterior a line y lo borra del
 * buffer (incluido el '\n'). Si la linea acaba en '\r' tambien lo quita.
 * Asi aceptamos "\n" solo (nc sin -C) y "\r\n" (clientes IRC).
 * Lo que va despues del '\n' se queda para la siguiente llamada.
 * Llamada por: Server::handleRead(), en bucle hasta que devuelva false.
 */
bool Client::extractLine(std::string &line)
{
    std::string::size_type pos = _inBuffer.find('\n');
    if (pos == std::string::npos)
        return false;
    line = _inBuffer.substr(0, pos);
    _inBuffer.erase(0, pos + 1);
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
    return true;
}

size_t Client::inputSize() const { return _inBuffer.size(); }

void Client::queueOutput(const std::string &data) { _outBuffer += data; }
bool Client::hasOutput() const { return !_outBuffer.empty(); }
const std::string &Client::getOutput() const { return _outBuffer; }
/**
 * @details Borra solo los n primeros bytes: si send() fue parcial, el resto
 * sigue en _outBuffer y hasOutput() hara que se pida POLLOUT otra vez.
 * Llamada por: Server::handleWrite().
 */
void Client::consumeOutput(size_t n) { _outBuffer.erase(0, n); }

void Client::markToClose() { _toClose = true; }
bool Client::shouldClose() const { return _toClose; }


//aurora 
void Client::addChannel(const std::string &name, Channel *channel)
{
    if (!channel)
        return;
    _clientChannels[name] = channel;
}

// Prefijo IRC del cliente: nick!user@host
// Devuelve por valor porque se construye en el momento (no hay nada guardado que se pueda devolver por referencia)
std::string Client::getPrefix() const
{
    return _nick + "!" + _username + "@" + _hostname;
}

// SOLO añade el mensaje al buffer de salida. NO llama a send() 
// el envío lo hace el server cuando poll() marque el fd como POLLOUT ??? 
void Client::queueMessage(const std::string &msg)
{
    //_outBuffer += msg; TO-DO !!!!
    queueOutput(msg);
}


/* METODOS PARA VER QUE USER ESTÁ CONECTAU, REGISTRAU Y TAL */
bool Client::isRegistered() const
{
    // esto lo compruebo así en vez de tener un booleano _registered para evitar desincronizaciones 
    //si el cliente tiene un nick vacio o asi o falla un USER 
    // TO-DO: mirar si tengo que hacer más comprobaciones aqui o si esto es necesario o es fumada 
    return _authenticated && !_nick.empty() && !_username.empty();
}

bool Client::isMarkedForDisconnect() const
{
    return _markedForDisconnect;
}

void Client::markForDisconnect()
{
    _markedForDisconnect = true;
}

/**
 * métodos para comprobar si está auth
 * (esto servirá para el nick y user para que nos deje registrarnos)
 */
bool Client::isAuthenticated() const
{
    return _authenticated;
}

/*
    una vez la contraseña sea correcta o no, le marcamos el value para saber 
*/
void Client::setAuthenticated(bool value)
{
    _authenticated = value;
}

/**METODOS PARA SABER SI HACEMOS LA BIENVENIDA O NO */
bool Client::hasWelcomed() const
{
    return _welcomed;
}

void Client::setWelcomed(bool value)
{
    _welcomed = value;
}


// Solo lectura: lo usa handleNick para saber a quién avisar de que alguien se ha modificado el nick
// _clientChannels es std::map<std::string, Channel*> 
const std::map<std::string, Channel *> &Client::getChannels() const
{
    return _clientChannels;
}


const std::string &Client::getUsername() const
{
    return _username;
}

void Client::setUsername(const std::string &username)
{
    _username = username;
}

void Client::setRealname(const std::string &realname)
{
    _realname = realname;
}

void Client::removeChannel(const std::string &name)
{
    _clientChannels.erase(name);
}

void Client::setQuitReason(const std::string &reason)
{
    _quitReason = reason;
}

const std::string &Client::getQuitReason() const
{
    return _quitReason;
}

void Client::setHostname(const std::string &hostname)
{
    _hostname = hostname;
}