/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vrads <vrads@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/13 00:51:04 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/18 14:37:12 by vrads            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"
#include <iostream>

/**
 * @brief Implementación de depuración deshabilitada para mostrar una instrucción.
 */

/*
*void print_instruction (const Instruction & instruction )
*{
*	if (instruction .command.empty() == true)
*		return;
*	std::cout << "Command: " << instruction .command << std::endl;
*	if (instruction .parameters.empty() == true)
*		return;
*	std::vector<std::string>::const_iterator it = instruction .parameters.begin();
*	std::vector<std::string>::const_iterator end = instruction .parameters.end();
*	std::cout << "Parameters: ";
*	while (it != end)
*	{
*		if (it != instruction .parameters.begin())
*			std::cout << ", ";
*		std::cout << "\"" << *it << "\"";
*		++it;
*	}
*	std::cout << std::endl;
*}
*/

/**
 * @brief Convierte un token a mayúsculas.
 * @param[in,out] token Token que se modifica directamente.
 */
static void to_upper(std::string& token)
{
	for (std::string::size_type i = 0; i < token.size(); ++i)
		token[i] = std::toupper(static_cast<unsigned char>(token[i]));
}

/**
 * @brief Comprueba si un token puede representar un comando IRC.
 *
 * Acepta cadenas formadas solo por letras o códigos numéricos de exactamente
 * tres cifras. No permite mezclar letras y números.
 *
 * @param[in] token Token que se desea validar.
 * @return true si el token es alfabético o un código numérico de tres cifras;
 * false en caso contrario.
 */
static bool command_candidate(std::string& token)
{
	bool isText = false;
	bool isNumber = false;
	
	if (token.empty() == true)
		return (false);
	for (std::string::size_type i = 0; i < token.size(); ++i)
	{
		if (std::isalpha(static_cast<unsigned char>(token[i])))
		{
			if (isNumber == true) // No puede ser alfanumérico
				return (false);
			if (isText == false)
				isText = true;
		}
		else if (std::isdigit(static_cast<unsigned char>(token[i])))
		{
			if (isText == true || i > 2) // Si es número no puede ser un código de más de 3 cifras
				return (false);
			if (isNumber == false)
				isNumber = true;
		}
		else
			return (false); // solo puede haber letras o números
	}
	if (isNumber == true && token.size() < 3) // Si es un número inferior a tres cifras
		return (false);
	return (true);
}

/**
 * @brief Analiza una línea IRC y extrae su comando y parámetros.
 * 
 * Si la línea comienza con un prefijo, este se omite. Cuando encuentra ':'
 * después del comando, conserva el resto de la línea como un único parámetro.
 *
 * @param[in] input Línea IRC sin el terminador CRLF.
 * @param[out] instruction Instrucción analizada si la operación tiene éxito.
 * @param[out] err_msg Mensaje de error si falta el comando o se supera el
 * límite de 15 parámetros.
 * @return true si la línea se ha analizado correctamente; false si no contiene
 * un comando válido o tiene más de 15 parámetros.
 */
bool Parser::parse(const std::string& input, Instruction& instruction , std::string& err_msg)
{
	Instruction parsed;
	bool prefixFound = false;
	bool commandFound = false;
	std::string::size_type token_start = 0;
	std::string::size_type token_end = 0;
	std::string token = "";
	
	while (token_end < input.size())
	{
		token_start= input.find_first_not_of(" ", token_end);
		if (token_start == std::string::npos)
			break;
		else if (*(input.begin() + token_start) == ':' && commandFound == true)
		{
			if (token_start < input.size())
				token_start += 1;
			token_end = input.size();
		}
		else
			token_end = input.find(" ", token_start);

		if (token_start == token_end)
			token = "";
		else if (token_end == std::string::npos)
			token = input.substr(token_start, input.size() - token_start);
		else
			token = input.substr(token_start, token_end - token_start);

		if (commandFound == false && token.empty() == false)
		{
			if (prefixFound == true || *token.begin() != ':')
			{
				if (command_candidate(token) == false)
					break;
				else
					to_upper(token); //aqui ya le mete cesar UPPERCASE 
				parsed.command = token;
				commandFound = true;
			}
			else
			{
				if (token.size() == 1) // trailing parameter solo contiene ':', que no es válido
					break;
				prefixFound = true;
			}
		}
		else
		{
			parsed.parameters.push_back(token);
			if (parsed.parameters.size() > 15)
			{
				err_msg = "Appropiate err_msg";
				return (false);
			}
		}
	}
	if (parsed.command.empty() == true)
	{
		err_msg = "Appropiate err_msg";
		return (false);
	}
	instruction = parsed;
	return (true);
}
