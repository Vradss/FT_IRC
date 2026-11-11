apuntes útiles (to fact check)
Each client is distinguished from other clients by a unique
   nickname having a maximum length of nine (9) characters

  In addition to the nickname, all servers must have the
   following information about all clients: the real name of the host
   that the client is running on, the username of the client on that
   host, and the server to which the client is connected. ?


 Operators should be able to perform basic
   network tasks such as disconnecting and reconnecting servers as
   needed to prevent long-term use of bad network routing.  In
   recognition of this need, the protocol discussed herein provides for
   operators only to be able to perform such functions.  See sections
   4.1.7 (SQUIT) and 4.3.5 (CONNECT).

   A more controversial power of operators is the ability  to  remove  a
   user  from  the connected network by 'force', i.e. operators are able
   to close the connection between any client and server.   The
   justification for  this  is delicate since its abuse is both
   destructive and annoying.  For further details on this type of
   action, see section 4.6.1 (KILL).?

   The channel is created
   implicitly when the first client joins it, and the channel ceases to
   exist when the last client leaves it.  While channel exists, any
   client can reference the channel using the name of the channel.

   Channels names are strings (beginning with a '&' or '#' character) of
   length up to 200 characters.  Apart from the the requirement that the
   first character being either '&' or '#'; the only restriction on a
   channel name is that it may not contain any spaces (' '), a control G
   (^G or ASCII 7), or a comma (',' which is used as a list item
   separator by the protocol).
