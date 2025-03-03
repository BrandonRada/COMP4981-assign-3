////
//// Created by brandon-rada on 3/2/25.
////
//

#include "server.h"
#include <arpa/inet.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define FIVE 5
// #define COMMAND_FILE "server_commands"

// Global variable for the server socket
static int server_socket;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
// Flag to indicate server shutdown
static volatile sig_atomic_t shutdown_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// Function to gracefully shutdown the server
__attribute__((noreturn)) void graceful_shutdown(void)
{
    const char *shutdown_message = "Server shutdown complete\n";
    // Ensure that any child processes have finished
    while(waitpid(-1, NULL, WNOHANG) > 0)
    {
        // Wait until all have finished.
    }
    // Wait for any child processes to finish

    write(STDERR_FILENO, shutdown_message, strlen(shutdown_message));
    _exit(0);
}

// Signal handler for SIGINT (Ctrl+C)
__attribute__((noreturn)) static void handle_sigint(int signal)
{
    const char *shutdown_message = "Shutdown initiated...\n";
    (void)signal;
    shutdown_flag = 1;       // Set flag to initiate graceful shutdown
    close(server_socket);    // Close the server socket immediately to stop accepting new connections
    write(STDERR_FILENO, shutdown_message, strlen(shutdown_message));
    graceful_shutdown();
}

int main(void)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t          client_len = sizeof(client_addr);

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    // Bind socket to port
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if(listen(server_socket, FIVE) < 0)
    {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    // Register SIGINT handler
    signal(SIGINT, handle_sigint);

    while(!shutdown_flag)
    {
        int   client_socket;
        pid_t pid;

        // Accept client connection (will stop if shutdown_flag is set)
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if(client_socket < 0)
        {
            if(shutdown_flag)
            {    // If shutdown is initiated, break out of the loop
                break;
            }
            perror("Accept failed");
            continue;
        }

        printf("New client connected!\n");

        // Fork a new process to handle the client
        pid = fork();
        if(pid == 0)
        {
            // Child process: handle the client
            close(server_socket);    // Child process does not need the server socket
            handle_client(client_socket);
            close(client_socket);
            exit(0);
        }
        else if(pid > 0)
        {
            // Parent process: close the client socket and continue
            close(client_socket);
        }
        else
        {
            perror("Fork failed");
        }
    }

    // Perform graceful shutdown actions after breaking out of the loop
    graceful_shutdown();
}

// Handles communication with a single client
void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE];

    // Send a message to notify the client when the server is shutting down
    if(shutdown_flag)
    {
        const char *shutdown_message = "The server is shutting down. Please disconnect.\n";
        write(client_socket, shutdown_message, strlen(shutdown_message));
    }

    while(1)
    {
        ssize_t bytes_read;
        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);

        if(bytes_read <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;    // Remove newline character

        // Exit if the client sends "exit"
        if(strcmp(buffer, "exit") == 0)
        {
            printf("Client requested to exit.\n");
            break;
        }

        execute_command(buffer, client_socket);
    }
    // Close the client socket when done
    close(client_socket);
}

// Executes the command using execv()
void execute_command(char *command, int client_socket)
{
    char *args[BUFFER_SIZE];
    char  full_command[BUFFER_SIZE];
    char *token;
    char *context;
    int   i = 0;
    pid_t pid;
    int   pipefd[2];

    // Tokenize command into arguments
    token = strtok_r(command, " ", &context);    // First call to strtok_r
    while(token != NULL)
    {
        args[i++] = token;
        token     = strtok_r(NULL, " ", &context);    // Subsequent calls
    }
    args[i] = NULL;

    // Construct full path for the command (assumes it's in /bin or /usr/bin)
    snprintf(full_command, sizeof(full_command), "/bin/%s", args[0]);

    pipe2(pipefd, SOCK_CLOEXEC);

    pid = fork();
    if(pid == 0)
    {
        // Redirect output to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // Execute command using execv
        execv(full_command, args);
        perror("Execution failed");
        exit(1);
    }
    else
    {
        char output[BUFFER_SIZE];
        close(pipefd[1]);

        while(read(pipefd[0], output, BUFFER_SIZE) > 0)
        {
            write(client_socket, output, strlen(output));
            memset(output, 0, BUFFER_SIZE);
        }

        close(pipefd[0]);
        wait(NULL);
    }
}
