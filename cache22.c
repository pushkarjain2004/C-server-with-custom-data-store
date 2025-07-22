#include "cache22.h" // Your custom server definitions (Client, Callback, CmdHandler)
#include "tree.h"    // Your tree implementation definitions (Node, Leaf, root, find_node_linear, create_leaf, lookup_linear, print_tree_forward_leaves etc.)

// Global flags for server and child process continuation
bool scontinuation; // Controls the main server loop in 'main'
bool ccontinuation; // Controls the client handling loop in 'childloop'

// --- Function Prototypes for Command Handlers ---
// These functions will be called when their respective commands are received.
// Each handler takes the client context, and two parsed string arguments (folder/path, args/key/value)
int32 handle_hello(Client *cli, int8 *arg1, int8 *arg2);
int32 handle_get(Client *cli, int8 *path, int8 *key);
int32 handle_put(Client *cli, int8 *path, int8 *key_val_pair); // put path key=value
int32 handle_cd(Client *cli, int8 *path, int8 *args); // cd /some/path
int32 handle_ls(Client *cli, int8 *path, int8 *args); // ls /some/path (list nodes/leaves)
int32 handle_quit(Client *cli, int8 *arg1, int8 *arg2); // quit command to disconnect client
int32 handle_print_tree(Client *cli, int8 *arg1, int8 *arg2); // Debug: print full tree to client

// --- Command Handler Array ---
// This array maps command strings (e.g., "GET") to their corresponding handler functions.
// To add a new command, implement its handler function and add an entry here.
CmdHandler handlers[] = {
    {(int8 *)"hello", handle_hello},
    {(int8 *)"GET", handle_get},
    {(int8 *)"PUT", handle_put},
    {(int8 *)"CD", handle_cd},
    {(int8 *)"LS", handle_ls},
    {(int8 *)"QUIT", handle_quit},
    {(int8 *)"PRINT_TREE", handle_print_tree} // Debug command to print the entire tree
    // Add more commands here (e.g., "DELETE", "UPDATE")
};

// --- Helper Functions ---

// Function to find a command handler by its name
Callback getcmd(int8 *cmd) {
    Callback cb = NULL; // Initialize function pointer to NULL (0)
    int16 n;
    // Calculate the number of elements in the handlers array dynamically.
    // This is safer than hardcoding the size (e.g., '16').
    int16 arrlen = sizeof(handlers) / sizeof(handlers[0]);

    // Loop through the 'handlers' array
    for (n = 0; n < arrlen; n++) {
        // Compare the input command string with the command string in the current handler entry.
        // 'strcmp' returns 0 if the strings are identical.
        if (!strcmp((char *)cmd, (char *)handlers[n].cmd)) {
            cb = handlers[n].handler; // Found a match, store the handler's function pointer.
            break;                    // Exit the loop as the command has been found.
        }
    }
    return cb; // Return the found handler function pointer (or NULL if no match was found).
}

// Custom error handling function: prints system error message and exits program.
void assert_perror(int system_call_return_value) {
    if (system_call_return_value == -1) {
        perror("System call error"); // Prints "System call error: [description of errno]"
        exit(EXIT_FAILURE);         // Terminates the entire program with an error status.
    }
}

// Utility function to set a block of memory to all zeros.



// --- Command Handler Implementations ---
// These functions implement the logic for each specific command.

// Handler for the "hello" command.
// Format: hello <any_folder_arg> <any_args_arg>
int32 handle_hello(Client *cli, int8 *folder, int8 *args) {
    // dprintf writes formatted output directly to the client's socket.
    // It's a GNU extension, available because of '#define _GNU_SOURCE'.
    dprintf(cli->s, "Server: Hello '%s'!\n", (char*)folder);
    return 0; // Return 0 to indicate success.
}

// Handler for the "GET" command.
// Format: GET <path> <key>
int32 handle_get(Client *cli, int8 *path, int8 *key) {
    // Basic validation of input arguments.
    if (!path || !key || strlen((char*)path) == 0 || strlen((char*)key) == 0) {
        dprintf(cli->s, "ERROR: GET command requires a path and a key. Usage: GET <path> <key>\n");
        return -1; // Return -1 to indicate an error to the calling function.
    }

    // Call the tree's lookup function to find the value associated with the path and key.
    int8 *value = lookup_linear((int8*)path, (int8*)key);
    if (value) {
        // If a value is found, send it back to the client.
        dprintf(cli->s, "VALUE: ");
        // 'write' is used for raw byte output, suitable for data that might not be null-terminated
        // or contain embedded nulls, though here it's a string.
        write(cli->s, (char*)value, (int)strlen((char*)value)); // write raw value bytes.
        dprintf(cli->s, "\n");
    } else {
        // If the key is not found, inform the client.
        dprintf(cli->s, "ERROR: Key '%s' not found in path '%s'.\n", (char*)key, (char*)path);
    }
    return 0; // Return 0 to indicate the command was processed (even if key not found).
}

// Handler for the "PUT" command.
// Format: PUT <path> <key>=<value>
// In cache22.c

int32 handle_put(Client *cli, int8 *full_path, int8 *key_value_pair) {
    // --- Initial Argument Validation ---
    if (!full_path || strlen((char*)full_path) == 0 || !key_value_pair || strlen((char*)key_value_pair) == 0) {
        dprintf(cli->s, "ERROR: PUT command requires a path and a key=value pair. Usage: PUT <path> <key>=<value>\n");
        return -1;
    }

    // --- Parse Key and Value ---
    char *equal_sign = strchr((char*)key_value_pair, '=');
    if (!equal_sign) {
        dprintf(cli->s, "ERROR: PUT value must be in key=value format.\n");
        return -1;
    }
    *equal_sign = '\0'; // Null-terminate the key part, effectively splitting the string.
    int8 *key = key_value_pair; // 'key' now points to the beginning of the key_value_pair string.
    int8 *value = (int8*)(equal_sign + 1); // 'value' points to the character after '='.
    int16 value_len = (int16)strlen((char*)value);

    // Validate parsed key and value content.
    if (strlen((char*)key) == 0 || value_len == 0) {
        dprintf(cli->s, "ERROR: Key or Value cannot be empty in PUT command.\n");
        return -1;
    }

    // --- Traverse/Create Nodes for the Path ---
    // This section ensures all nodes in the 'full_path' exist or are created.
    Node *current_parent_node = (Node*)&root; // Start traversal from the global root node.
    char temp_path_segment[256];             // Temporary buffer to hold each segment name (e.g., "users", "login").
    char current_full_path_so_far[256];      // Buffer to build the full path string for find_node_linear.
    Node *found_node_for_segment;            // Pointer to store a node found by find_node_linear.
    char *path_walk_ptr;                     // Pointer to walk through the input 'full_path' string.

    // Initialize current_full_path_so_far to just the root path "/".
    current_full_path_so_far[0] = '/';
    current_full_path_so_far[1] = '\0';

    // Set 'path_walk_ptr' to point after the leading slash, if present.
    path_walk_ptr = (*full_path == '/') ? (char*)full_path + 1 : (char*)full_path;

    // Loop through each segment of the 'full_path' (e.g., "users", then "login" for "/users/login").
    while (path_walk_ptr && *path_walk_ptr != '\0') {
        char *next_slash = strchr(path_walk_ptr, '/'); // Find the next '/' to delimit the current segment.
        size_t segment_len;

        // Determine the length of the current path segment.
        if (next_slash) {
            segment_len = next_slash - path_walk_ptr; // Length up to the next slash.
        } else {
            segment_len = strlen(path_walk_ptr);      // Length to the end of the string.
        }

        // Copy the current segment name into 'temp_path_segment'.
        strncpy(temp_path_segment, path_walk_ptr, segment_len);
        temp_path_segment[segment_len] = '\0'; // Null-terminate the segment name.

        // Append the current segment to 'current_full_path_so_far'.
        // This builds the full path string (e.g., "/users", then "/users/login").
        if (strlen(current_full_path_so_far) > 1 || current_full_path_so_far[0] != '/') { // If not just "/" at start
            strncat(current_full_path_so_far, "/", sizeof(current_full_path_so_far) - strlen(current_full_path_so_far) - 1);
        }
        strncat(current_full_path_so_far, temp_path_segment, sizeof(current_full_path_so_far) - strlen(current_full_path_so_far) - 1);
        current_full_path_so_far[sizeof(current_full_path_so_far) - 1] = '\0'; // Ensure it's null-terminated.


        // Search for the node corresponding to 'current_full_path_so_far'.
        // find_node_linear traverses the global linear 'west' chain from root.
        found_node_for_segment = find_node_linear((int8*)current_full_path_so_far);

        if (!found_node_for_segment) {
            // If the node for this segment does not exist in the tree's linear path, create it.
            // 'create_node' links the new node as the 'west' child of 'current_parent_node'.
            // WARNING: With the current tree structure (Node->west being the only child pointer),
            // this will overwrite existing 'west' links if 'current_parent_node' already has one.
            // This means it only effectively extends a single linear branch from the root.
            Node *new_node = create_node(current_parent_node, (int8*)current_full_path_so_far);
            if (!new_node) {
                dprintf(cli->s, "ERROR: Failed to allocate memory for path node '%s'.\n", (char*)current_full_path_so_far);
                return -1; // Critical failure, cannot create path.
            }
            current_parent_node = new_node; // Update 'current_parent_node' to point to the newly created node.
        } else {
            // If the node already exists, simply move 'current_parent_node' to the found node.
            current_parent_node = found_node_for_segment;
        }

        // Move 'path_walk_ptr' to the start of the next segment.
        if (next_slash) {
            path_walk_ptr = next_slash + 1;
        } else {
            path_walk_ptr = NULL; // No more segments to process.
        }
    }

    // After the loop, 'current_parent_node' points to the final Node where the leaf should be stored.
    // If 'current_parent_node' is somehow NULL here, it indicates an internal logic error.
    if (!current_parent_node) {
        dprintf(cli->s, "INTERNAL ERROR: Target path node is NULL after creation/lookup for '%s'.\n", (char*)full_path);
        return -1;
    }

    // --- Step 3: Store/Update Leaf under the found/created Node ---
    // Now, 'current_parent_node' is the actual Node where the leaf should reside.
    // We pass 'current_parent_node->path' to find_leaf_linear to ensure
    // we search for the leaf directly under this specific node.
    Leaf *existing_leaf = find_leaf_linear((int8*)current_parent_node->path, (int8*)key);
    if (existing_leaf) {
        // If the key already exists, update its value.
        free(existing_leaf->value); // Free the old dynamically allocated value.
        existing_leaf->value = (int8*)malloc(value_len + 1); // Allocate new memory (+1 for null terminator).
        if (!existing_leaf->value) { perror("malloc failed for leaf value update"); return -1; }
        zero(existing_leaf->value, value_len + 1); // Zero out the new memory block.
        strncpy((char*)existing_leaf->value, (char*)value, value_len); // Copy the new value string.
        existing_leaf->value[value_len] = '\0'; // Ensure null-termination for the copied value.
        existing_leaf->size = value_len; // Update the size.
        dprintf(cli->s, "OK: Key '%s' updated in path '%s'.\n", (char*)key, (char*)current_parent_node->path);
    } else {
        // If the key does not exist, create a new leaf.
        // 'create_leaf' will handle allocating memory for the key and value.
        create_leaf(current_parent_node, key, value, value_len);
        dprintf(cli->s, "OK: Key '%s' created in path '%s'.\n", (char*)key, (char*)current_parent_node->path);
    }
    return 0;
}

// Handler for the "CD" command (Change Directory/Node context).
// Format: CD <path>
int32 handle_cd(Client *cli, int8 *path, int8 *args) {
    // Check if a path argument is provided.
    if (!path || strlen((char*)path) == 0) {
        dprintf(cli->s, "ERROR: CD command requires a path. Usage: CD <path>\n");
        return -1;
    }
    // Find the Node specified by the path.
    Node *target_node = find_node_linear((int8*)path);
    if (target_node) {
        // In a more complex server, the 'Client' struct would have a 'current_node' field
        // to keep track of each client's "current directory" in the tree.
        // For example: 'cli->current_node = target_node;'
        dprintf(cli->s, "OK: Changed context to node '%s' (not persistent per client yet).\n", (char*)target_node->path);
    } else {
        dprintf(cli->s, "ERROR: Path '%s' not found.\n", (char*)path);
    }
    return 0;
}

// Handler for the "LS" command (List contents of a Node/folder).
// Format: LS [<path>] (lists children nodes and leaves under that path, default to root)
int32 handle_ls(Client *cli, int8 *path, int8 *args) {
    Node *target_node;
    // Determine the target node: default to root if no path given, otherwise find the specified path.
    if (!path || strlen((char*)path) == 0) {
        target_node = (Node*)&root; // Assuming 'root' is globally accessible from tree.h/tree.c.
    } else {
        target_node = find_node_linear((int8*)path);
    }

    if (!target_node) {
        dprintf(cli->s, "ERROR: Path '%s' not found.\n", (char*)path);
        return -1;
    }

    dprintf(cli->s, "Listing contents of '%s':\n", (char*)target_node->path);

    // List child Nodes (if your tree supported horizontal node children)
    // Currently, your Node's 'west' is a linear chain, not typically for listing children of 'n'.
    // A typical 'ls' would list n->west's children if Node supported a 'children' list.
    // For now, we only list leaves under the target_node.

    // List Leaves under this node
    Leaf *l = target_node->east; // Start from the first leaf connected via 'east'.
    if (!l) {
        dprintf(cli->s, " (No leaves found)\n");
    } else {
        while(l != NULL) { // Iterate through all leaves in the 'east' chain.
            dprintf(cli->s, "  L: %s -> '", (char*)l->key);
            write(cli->s, (char*)l->value, (int)l->size); // Write raw value.
            dprintf(cli->s, "'\n");
            l = l->east; // Move to the next leaf.
        }
    }
    return 0;
}

// Handler for the "QUIT" command.
// Format: QUIT
int32 handle_quit(Client *cli, int8 *folder, int8 *args) {
    dprintf(cli->s, "Server: Goodbye!\n");
    ccontinuation = false; // Set this flag to 'false' to break the 'childloop'
                           // which will lead to the child process exiting.
    return 0;
}

// Handler for the "PRINT_TREE" debug command.
// Format: PRINT_TREE
int32 handle_print_tree(Client *cli, int8 *folder, int8 *args) {
    dprintf(cli->s, "Server: Printing entire tree to your client (debug output)...\n");
    // Call the tree printing function, redirecting output to client's socket.
    // Assumes 'print_tree_forward_leaves' is the desired printer.
    print_tree_forward_leaves(cli->s, &root);
    dprintf(cli->s, "Server: Tree print complete.\n");
    return 0;
}


// --- Main Client Handling Loop for a Child Process ---
// This function runs in each forked child process to handle a single client's commands.
void childloop(Client *cli) {
    int8 buf[256];      // Buffer for raw client input (max 255 chars + null)
    int8 cmd[256];      // Parsed command string
    int8 folder[256];   // Parsed folder/path string (argument 1)
    int8 args[256];     // Parsed arguments string (argument 2)
    ssize_t bytes_read; // Number of bytes read from socket (can be 0 or -1)
    Callback handler_func; // Function pointer for the command handler

    // Loop continuously to handle multiple commands from the same client.
    ccontinuation = true; // Ensure this flag is true to start the loop.
    while (ccontinuation) {
        zero(buf, sizeof(buf)); // Clear the input buffer before each read for safety.

        // --- Read Data from Client Socket ---
        // Reads up to (sizeof(buf) - 1) bytes to leave space for a null terminator.
        bytes_read = read(cli->s, (char *)buf, sizeof(buf) - 1);

        if (bytes_read <= 0) { // Check if client disconnected (0 bytes) or an error occurred (-1).
            if (bytes_read == 0) {
                // Client gracefully disconnected (read returned 0 bytes).
                printf("Server: Client %s:%d disconnected gracefully.\n", cli->ip, cli->port);
            } else { // bytes_read < 0, an error during read.
                if (errno == EINTR) { // Read was interrupted by a signal, often safe to retry.
                    continue; // Skip the rest of this iteration and try reading again.
                }
                // For other read errors, print to server console and mark for termination.
                perror("Error reading from client socket");
                printf("Server: Error reading from client %s:%d. Terminating connection.\n", cli->ip, cli->port);
            }
            ccontinuation = false; // Set flag to 'false' to break out of this 'while' loop.
            break; // Exit the loop.
        }

        buf[bytes_read] = '\0'; // CRITICAL: Null-terminate the received data. This turns 'buf' into a valid C string.

        // --- Command Parsing using sscanf ---
        // sscanf parses formatted input from a string.
        // %255s: reads up to 255 non-whitespace characters into cmd, folder, args.
        // %255[^\n\r]s: reads up to 255 characters until a newline or carriage return is encountered (for the 'args' part).
        // It returns the number of items successfully assigned.
        int num_parsed = sscanf((char*)buf, "%255s %255s %255[^\n\r]s", (char*)cmd, (char*)folder, (char*)args);

        // Ensure all parsed buffers are null-terminated, even if sscanf didn't fill them.
        // This is necessary if sscanf didn't assign to a variable.
        if (num_parsed < 1) zero(cmd, sizeof(cmd));
        if (num_parsed < 2) zero(folder, sizeof(folder));
        if (num_parsed < 3) zero(args, sizeof(args));


        // Handle cases where parsing might not have extracted a command (e.g., empty line or just whitespace).
        if (strlen((char*)cmd) == 0) {
            dprintf(cli->s, "ERROR: Please enter a command.\n> "); // Prompt again.
            continue; // Skip to next iteration of while loop.
        }

        // --- Command Execution ---
        handler_func = getcmd(cmd); // Look up the command handler function using the parsed command string.

        if (handler_func) {
            // If a handler function is found (not NULL), execute it.
            // Pass the client context and the parsed arguments.
            handler_func(cli, folder, args);
        } else {
            // If no handler is found for the given command, inform the client.
            dprintf(cli->s, "ERROR: Unknown command '%s'. Type QUIT to exit.\n", (char*)cmd);
        }
        
        // Send a prompt to the client for their next command, after processing the current one.
        dprintf(cli->s, "> ");
    } // End of while(ccontinuation) loop
}

// --- Server Initialization Function ---
// This function sets up the main listening socket for the server.
int initserver(int16 port) {
    struct sockaddr_in sock; // Structure to hold the server's network address (IP and Port).
    int s;                   // File descriptor for the main listening socket.

    // Set up the server's address structure.
    sock.sin_family = AF_INET;                 // Use IPv4 addresses.
    sock.sin_port = htons((int)port);          // Set the port number (converted to network byte order).
    sock.sin_addr.s_addr = inet_addr(HOST);    // Set the IP address (converted from string to network byte order).

    // Create the listening socket.
    s = socket(AF_INET, SOCK_STREAM, 0); // Creates a TCP socket (IPv4, Stream type, default protocol).
    assert_perror(s);                    // Check for errors during socket creation.

    // Bind the socket to the specified address and port.
    // This assigns the socket to a specific local network interface and port.
    int bind_result = bind(s, (struct sockaddr *)&sock, sizeof(sock));
    assert_perror(bind_result); // Check for errors during binding.

    // Put the socket into listening mode.
    // '20' is the backlog queue size, defining how many pending client connections
    // can wait to be accepted before the server starts rejecting new connections.
    int listen_result = listen(s, 20);
    assert_perror(listen_result); // Check for errors during listening setup.

    printf("Server listening on %s:%d\n", HOST, port); // Inform the server operator.
    return s; // Return the file descriptor of the listening socket.
}


// --- Main Server Loop for Accepting Connections ---
// This function runs in the parent process, continuously accepting new client connections.
void mainloop(int s) {
    struct sockaddr_in cli;       // Structure to hold the connecting client's address.
    socklen_t len = sizeof(cli);  // Correctly initialized length for accept().
    int s2;                       // File descriptor for the NEW socket specific to the accepted client.
    char *ip;                     // Pointer to the client's IP address string.
    int16 port;                   // Client's port number.
    Client *client;               // Dynamically allocated structure to store client-specific data.
    pid_t pid;                    // Variable to store the process ID returned by fork().

    // Accept an incoming connection.
    // This call blocks the parent process until a client attempts to connect to the listening socket 's'.
    s2 = accept(s, (struct sockaddr *)&cli, &len);
    if (s2 < 0) {
        // Handle accept errors. EINTR means the call was interrupted by a signal (e.g., Ctrl+C).
        // For EINTR, we can often just retry accepting. For other errors, we might consider exiting.
        if (errno == EINTR) {
            return; // Return to the main loop to try accept again.
        }
        assert_perror(s2); // For other critical errors, this will print and exit.
    }

    // Extract and print client details for the server's console.
    port = (int16)ntohs(cli.sin_port); // Convert port from network to host byte order.
    ip = inet_ntoa(cli.sin_addr);       // Convert IP address to human-readable string.
    printf("Server: Connection from %s:%d (socket %d)\n", ip, port, s2);

    // Allocate and populate the Client struct.
    client = (Client *)malloc(sizeof(struct s_client));
    if (!client) { // Robust check for malloc failure.
        perror("malloc failed for client struct");
        close(s2); // Close the client socket to avoid a leak if malloc fails.
        return;    // Return to main loop.
    }
    zero((int8 *)client, sizeof(struct s_client)); // Initialize allocated memory to zeros.
    client->s = s2;                                // Store the client-specific socket file descriptor.
    client->port = port;
    // Copy client IP, ensuring buffer safety by limiting length and explicitly null-terminating.
    strncpy(client->ip, ip, sizeof(client->ip) - 1);
    client->ip[sizeof(client->ip) - 1] = '\0'; // Guarantee null-termination.

    // Fork a new process to handle the client.
    pid = fork();

    if (pid > 0) { // This block is executed by the PARENT PROCESS.
        // Parent's responsibilities after forking:
        // 1. Close its copy of the client-specific socket ('s2').
        //    The child process will handle communication with this client.
        //    The parent doesn't need 's2' and keeping it open would cause resource leaks.
        close(s2);
        // 2. Free the 'Client' struct memory.
        //    The child process has its own duplicate memory space for this struct.
        free(client);
        // 3. Return to the 'main' loop to 'accept()' the next client connection.
        return;
    } else if (pid == 0) { // This block is executed by the CHILD PROCESS.
        // Child's responsibilities:
        // 1. Close its copy of the main listening socket ('s').
        //    The child's purpose is to communicate with its specific client ('s2'),
        //    not to accept new connections. Closing 's' prevents resource leaks in the child.
        close(s);

        // Send an initial welcome message and prompt to the client.
        dprintf(client->s, "100 Connected to Cache22 server.\n");
        dprintf(client->s, "Type 'HELP' for commands, 'QUIT' to disconnect.\n> ");

        // Enter the client handling loop.
        childloop(client); // This function now contains the while(ccontinuation) loop for persistent interaction.

        // Child process cleanup after 'childloop' exits (e.g., client sends 'QUIT' or disconnects).
        close(client->s); // Close the client-specific socket.
        free(client);     // Free the dynamically allocated 'Client' struct memory.
        printf("Server: Child process for %s:%d exited.\n", ip, port); // Log child exit.
        exit(0); // CRITICAL: The child process MUST call 'exit(0)' here.
                 // This terminates the child process. If omitted, the child would return
                 // to the parent's context and likely try to accept new connections,
                 // leading to chaotic and incorrect server behavior.
    } else { // pid == -1 (fork failed).
        // Handle error if 'fork()' itself failed (e.g., system out of process slots).
        // 'assert_perror' will print the error and exit the parent process.
        assert_perror(pid);
    }
    // This 'return;' is unreachable as all execution paths (parent, child, fork failure)
    // contain either a 'return' or an 'exit()'.
}

// --- Main Program Entry Point ---
// This is where the server program begins execution.
int main(int argc, char *argv[]) {
    char *sport;
    int16 port;
    int s; // File descriptor for the main listening socket.

    // 1. Determine the port number for the server:
    // If no command-line argument is provided (argc < 2), use the default PORT defined in cache22.h.
    // Otherwise, use the first command-line argument (argv[1]) as the port string.
    if (argc < 2) {
        sport = PORT;
    } else {
        sport = argv[1];
    }
    port = (int16)atoi(sport); // Convert the port string (e.g., "12049") to an integer.

    // --- REMOVED TEST BLOCK ---
    // The previous test block for getcmd is removed.
    // This ensures the server initialization and main loop execute.

    // 2. Initialize the server's listening socket:
    s = initserver(port); // Call 'initserver' to set up the server socket.

    // 3. Enter the main server loop:
    // This loop continuously calls 'mainloop' to accept new client connections.
    scontinuation = true; // Set the flag to 'true' to start the loop.
    while (scontinuation) {
        mainloop(s); // Call 'mainloop', which will block until a client connects.
    }

    // 4. Server Shutdown:
    // These lines are only executed if 'scontinuation' becomes 'false' (e.g., if a signal handler
    // for Ctrl+C were implemented to set it to 'false').
    printf("Server: Shutting down...\n");
    close(s); // Close the main listening socket, releasing its resources.
    return 0; // Program exits successfully.
}