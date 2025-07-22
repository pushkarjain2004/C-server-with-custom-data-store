# C-server-with-custom-data-store
Cache22 is a foundational, multi-client database server implemented in C. It demonstrates core concepts of network programming, concurrent client handling using process forking, and a custom in-memory data storage mechanism. It serves as a robust learning project for understanding low-level server architecture.


A) Prerequisites
1. A C compiler (e.g., gcc, clang)
2. make utility
3. telnet or netcat client for testing (install via Homebrew on macOS: brew install telnet or brew install netcat)

B) Building the Server
1. Clone this repository (or place cache22.c, tree.c, cache22.h, tree.h, and Makefile in the same directory).
2. Open your terminal in the project directory.
3. Run make to compile the server:
   ```bash
   make
   ```
5. this will create an executable named cache22_server.

C) Running the Server
 1. In your first terminal window:
```bash
     ./cache22_server 
```
You should see: Server listening on 127.0.0.1:12049
2. In a second terminal window:
```bash
 telnet 127.0.0.1 12049
```
You should see a welcome message and a prompt:
```bash
100 Connected to Cache22 server.
Type 'HELP' for commands, 'QUIT' to disconnect.
>
```
D) Usage Examples
      
Once connected, type the commands at the > prompt:

1. Store Data:
```bash
PUT /app/logs log_level=DEBUG
PUT /app/configs timeout=30
PUT /app/configs/auth method=OAuth2
PUT /data/users/profile user_id=1001
PUT /data/users/profile status=active
```
2. Retrieve Data:
```bash
GET /app/logs log_level
GET /data/users/profile user_id
```
3. List Contents:
```bash
LS /app/logs
LS /data/users/profile
LS /
```
4. Debug Tree Structure:
```bash
PRINT_TREE
```
5.Disconnect:
```bash
QUIT
```



THE END
  


