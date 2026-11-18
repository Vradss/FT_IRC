/* ************************************************************************** */
/*  [PARSER FAKE] - TEMPORAL. Ver includes/ParserFake.hpp                     */
/* ************************************************************************** */

#include "ParserFake.hpp"

/*
 * Formato de un mensaje IRC (RFC 1459, seccion 2.3.1):
 *
 *   [:prefijo] COMANDO param1 param2 ... [:trailing con espacios]
 *
 * Reglas que aplicamos:
 *   1. Si empieza por ':', es un prefijo (nick!user@host): un cliente no
 *      deberia mandarlo, asi que lo saltamos.
 *   2. La primera palabra es el comando.
 *   3. Las siguientes palabras son params, separadas por uno o MAS espacios.
 *   4. Si una palabra empieza por ':', TODO lo que queda de linea es un unico
 *      param (el "trailing"), sin el ':' y con sus espacios.
 *      Por eso funcionan  USER ... :Aurora Moreno  y  PRIVMSG #a :hola que tal
 *   5. Maximo 15 params (RFC): el resto se ignora.
 */

static const size_t MAX_PARAMS = 15;

// Avanza i mientras haya espacios. Devuelve la nueva posicion.
static size_t skipSpaces(const std::string &s, size_t i)
{
    while (i < s.size() && s[i] == ' ')
        ++i;
    return i;
}

// Lee una palabra desde i hasta el siguiente espacio. Deja i despues de ella.
static std::string readWord(const std::string &s, size_t &i)
{
    size_t start = i;
    while (i < s.size() && s[i] != ' ')
        ++i;
    return s.substr(start, i - start);
}

bool Parser::parse(std::string &line, Instruction &out, std::string &err)
{
    out.command.clear();
    out.params.clear();
    err.clear();

    size_t i = skipSpaces(line, 0);

    // Regla 1: saltar el prefijo
    if (i < line.size() && line[i] == ':')
    {
        readWord(line, i);
        i = skipSpaces(line, i);
    }

    // Regla 2: el comando
    out.command = readWord(line, i);
    if (out.command.empty())
    {
        err = "empty line";
        return false;
    }

    // Reglas 3, 4 y 5: los parametros
    while (out.params.size() < MAX_PARAMS)
    {
        i = skipSpaces(line, i);
        if (i >= line.size())
            break;
        if (line[i] == ':')                        // trailing
        {
            out.params.push_back(line.substr(i + 1));
            break;
        }
        out.params.push_back(readWord(line, i));
    }
    return true;
}
