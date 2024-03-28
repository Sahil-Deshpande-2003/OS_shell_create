#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define MAX_PATH_SIZE 1024

char prompt[MAX_PATH_SIZE] = "\w$";
char path[MAX_PATH_SIZE] = "/usr/bin";

void display_prompt() {
    char cwd[MAX_PATH_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(prompt, sizeof(prompt), "%s$", cwd);
        printf("%s ", prompt);
    } else {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
}

void set_prompt(char *new_prompt) {
    strncpy(prompt, new_prompt, sizeof(prompt));
    prompt[sizeof(prompt) - 1] = '\0';
}

void set_path(char *new_path) {
    strncpy(path, new_path, sizeof(path));
    path[sizeof(path) - 1] = '\0';
}

void execute_command(char **args) {
    pid_t pid, wpid;
    int status;

    if ((pid = fork()) == 0) {
        // Child process
        char command[MAX_PATH_SIZE] = "";
        for (int i = 0; args[i] != NULL; i++) {
            strcat(command, args[i]);
            strcat(command, " ");
        }

        char *token;
        token = strtok(command, " ");
        while (token != NULL) {
            char full_path[MAX_PATH_SIZE];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, token);

            if (access(full_path, X_OK) == 0) {
                execv(full_path, args);
            }

            token = strtok(NULL, " ");
        }

        perror("Command not found");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARG_SIZE];
    char *token;

    while (1) {
        display_prompt();
        if (fgets(input, sizeof(input), stdin) == NULL || strcmp(input, "exit\n") == 0) {
            printf("Exiting shell...\n");
            break;
        }

        int i = 0;
        token = strtok(input, " \t\n");
        while (token != NULL && i < MAX_ARG_SIZE - 1) {
            args[i++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[i] = NULL;

        if (i > 0) {
            if (strcmp(args[0], "cd") == 0) {
                if (chdir(args[1]) != 0) {
                    perror("chdir");
                }
            } else if (strncmp(args[0], "PS1=", 4) == 0) {
                set_prompt(args[0] + 4);
            } else if (strncmp(args[0], "PATH=", 5) == 0) {
                set_path(args[0] + 5);
            } else {
                execute_command(args);
            }
        }
    }

    return 0;
}