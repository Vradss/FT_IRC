/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dispatcher_utils.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aumoreno < aumoreno@student.42madrid.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/14 11:47:50 by aumoreno          #+#    #+#             */
/*   Updated: 2026/11/15 13:31:22 by aumoreno         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
    un fichero que tendrá los métodos rollo handleJoin, el switch de los comandos, etc
    estos métodos creo que estarán todos en la clase server.hpp
*/

#include "Server.hpp"
#include <vector>
#include <string>

static std::vector<std::string> splitByComma(const std::string &s)
{
    std::vector<std::string> result;
    std::size_t start = 0;

    while (true)
    {
        std::size_t pos = s.find(',', start);
        if (pos == std::string::npos)
        {
            result.push_back(s.substr(start));
            break;        }
        result.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return result;
}

//en params tendré tanto los channels como los keys 
// asi es mas abstracto params[0] channles params[1] keys
//los errores los encolo asi que void 
//primero miramos si el primer parámetro es 0
//si es 0 => haremos el part, no seguimos con el código
//si no, recorremos lista de channels y llamamos a joinChannel pasandole el user, canal y key
//si no hay key, pasamos string vacio y joinChannel se encarga de ver si es correcto o no 
/*LANZAR LOS ERRORES ADECUADOS*/
//NOTA: CONSULTAR CON CÉSAR COMO LLEGA TODO Y MODIFICARLO ACORDE 
void Server::handleJoin(Client *client, const std::vector<std::string> &params)
{
    const std::string srv = ":" + _serverName + " ";

    // antes de registrarse el nick puede estar vacío: el protocolo usa "*"
    // comprobar si esto es correcto 
    // el caso sería: JOIN #channelName y el cliente aun no tiene nickname 
    std::string nick = client->getNick();
    if (nick.empty())
        nick = "*";

    /* 
    YA NO HACE FALTA PORQUE LO HAGO EN EL DISPATCHER 
    // validamos si el usuario tiene un nickname 
    if (!client->isRegistered())
    {
        client->queueMessage(srv + "451 " + nick + " :You have not registered\r\n");
        return;
    }
    */
    // validamos si viene el nombre del channel o channels a los que el usuario se quiera unir 
    if (params.empty())
    {
        client->queueMessage(srv + "461 " + nick + " JOIN :Not enough parameters\r\n");
        return;
    }

    // JOIN 0 -> salir de todos los canales
    // IMPORTANTE: usar una copia de la lista de nombres de canal porque si uso la original
    // y voy borrando, los iteradores son inválidos y peta 
    // asi aunque la lista original cambie cada vez que quito un canal, no peta **
    if (params[0] == "0")
        return; //TO-DO: PONER AQUI LA FUNCION DE LIMPIAR DEL PART 

    // spliteamos canales y keys y los asociamos con su key correspondiente 
    const std::vector<std::string> channels = splitByComma(params[0]);
    std::vector<std::string> keys;
    if (params.size() > 1) // si hay keys spliteamos, si debiera de haber keys y no hay ya luego lanzaré el error 
        keys = splitByComma(params[1]);

    // un joinChannel por cada canal
    for (size_t i = 0; i < channels.size(); ++i)
    {
        // huecos tipo "#a,,#b": se saltan, pero el índice sigue contando
        // para que las keys sigan alineadas con su canal
        if (channels[i].empty())
            continue;

        // sin key en esa posición -> string vacío
        std::string key;
        if (i < keys.size())
            key = keys[i];

        int result = joinChannel(client, channels[i], key);

        //lanzamos los errores
        switch (result)
        {
            case 403:
                client->queueMessage(srv + "403 " + nick + " " + channels[i] + " :No such channel\r\n");
                break;
            case 471:
                client->queueMessage(srv + "471 " + nick + " " + channels[i] + " :Cannot join channel (+l)\r\n");
                break;
            case 473:
                client->queueMessage(srv + "473 " + nick + " " + channels[i] + " :Cannot join channel (+i)\r\n");
                break;
            case 475:
                client->queueMessage(srv + "475 " + nick + " " + channels[i] + " :Cannot join channel (+k)\r\n");
                break;
            default:
                //no hacemos nada, pasamos al siguiente canal porque ya se ha encolado el error arriba 
                // 0 = éxito (joinChannel ya encoló JOIN, topic y names)
                // -1 = fallo interno: no se responde nada 
                break;
        }
    }
}

