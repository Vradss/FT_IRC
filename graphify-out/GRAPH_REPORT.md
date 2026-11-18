# Graph Report - IRC  (2026-11-18)

## Corpus Check
- 17 files · ~11,790 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 241 nodes · 378 edges · 15 communities (14 shown, 1 thin omitted)
- Extraction: 89% EXTRACTED · 11% INFERRED · 0% AMBIGUOUS · INFERRED: 42 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `5604f45c`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Clase Server (declaracion)
- Implementacion de Client
- Arranque y utilidades Server
- Clase Client (datos)
- Contrato sendTo/disconnect
- main y senales
- Cabeceras
- Lectura recv a lineas
- Escritura send
- Parte de red (Persona A)
- Server::handlePart
- Server::handleJoin
- Server::joinChannel
- README.md

## God Nodes (most connected - your core abstractions)
1. `Client` - 68 edges
2. `Server` - 37 edges
3. `Channel` - 32 edges
4. `Server::dispatcher()` - 13 edges
5. `Server::handleKick()` - 9 edges
6. `Server::handlePart()` - 8 edges
7. `Server::privateMessage()` - 8 edges
8. `Parte de red (Persona A)` - 8 edges
9. `Server::handleRead()` - 7 edges
10. `Server::handleNick()` - 7 edges

## Surprising Connections (you probably didn't know these)
- `Channel::addOperator()` --references--> `Client`  [EXTRACTED]
  srcs/Channel.cpp → includes/Client.hpp
- `Channel::addUser()` --references--> `Client`  [EXTRACTED]
  srcs/Channel.cpp → includes/Client.hpp
- `Channel::isInvited()` --references--> `Client`  [EXTRACTED]
  srcs/Channel.cpp → includes/Client.hpp
- `Channel::isMember()` --references--> `Client`  [EXTRACTED]
  srcs/Channel.cpp → includes/Client.hpp
- `Channel::isOperator()` --references--> `Client`  [EXTRACTED]
  srcs/Channel.cpp → includes/Client.hpp

## Import Cycles
- None detected.

## Communities (15 total, 1 thin omitted)

### Community 0 - "Clase Server (declaracion)"
Cohesion: 0.09
Nodes (30): map, string, Server, acceptClient, _channels, _clients, _fds, handleJoin (+22 more)

### Community 1 - "Implementacion de Client"
Cohesion: 0.10
Nodes (10): Client::addChannel(), Client::extractLine(), Client::getPrefix(), Client::queueOutput(), Client::removeChannel(), Client::setNick(), Client::setRealname(), Client::setUsername() (+2 more)

### Community 2 - "Arranque y utilidades Server"
Cohesion: 0.16
Nodes (14): queueOutput, dispatcher, fail, setupSocket, Client::queueMessage(), string, Server::acceptClient(), Server::fail() (+6 more)

### Community 3 - "Clase Client (datos)"
Cohesion: 0.06
Nodes (41): Client, addChannel, appendInput, _authenticated, _clientChannels, consumeOutput, extractLine, _fd (+33 more)

### Community 4 - "Contrato sendTo/disconnect"
Cohesion: 0.09
Nodes (17): Channel, Channel::addOperator(), Channel::addUser(), Channel::Channel(), Channel::isInvited(), Channel::isMember(), Channel::isOperator(), Channel::removeInvite() (+9 more)

### Community 5 - "main y senales"
Cohesion: 0.47
Nodes (4): string, main(), parsePort(), setupSignals()

### Community 6 - "Cabeceras"
Cohesion: 0.07
Nodes (32): Channel, addOperator, addUser, _chInvited, _chInviteOnly, _chKey, _chName, _chOperators (+24 more)

### Community 7 - "Lectura recv a lineas"
Cohesion: 0.19
Nodes (15): queueMessage, findByNick, findChannel, leaveChannel, string, vector, Server::handleKick(), splitByComma() (+7 more)

### Community 8 - "Escritura send"
Cohesion: 0.33
Nodes (11): completeRegistration, string, vector, isNickLetter(), isNickSpecial(), isValidNick(), isValidUsername(), Server::completeRegistration() (+3 more)

### Community 9 - "Parte de red (Persona A)"
Cohesion: 0.22
Nodes (8): 1. Quién es quién (clases), 2. El bucle principal (`Server::run`), 3. Leer: de bytes a líneas IRC (`handleRead`), 4. Escribir: nunca enviamos directamente (`sendTo` → `handleWrite`), 5. Vida de un cliente (estados de red), 6. Contrato con B y C, 7. Señales (`main.cpp`), Parte de red (Persona A)

### Community 10 - "Server::handlePart"
Cohesion: 0.46
Nodes (7): Channel, string, vector, Server::handlePart(), Server::leaveChannel(), splitByComma(), toLower()

### Community 11 - "Server::handleJoin"
Cohesion: 0.43
Nodes (6): vector, joinChannel, string, vector, Server::handleJoin(), splitByComma()

### Community 12 - "Server::joinChannel"
Cohesion: 0.80
Nodes (4): string, isValidChannelName(), Server::joinChannel(), toLowerStr()

## Knowledge Gaps
- **70 isolated node(s):** `_chName`, `_chTopic`, `_chUsers`, `_chOperators`, `_chInvited` (+65 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Client` connect `Clase Client (datos)` to `Clase Server (declaracion)`, `Implementacion de Client`, `Arranque y utilidades Server`, `Contrato sendTo/disconnect`, `Cabeceras`, `Lectura recv a lineas`, `Escritura send`, `Server::handlePart`, `Server::handleJoin`, `Server::joinChannel`?**
  _High betweenness centrality (0.674) - this node is a cross-community bridge._
- **Why does `Channel` connect `Cabeceras` to `Clase Client (datos)`?**
  _High betweenness centrality (0.222) - this node is a cross-community bridge._
- **Why does `Server` connect `Clase Server (declaracion)` to `Arranque y utilidades Server`, `Clase Client (datos)`, `Contrato sendTo/disconnect`, `Cabeceras`, `Lectura recv a lineas`, `Escritura send`, `Server::handleJoin`?**
  _High betweenness centrality (0.213) - this node is a cross-community bridge._
- **Are the 7 inferred relationships involving `Server::dispatcher()` (e.g. with `handleJoin` and `handleKick`) actually correct?**
  _`Server::dispatcher()` has 7 INFERRED edges - model-reasoned connections that need verification._
- **What connects `_chName`, `_chTopic`, `_chUsers` to the rest of the system?**
  _70 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Clase Server (declaracion)` be split into smaller, more focused modules?**
  _Cohesion score 0.09462365591397849 - nodes in this community are weakly interconnected._
- **Should `Implementacion de Client` be split into smaller, more focused modules?**
  _Cohesion score 0.09846153846153846 - nodes in this community are weakly interconnected._