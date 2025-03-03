//
// Created by brandon-rada on 3/2/25.
//

#include "server_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void execute_exit(char *args[]) {
    int exit_status = 0;
    if (args[1] != NULL) {
        exit_status = atoi(args[1]); // Convert exit status to integer
    }
    printf("Exiting with status %d\n", exit_status);
    exit(exit_status);
}

void execute_cd(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: expected argument\n");
    } else if (chdir(args[1]) != 0) {
        perror("cd failed");
    }
}

void execute_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd failed");
    }
}

void execute_echo(char *args[]) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s ", args[i]);
    }
    printf("\n");
}

void execute_type(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "type: missing argument\n");
        return;
    }
    if (strcmp(args[1], "exit") == 0 || strcmp(args[1], "cd") == 0 ||
        strcmp(args[1], "pwd") == 0 || strcmp(args[1], "echo") == 0 ||
        strcmp(args[1], "type") == 0) {
        printf("%s is a shell builtin\n", args[1]);
        } else {
            printf("%s is an external command\n", args[1]);
        }
}