/* ************************************************************************** */
/*																			*/
/*														:::	  ::::::::   */
/*   main.cpp										   :+:	  :+:	:+:   */
/*													+:+ +:+		 +:+	 */
/*   By: ccarro-d <ccarro-d@student.42madrid.com	+#+  +:+	   +#+		*/
/*												+#+#+#+#+#+   +#+		   */
/*   Created: 2026/11/17 20:27:27 by ccarro-d		  #+#	#+#			 */
/*   Updated: 2026/11/17 20:27:55 by ccarro-d		 ###   ########.fr	   */
/*																			*/
/* ************************************************************************** */

#include "Parser.hpp"

#include <iostream>

/**
 * @brief Imprime una instrucción analizada en un formato legible.
 * @param[in] instruction Instrucción que se mostrará por la salida estándar.
 */
void print_instruction(const Instruction& instruction);

/**
 * @brief Ejecuta una prueba manual del parser con la línea recibida por argv.
 * @param argc Número de argumentos del programa; debe ser 2.
 * @param argv Argumentos del programa; argv[1] contiene la línea IRC.
 * @return 0 tras ejecutar el parser; 1 si no se proporciona una única línea.
 */
int main(int argc, char **argv)
{
	if (argc != 2)		
	{
		std::cerr << "Usage: ./parser_test \"IRC message\"" << std::endl;
		return (1);
	}
	Instruction instruction;
	std::string err_msg;
	bool parsing = Parser::parse(argv[1], instruction, err_msg);
	if (parsing == true)
		print_instruction(instruction);
	else
		std::cerr << "Error: " << err_msg << std::endl;
	return (0);
}
