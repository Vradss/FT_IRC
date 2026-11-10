# Graph Report - .  (2026-11-09)

## Corpus Check
- Corpus is ~1,521 words - fits in a single context window. You may not need a graph.

## Summary
- 75 nodes · 116 edges · 9 communities
- Extraction: 81% EXTRACTED · 19% INFERRED · 0% AMBIGUOUS · INFERRED: 22 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

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

## God Nodes (most connected - your core abstractions)
1. `Client` - 25 edges
2. `Server` - 22 edges
3. `Server::handleRead()` - 7 edges
4. `Server::run()` - 6 edges
5. `Server::disconnect()` - 6 edges
6. `Server::handleLine()` - 5 edges
7. `Server::handleWrite()` - 4 edges
8. `Server::sendTo()` - 4 edges
9. `Server::findByNick()` - 4 edges
10. `getFd` - 3 edges

## Surprising Connections (you probably didn't know these)
- `Server::handleWrite()` --references--> `Client`  [INFERRED]
  srcs/Server.cpp → includes/Client.hpp
- `Server::handleRead()` --calls--> `appendInput`  [INFERRED]
  srcs/Server.cpp → includes/Client.hpp
- `Server::handleRead()` --calls--> `extractLine`  [INFERRED]
  srcs/Server.cpp → includes/Client.hpp
- `Server::handleRead()` --calls--> `inputSize`  [INFERRED]
  srcs/Server.cpp → includes/Client.hpp
- `Server::sendTo()` --calls--> `queueOutput`  [INFERRED]
  srcs/Server.cpp → includes/Client.hpp

## Import Cycles
- None detected.

## Communities (9 total, 0 thin omitted)

### Community 0 - "Clase Server (declaracion)"
Cohesion: 0.16
Nodes (16): string, Server, acceptClient, _clients, _fds, findByNick, handleRead, handleWrite (+8 more)

### Community 1 - "Implementacion de Client"
Cohesion: 0.18
Nodes (4): Client::extractLine(), Client::queueOutput(), Client::setNick(), string

### Community 2 - "Arranque y utilidades Server"
Cohesion: 0.27
Nodes (8): fail, string, Server::fail(), Server::findByNick(), Server::sendTo(), Server::Server(), Server::setupSocket(), toLower()

### Community 3 - "Clase Client (datos)"
Cohesion: 0.20
Nodes (10): Client, _fd, hasOutput, _inBuffer, _nick, _outBuffer, queueOutput, setNick (+2 more)

### Community 4 - "Contrato sendTo/disconnect"
Cohesion: 0.33
Nodes (6): getFd, markToClose, shouldClose, sendTo, Server::disconnect(), Server::handleLine()

### Community 5 - "main y senales"
Cohesion: 0.47
Nodes (4): string, main(), parsePort(), setupSignals()

### Community 6 - "Cabeceras"
Cohesion: 0.50
Nodes (3): string, map, vector

### Community 7 - "Lectura recv a lineas"
Cohesion: 0.40
Nodes (5): appendInput, extractLine, inputSize, handleLine, Server::handleRead()

### Community 8 - "Escritura send"
Cohesion: 0.67
Nodes (3): consumeOutput, disconnect, Server::handleWrite()

## Knowledge Gaps
- **14 isolated node(s):** `setNick`, `hasOutput`, `_fd`, `_nick`, `_inBuffer` (+9 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Client` connect `Clase Client (datos)` to `Clase Server (declaracion)`, `Implementacion de Client`, `Arranque y utilidades Server`, `Contrato sendTo/disconnect`, `Cabeceras`, `Lectura recv a lineas`, `Escritura send`?**
  _High betweenness centrality (0.679) - this node is a cross-community bridge._
- **Why does `Server` connect `Clase Server (declaracion)` to `Arranque y utilidades Server`, `Clase Client (datos)`, `Contrato sendTo/disconnect`, `Cabeceras`, `Lectura recv a lineas`, `Escritura send`?**
  _High betweenness centrality (0.421) - this node is a cross-community bridge._
- **Why does `Server::findByNick()` connect `Arranque y utilidades Server` to `Clase Client (datos)`?**
  _High betweenness centrality (0.034) - this node is a cross-community bridge._
- **Are the 6 inferred relationships involving `Server::handleRead()` (e.g. with `appendInput` and `extractLine`) actually correct?**
  _`Server::handleRead()` has 6 INFERRED edges - model-reasoned connections that need verification._
- **Are the 5 inferred relationships involving `Server::run()` (e.g. with `acceptClient` and `handleRead`) actually correct?**
  _`Server::run()` has 5 INFERRED edges - model-reasoned connections that need verification._
- **What connects `setNick`, `hasOutput`, `_fd` to the rest of the system?**
  _14 weakly-connected nodes found - possible documentation gaps or missing edges._