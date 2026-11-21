/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/13 00:51:04 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/21 16:32:53 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"
#include <iostream>

static void to_upper(std::string& token)
{
	for (std::string::size_type i = 0; i < token.size(); ++i)
		token[i] = std::toupper(static_cast<unsigned char>(token[i]));
}

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
			if (isNumber == true)
				return (false);
			if (isText == false)
				isText = true;
		}
		else if (std::isdigit(static_cast<unsigned char>(token[i])))
		{
			if (isText == true || i > 2)
				return (false);
			if (isNumber == false)
				isNumber = true;
		}
		else
			return (false);
	}
	if (isNumber == true && token.size() < 3)
		return (false);
	return (true);
}


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
					to_upper(token);
				parsed.command = token;
				commandFound = true;
			}
			else
			{
				if (token.size() == 1)
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
