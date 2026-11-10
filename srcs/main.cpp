#include "Server.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cctype>
#include <csignal>
#include <exception>

/**
 * @file main.cpp
 * @brief Punto de entrada: valida argumentos, prepara señales y arranca Server.
 *
 * @verbatim
 *  main()
 *   ├─ parsePort()       valida <port>
 *   ├─ (valida password)
 *   ├─ setupSignals()    Ctrl+C y SIGPIPE
 *   └─ Server(port, pass) ── run()   hasta Ctrl+C
 * @endverbatim
 */

/**
 * @brief 1 mientras el servidor debe seguir; Ctrl+C lo pone a 0.
 *
 * - volatile: el compilador debe releerla siempre (la cambia el handler,
 *   "por fuera" del flujo normal del programa).
 * - sig_atomic_t: tipo que se lee/escribe de una vez, seguro en un handler.
 * Leida por: Server::run() en el while.
 */
volatile sig_atomic_t g_running = 1;

/**
 * @brief Handler de SIGINT/SIGTERM. Solo cambia la bandera.
 *
 * En un handler no se debe hacer casi nada (ni cout, ni new): solo
 * marcar y dejar que el bucle salga solo y el destructor cierre los fds.
 */
static void onSigint(int)
{
    g_running = 0;
}

/**
 * @brief Instala los handlers de señales.
 *
 * - sigaction sin SA_RESTART: Ctrl+C interrumpe poll() (devuelve -1) y
 *   run() sale del bucle limpio.
 * - SIGPIPE ignorado: enviar a un cliente que ya cerro no mata el servidor;
 *   send() solo devuelve -1 y lo desconectamos.
 * Llamada por: main(), antes de crear Server.
 */
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

/**
 * @brief Convierte <port> a int si es valido.
 * @param s         Texto de argv[1].
 * @param[out] port Puerto si es valido.
 * @return false si no son solo digitos o esta fuera de 1024-65535
 *         (por debajo de 1024 hacen falta permisos de root).
 */
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

/**
 * @brief ./ircserv <port> <password>
 * @return 0 al salir con Ctrl+C; 1 si los argumentos o el arranque fallan.
 *
 * El try/catch recoge el runtime_error de Server::fail() (bind ocupado...).
 * Server vive dentro del try: al salir, su destructor cierra todos los fds.
 */
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
