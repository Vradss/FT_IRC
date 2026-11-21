/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ccarro-d <ccarro-d@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/13 00:32:26 by ccarro-d          #+#    #+#             */
/*   Updated: 2026/11/17 02:58:30 by ccarro-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

#include "Instruction.hpp"

/**
 * @brief Convierte líneas del protocolo IRC en instrucciones estructuradas.
 */
class Parser
{
	public:
		/**
		 * @brief Analiza una línea IRC y extrae su comando y parámetros.
		 *
		 * Admite un prefijo inicial y un último parámetro precedido por ':', que
		 * puede contener espacios. Una instrucción puede tener hasta 15 parámetros.
		 *
		 * @param[in] input Línea IRC sin el terminador CRLF.
		 * @param[out] instruction Instrucción analizada si la operación tiene éxito.
		 * @param[out] err_msg Mensaje de error si la operación falla.
		 * @return true si la línea contiene un comando válido y como máximo 15
		 * parámetros; false en caso contrario.
		 */
		static bool parse(const std::string& input, Instruction& instruction, std::string& err_msg);
};

#endif
