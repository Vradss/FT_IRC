/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dispatcher.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/15 12:02:18 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/18 21:40:36 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Client.hpp"

/*
 * TODO (ver con César porq esto no lo estoy haciendo) 
 * El dispatcher procesa una línea ya parseada y no sabe nada del buffer
 * Quien lee del socket debe:
 *   - Acumular lo recibido en el buffer de entrada del cliente (TCP es un
 *     flujo de bytes, no de mensajes: un recv puede traer medio comando o varios).
 *   - Mientras haya líneas COMPLETAS (hasta \r\n): parsear y llamar a dispatcher.
 *   - Dejar en el buffer lo que sobre sin terminar para el siguiente recv.
 *   - Dejar de procesar líneas si el cliente queda marcado "a desconectar".
 *   - Rodear cada llamada a dispatcher con try/catch de std::bad_alloc
 *     (una sola captura aquí, no una en cada handler).
 *   - Borrar a los clientes marcados al terminar de procesar el buffer,
 *     nunca dentro de un handler.
 */

// esto borrar porque igual lo hizo césar 
static std::string toUpperStr(const std::string &s)
{
    std::string result(s);
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
    return result;
}


// Comandos que se admiten ANTES de estar registrado. TO-DO: ver que son todos correctos
static bool isPreRegistrationCommand(const std::string &cmd)
{
    return cmd == "PASS" || cmd == "NICK" || cmd == "USER"
        || cmd == "CAP" || cmd == "PING" || cmd == "QUIT";
}



//check esto con César para ver qué me llega 
//esto habrá que llamarlo en el código que lee el socket 
// se acumula en el buffer de entrada 
// en teoria: hay una linea completa, se parsea y se despacha
// repetimos este proceso mientras queden lineas COMPLETAS en el buffer 
// esto es porque en un solo uso de recv pueden vernir varios comandos a la vez 
// esto es porque TCP no tiene un concepto de mensajes o cmds, es un flujo continuo de bytes 
// tonces lo que sobra se queda en el buffer 
/*
    el "problema" con esto es que el cliente puede dejar de existir despues del handler
    Es decir, al venir varios comandos a la vez si primero viene un QUIT o hay un error en alguno de los comandos que 
    pueda borrar al cliente, hay que asegurarse que ni el dispatcher ni el bucle de lectura usen el puntero sin comprobar que sigue vivo
    Ejemplo: si el primer comando desconecta al cliente, los demás no pueden ejecutarse
*/
/*
    para la memoria ver bad_alloc             
*/
void Server::dispatcher(Client *client, const std::string &command, const std::vector<std::string> &params)
{
    //en teoria el nombre del comando ya vendría normalizado
    //ignorar las lineas vacías? 
    //si el cliente no está registrado solo admitimos los comadnos de registro
    //en teoria esto lo checka join pero igual es mejor hacerlo aqui para no tener que hacerlo por cada comando?
    //buscamos ya el comando en concreto y llamamos a su handler correspondiente 
    /*LANZAR LOS ERRORES ADECUADOS*/
    
    //si no hay cliente F 
    if (!client)
        return;

    // Si un comando anterior (QUIT, error grave, KICK...) ya marcó al client como desconectado
    //no se ejecuta nada más de lo que quede en su buffer 
    if (client->isMarkedForDisconnect())
        return;

    // Línea vacía => seguimos 
    if (command.empty())
        return;

    /*ESTO VERLO CON CÉSAR QUE IGUAL LO HACE ÉL */
    const std::string cmd = toUpperStr(command);

    //aqui pongo bien el formto y tal para si el user no está registrado 
    // TO-DO: tengo que quitar del joinChannel esto 
    const std::string srv = ":" + _serverName + " ";
    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    // comprubeo aqui si el user no está registrado por 
    //y si no es un comando admitido, rollo intenta hacer JOIN y no hizo NICK 
    // lo hago aqui una vez y no en cada handler

    
    if (!client->isRegistered() && !isPreRegistrationCommand(cmd))
    {
        client->queueMessage(srv + "451 " + nick + " :You have not registered\r\n");
        return;
    }
    
   
    //toda esta parte no sé si la meteré mejor en un switch que se vea más mono 
    // TO-DO: VER SI SE ME HA OLVIDADO ALGUN COMANDO
    // TO-DO: PRIVMESSAGE Y PART
    if (cmd == "JOIN")
        handleJoin(client, params); 
    else if (cmd == "PASS")   
        handlePass(client, params);
    else if (cmd == "NICK")   
        handleNick(client, params); 
    else if (cmd == "USER")   
        handleUser(client, params);
    // else if (cmd == "INVITE") handleInvite(client, params); *
    // else if (cmd == "TOPIC")  handleTopic(client, params); *
    // else if (cmd == "MODE")   handleMode(client, params);
    else if (cmd == "KICK")   
        handleKick(client, params); 
    else if (cmd == "PRIVMSG")   
        privateMessage(client, params); 
    else if (cmd == "PART")   
        handlePart(client, params);
    else if (cmd == "QUIT")   
        handleQuit(client, params);
    else if (cmd == "PING")   
        handlePing(client, params);
    else
        client->queueMessage(srv + "421 " + nick + " " + command + " :Unknown command\r\n");
}