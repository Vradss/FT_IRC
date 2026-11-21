/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Instruction.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 00:57:50 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/17 01:05:28 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <string>
#include <vector>

/**
 * @brief Resultado estructurado de analizar una línea de comando IRC.
 */
struct Instruction 
{
	std::string command;                  ///< Nombre del comando IRC.
	std::vector<std::string> parameters; ///< Parámetros en el orden recibido.
};

#endif
