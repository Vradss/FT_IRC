# Parte de red (Persona A)

Archivos: `srcs/main.cpp`, `srcs/Server.cpp`, `srcs/Client.cpp`, `includes/Server.hpp`, `includes/Client.hpp`.

Idea en una frase: **un único `poll()` vigila todos los sockets; cuando uno está listo, leemos o
escribimos sin bloquear, y nunca cerramos a un cliente en mitad de la vuelta.**

---

## 1. Quién es quién (clases)

```mermaid
classDiagram
    %% Server tiene muchos Client, guardados por su fd
    class Server {
        -int _listenFd
        -vector~pollfd~ _fds
        -map~int,Client~ _clients
        +run()
        +sendTo(client, line)
        +disconnect(client, reason)
        +findByNick(nick)
    }
    class Client {
        -int _fd
        -string _inBuffer
        -string _outBuffer
        -bool _toClose
        +appendInput()
        +extractLine()
        +queueOutput()
        +consumeOutput()
        +markToClose()
    }
    Server "1" o-- "*" Client : _clients[fd]
```

- **Server**: el que escucha, acepta y reparte. Es el único que llama a `poll`, `recv` y `send`.
- **Client**: un usuario conectado. Guarda **dos buffers**:
  - `_inBuffer`: lo que nos llegó y aún no forma una línea completa.
  - `_outBuffer`: lo que queremos enviarle y aún no ha salido.
- `_fds` es el array que le pasamos a `poll()`. La posición `[0]` es siempre el socket de escucha.

---

## 2. El bucle principal (`Server::run`)

```mermaid
flowchart TD
    A[updatePollEvents<br/>¿quién tiene algo que enviar?] --> B["poll() — el ÚNICO del programa<br/>espera hasta que algún fd esté listo"]
    B -->|Ctrl+C o error| Z[salir del bucle]
    B --> C{recorrer _fds}
    C -->|fd de escucha| D[acceptClient<br/>nuevo cliente]
    C -->|POLLERR / POLLNVAL| E[markToClose]
    C -->|POLLIN| F[handleRead<br/>recv]
    C -->|POLLOUT| G[handleWrite<br/>send]
    D --> H
    E --> H
    F --> H
    G --> H
    H[removeClosedClients<br/>cerrar los marcados] --> A
```

Explicación paso a paso:
1. **updatePollEvents**: a todos les pedimos `POLLIN` (avísame si me escriben). `POLLOUT` solo a
   quien tiene algo en `_outBuffer`. Si se lo pidiéramos a todos, `poll` volvería al instante → 100 % CPU.
2. **poll**: el programa se duerme aquí hasta que pase algo. Por eso no gasta CPU.
3. **Recorremos** los fds que tienen `revents` distinto de 0 y hacemos lo que toque.
4. **removeClosedClients**: al final, y solo al final, cerramos a los marcados. Así nadie borra
   un cliente mientras otro trozo de código lo está usando.

---

## 3. Leer: de bytes a líneas IRC (`handleRead`)

```mermaid
sequenceDiagram
    participant HC as HexChat / nc
    participant S as Server
    participant C as Client
    HC->>S: "NICK pe" (llega a trozos)
    S->>S: recv(fd, buf, 512)
    alt recv devuelve 0 o menos
        S->>C: disconnect → markToClose
    else llegan n bytes
        S->>C: appendInput(buf, n)
        HC->>S: "pe\r\n"
        S->>C: appendInput
        loop mientras haya una línea completa
            C-->>S: extractLine → "NICK pepe"
            S->>S: handleLine (aquí entrará el código de B)
        end
    end
```

- TCP **no respeta mensajes**: "NICK pepe\r\n" puede llegar en varios trozos, o dos comandos
  juntos en un solo `recv`. Por eso acumulamos en `_inBuffer` y solo sacamos líneas terminadas en `\n`.
- Aceptamos `\r\n` (clientes IRC) y `\n` solo (`nc` sin `-C`).
- Si `_inBuffer` pasa de 1024 bytes sin un `\n`, desconectamos: protege contra quien manda basura sin fin.
- **No miramos `errno`** (lo prohíbe el subject): solo el valor que devuelve `recv`.

---

## 4. Escribir: nunca enviamos directamente (`sendTo` → `handleWrite`)

```mermaid
sequenceDiagram
    participant B as Código de B / C
    participant S as Server
    participant C as Client
    participant HC as HexChat
    B->>S: sendTo(client, ":ircserv 001 pepe :Welcome")
    S->>C: queueOutput(line + "\r\n")
    Note over S: siguiente vuelta: poll marca POLLOUT
    S->>HC: send(fd, _outBuffer)
    S->>C: consumeOutput(n) — borra solo lo enviado
    Note over C: si send fue parcial, el resto espera a la siguiente vuelta
```

- `sendTo` **no envía**: solo deja el mensaje en la cola (`_outBuffer`).
- El `send` real ocurre cuando `poll` dice que ese socket puede escribir (regla del subject:
  nunca `send` sin que `poll` lo marque listo).
- `send` puede enviar solo una parte. Borramos lo enviado y el resto sale en la siguiente vuelta.

---

## 5. Vida de un cliente (estados de red)

```mermaid
stateDiagram-v2
    [*] --> Conectado: accept() + O_NONBLOCK
    Conectado --> Conectado: recv / send normales
    Conectado --> Marcado: disconnect()<br/>recv = 0, error, POLLERR,<br/>línea demasiado larga
    Marcado --> [*]: removeClosedClients()<br/>close(fd) y se borra
```

- **Marcado** = `_toClose = true`. El cliente sigue en memoria hasta el final de la vuelta.
- El registro (PASS/NICK/USER) es otro estado que lleva **B**; aquí solo está la parte de red.

---

## 6. Contrato con B y C

B y C **nunca** llaman a `recv`, `send` ni `close`. Solo usan:

| Función | Qué hace |
|---|---|
| `sendTo(client, line)` | Encola `line + "\r\n"`. No pongáis `\r\n` vosotros. |
| `disconnect(client, reason)` | Marca para cerrar al final de la vuelta. |
| `findByNick(nick)` | Busca sin distinguir mayúsculas. `NULL` si no existe. |
| `getPassword()` | La contraseña de `./ircserv <port> <password>`. |

Pendiente de integrar:
- **B**: `handleLine` pasará a llamar a `CommandHandler::execute(...)`.
- **C**: en `removeClosedClients`, sacar al cliente de sus canales antes de borrarlo.

---

## 7. Señales (`main.cpp`)

- **SIGPIPE ignorado**: si enviamos a un cliente que ya cerró, el sistema mandaría SIGPIPE y
  mataría el servidor. Ignorándolo, `send` solo devuelve -1 y desconectamos a ese cliente.
- **SIGINT (Ctrl+C)**: pone `g_running = false`; `poll` se interrumpe y salimos limpio
  (el destructor cierra todos los fds).
