/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/13 00:51:04 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/17 02:54:23 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"
#include <iostream>

void print_instruction (const Instruction & instruction )
{
	if (instruction .command.empty() == true)
		return;
	std::cout << "Command: " << instruction .command << std::endl;
	if (instruction .parameters.empty() == true)
		return;
	std::vector<std::string>::const_iterator it = instruction .parameters.begin();
	std::vector<std::string>::const_iterator end = instruction .parameters.end();
	std::cout << "Parameters: ";
	while (it != end)
	{
		if (it != instruction .parameters.begin())
			std::cout << ", ";
		std::cout << "\"" << *it << "\"";
		++it;
	}
	std::cout << std::endl;
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
 * un comando o tiene más de 15 parámetros.
 */
bool Parser::parse(const std::string& input, Instruction& instruction , std::string& err_msg)
{
	Instruction parsed;
	bool prefixFound = false;
	bool commandFound = false;
	std::string::size_type token_start = 0;
	std::string::size_type token_end = 0;
	std::string token = "";
	
	while (token_end < input.length())
	{
		token_start= input.find_first_not_of(" ", token_end);
		if (token_start == std::string::npos)
			break;
		else if (*(input.begin() + token_start) == ':' && commandFound == true)
		{
			if (token_start < input.length())
				token_start += 1;
			token_end = input.length();
		}
		else
			token_end = input.find(" ", token_start);

		if (token_start == token_end)
			token = "";
		else if (token_end == std::string::npos)
			token = input.substr(token_start, input.length() - token_start);
		else
			token = input.substr(token_start, token_end - token_start);

		if (commandFound == false && token.empty() == false)
		{
			if (prefixFound == true || *token.begin() != ':')
			{
				parsed.command = token;
				commandFound = true;
			}
			else
				prefixFound = true;
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
