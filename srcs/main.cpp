/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflorez <vflorez@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 12:46:37 by vrads             #+#    #+#             */
/*   Updated: 2026/11/21 16:32:21 by vflorez          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "Server.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cctype>
#include <csignal>
#include <exception>


volatile sig_atomic_t g_running = 1;

static void onSigint(int)
{
    g_running = 0;
}


static void setupSignals()
{
    struct sigaction sa;
    sa.sa_handler = onSigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
}



static bool parsePort(const std::string &s, int &port)
{
    if (s.empty() || s.size() > 5)
        return false;
    for (std::string::size_type i = 0; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    long value = std::strtol(s.c_str(), NULL, 10);
    if (value < 1024 || value > 65535)
        return false;
    port = static_cast<int>(value);
    return true;
}


int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }
    int port;
    if (!parsePort(argv[1], port))
    {
        std::cerr << "Error: port must be a number between 1024 and 65535" << std::endl;
        return 1;
    }
    std::string password(argv[2]);
    if (password.empty() || password.find_first_of(" \t\r\n") != std::string::npos)
    {
        std::cerr << "Error: password cannot be empty or contain spaces" << std::endl;
        return 1;
    }
    setupSignals();
    try
    {
        Server server(port, password);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
