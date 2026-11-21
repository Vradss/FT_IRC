/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/13 00:32:26 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/21 16:27:48 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

#include "Instruction.hpp"

class Parser
{
	public:
		static bool parse(const std::string& input, Instruction& instruction, std::string& err_msg);
};

#endif
