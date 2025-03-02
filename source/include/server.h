//
// Created by brandon-rada on 3/2/25.
//

#ifndef SERVER_H
#define SERVER_H
void handle_client(int client_socket);
void execute_command(char *command, int client_socket);
#endif    // SERVER_H
