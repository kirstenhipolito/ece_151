# ece_151
ECE 151 Project

Added client.c and server.c

Client.c:
  Functional, can send a file to IP address specified
  Segment headers
  
Server.c:
  Functional, can receive and store to file
  Segment headers
  
Still need to implement:
  -Protocols
  
  To test:
  1. Have a savetothisfile.txt and sendthisfile.txt ready.
  2. Run server:
    ./server (port number) (filename of savetothisfile.txt) (proto)
  3. Run client:
    ./client (IP address e.g. 127.0.0.1 for same machine) (port number) (filename of sendthisfile.txt) (proto)
    
