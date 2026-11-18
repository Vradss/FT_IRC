#ifndef PARSERFAKE_HPP
#define PARSERFAKE_HPP

/* ************************************************************************** */
/*  [PARSER FAKE] - TEMPORAL                                                  */
/*                                                                            */
/*  Parser de prueba mientras César sube el suyo (en el repo solo estan los   */
/*  binarios srcs/parser/a.out y parser_test, sin codigo fuente).             */
/*                                                                            */
/*  Imita la interfaz que se ve en su binario:                                */
/*      Parser::parse(std::string &line, Instruction &out, std::string &err)  */
/*                                                                            */
/*  CUANDO LLEGUE EL PARSER DE CÉSAR:                                         */
/*    1. Borrar includes/ParserFake.hpp y srcs/ParserFake.cpp                 */
/*    2. En Server.cpp cambiar  #include "ParserFake.hpp"  por el suyo        */
/*    3. En el Makefile cambiar ParserFake.cpp por su .cpp                    */
/*    4. Revisar el bloque [INTEGRACION] de Server::handleLine()              */
/* ************************************************************************** */

#include <string>
#include <vector>

/**
 * @brief Un comando IRC ya troceado.
 *
 * Ejemplo:  "USER aurora 0 * :Aurora Moreno"
 *     command = "USER"
 *     params  = ["aurora", "0", "*", "Aurora Moreno"]
 *                                     ^^^^^^^^^^^^^ trailing: sin ':' y con espacios
 */
struct Instruction
{
    std::string                 command;
    std::vector<std::string>    params;
};

class Parser
{
    public:
        /**
         * @brief Trocea una linea (ya sin \r\n) en command + params.
         * @param line  Linea completa que saca Client::extractLine().
         * @param out   Se rellena con el comando y sus parametros.
         * @param err   Si devuelve false, explica por que (solo para depurar).
         * @return true si hay un comando; false si la linea no sirve.
         */
        static bool parse(std::string &line, Instruction &out, std::string &err);
};

#endif
