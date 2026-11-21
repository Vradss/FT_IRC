*This project has been created as part of the 42 curriculum by vflorez, aumoreno, ccarro-d.*

# ft_irc

## Description

**ft_irc** is an IRC (Internet Relay Chat) server written in **C++98**. The goal of the
project is to understand how a real network server works from the inside: TCP sockets,
non-blocking I/O, multiplexing many clients with a single `poll()`, and implementing a
text protocol (RFC 1459 / RFC 2812) well enough for a real IRC client to connect, register,
join channels and talk.

Overview:

- One process, one thread, **one `poll()`** for every socket (listening socket + all clients).
- All sockets are **non-blocking**; nothing ever blocks on `recv()`/`send()`.
- Each client has its own **input buffer** (TCP is a byte stream: one `recv()` can carry
  half a command or several) and its own **output buffer** (replies are queued and sent
  only when `poll()` reports `POLLOUT`).
- Clients are **never deleted inside a command handler**: they are marked and closed at the
  end of the loop turn (deferred close).
- Clean shutdown on `Ctrl+C` / `SIGTERM`; `SIGPIPE` is ignored.

### Supported commands

| Group          | Commands                          |
| -------------- | --------------------------------- |
| Registration   | `CAP`, `PASS`, `NICK`, `USER`     |
| Connection     | `PING`, `PONG`, `QUIT`            |
| Channels       | `JOIN` (incl. `JOIN 0`), `PART`, `KICK`, `INVITE`, `TOPIC` |
| Messages       | `PRIVMSG` (to a user or a channel) |
| Channel modes  | `MODE` `+i` `+t` `+k` `+o` `+l` *(work in progress)* |

Before registration only `CAP`, `PASS`, `NICK`, `USER`, `PING`, `PONG` and `QUIT` are
accepted; anything else gets `451 :You have not registered`. Unknown commands get `421`.

## Instructions

### Requirements

- A C++ compiler supporting C++98 (`c++` / `clang++` / `g++`)
- `make`
- A Unix-like OS (Linux or macOS)

### Compilation

```sh
make        # builds ./ircserv
make clean  # removes object files
make fclean # removes object files and the binary
make re     # fclean + all
```

Flags: `-Wall -Wextra -Werror -std=c++98`.

### Execution

```sh
./ircserv <port> <password>
```

- `port`: number between `1024` and `65535`.
- `password`: required by clients with `PASS`; cannot be empty or contain spaces.

Example:

```sh
./ircserv 6667 secret
```

### Connecting

With any IRC client (e.g. irssi):

```sh
irssi -c 127.0.0.1 -p 6667 -w secret -n mynick
```

Or by hand with `nc` (`-C` sends `\r\n` line endings):

```sh
nc -C 127.0.0.1 6667
PASS secret
NICK alice
USER alice 0 * :Alice
JOIN #42
PRIVMSG #42 :hello world
```

To test partial commands (TCP fragmentation), use `nc` and press `Ctrl+D` in the middle of
a line: the server keeps the fragment in the buffer until the `\r\n` arrives.

## Project structure

```
.
├── Makefile
├── includes/
│   ├── Server.hpp        server state, main loop, every command handler
│   ├── Client.hpp        one connection: buffers + identity + registration state
│   ├── Channel.hpp       members, operators, invites, topic, modes
│   ├── Parser.hpp        IRC line -> Instruction
│   └── Instruction.hpp   { command, parameters }
└── srcs/
    ├── main.cpp          argument checks, signals, creates and runs Server
    ├── Server.cpp        sockets, poll() loop, read/write, deferred close
    ├── Client.cpp
    ├── Channel.cpp
    ├── Parser.cpp
    ├── dispatcher.cpp    command name -> handler (switch)
    ├── channel_actions.cpp   TOPIC, INVITE
    └── Commands/
        ├── register.cpp  PASS, NICK, USER, welcome
        ├── cap.cpp
        ├── join.cpp / join_utils.cpp
        ├── part.cpp
        ├── kick.cpp
        ├── privmsg.cpp
        ├── ping.cpp / pong.cpp
        └── quit.cpp
```

## Execution flow

```
main()
 ├─ parsePort() / password check          invalid args -> usage error
 ├─ setupSignals()                        SIGINT/SIGTERM stop the loop, SIGPIPE ignored
 ├─ Server()  ── setupSocket()            socket, bind, listen, O_NONBLOCK
 │                └─ fail()               throws if anything goes wrong
 └─ run()  (loop until Ctrl+C)
     ├─ updatePollEvents()                POLLIN for all, POLLOUT only if output pending
     ├─ poll()                            the ONLY poll() in the program
     └─ for each ready fd:
         ├─ acceptClient()                listening fd ready -> new Client in _fds/_clients
         ├─ POLLERR | POLLNVAL            -> markToClose()
         ├─ handleRead()                  POLLIN | POLLHUP
         │    ├─ recv() <= 0              -> disconnect()
         │    ├─ Client::appendInput()
         │    ├─ Client::extractLine()    in a loop, one full line at a time
         │    │    └─ handleLine()
         │    │         ├─ Parser::parse()       -> Instruction { command, params }
         │    │         └─ dispatcher()          registration check + switch
         │    │              └─ handleXxx()      replies via Client::queueMessage()
         │    └─ buffer > 512 bytes       -> disconnect("input too long")
         ├─ handleWrite()                 POLLOUT
         │    ├─ send()  (may be partial)
         │    └─ Client::consumeOutput() / disconnect() on error
     └─ removeClosedClients()             deferred close: cleanupClient(), close(), erase
```

## Technical choices

- **Single `poll()`**: the subject forbids blocking I/O and more than one `poll()`
  (or equivalent). All reads and writes happen only after `poll()` says the fd is ready.
- **`Client` does no I/O**: it only stores buffers and user data. All network code lives in
  `Server`, so there is one place where sockets are read, written and closed.
- **Output buffering**: handlers never call `send()` directly; they queue the reply. This
  handles slow clients and partial sends without blocking.
- **Deferred close**: `disconnect()` only marks the client; `removeClosedClients()` closes
  it at the end of the turn, so no handler ever uses a deleted `Client`.
- **Line limit**: IRC lines are at most 512 bytes including `\r\n`; longer input
  disconnects the client.

## Resources

- [RFC 1459 – Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 – IRC: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [Modern IRC Client Protocol (ircdocs)](https://modern.ircdocs.horse/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- `man 2 poll`, `man 2 socket`, `man 2 accept`, `man 2 recv`, `man 2 send`, `man 2 fcntl`
- [irssi documentation](https://irssi.org/documentation/)

### Use of AI

AI (Claude) was used as a support tool, not to write the project for us:

- **Understanding concepts**: explanations of `poll()`, non-blocking sockets, TCP stream
  fragmentation and the IRC numeric replies from the RFCs.
- **Code review and debugging**: reviewing handlers for edge cases (partial lines,
  disconnects during a command, channel operator succession) and helping find bugs.
- **Documentation**: help with code comments and with writing this README.

All code was written, tested and understood by the team; every AI suggestion was checked
against the RFCs and tested with a real client before being kept.
