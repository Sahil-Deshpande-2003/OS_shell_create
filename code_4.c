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

// Custom execvp function
int custom_execvp(char *cmd, char **args) {
    // Check if the command is an absolute path
    if (cmd[0] == '/') {
        // Absolute path, try executing directly
        execv(cmd, args);
    } else {
        // Relative path or just the command name
        char *token;
        char *path_copy = strdup(path); // Duplicate the PATH variable

        // Tokenize the PATH variable
        token = strtok(path_copy, ":");
        while (token != NULL) {
            char full_path[MAX_PATH_SIZE];
            snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd);

            // Try executing the command with the current path
            execv(full_path, args);

            // Move to the next path in PATH
            token = strtok(NULL, ":");
        }

        free(path_copy); // Free the duplicated PATH variable

        // If we reach here, the command was not found in any path
        return -1;
    }

    // execv should not return, so if it does, it indicates an error
    return -1;
}


void display_prompt(const char *custom_prompt) {
    char prompt_to_display[MAX_PATH_SIZE];

    if (custom_prompt != NULL) {
        // Use the provided custom prompt
        snprintf(prompt_to_display, sizeof(prompt_to_display), "%s", custom_prompt);
    } else {
        // Get the current working directory
        if (getcwd(prompt_to_display, sizeof(prompt_to_display)) == NULL) {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }
    }

    // Print the prompt
    printf("%s$ ", prompt_to_display);
}


void set_prompt(char *new_prompt) {
    printf("new_prompt = %s\n", new_prompt);

    if (new_prompt == NULL || strlen(new_prompt) == 0) {
        printf("Invalid PS1: Missing prompt string\n");
        return;
    }

    // Directly set the prompt
    snprintf(prompt, sizeof(prompt), "%s", new_prompt); // Skip "PS1="
    printf("Prompt set to: %s\n", prompt);
}






void set_path(char *new_path) {
    if (new_path == NULL || strlen(new_path) == 0) {
        printf("Invalid PATH: Missing path string\n");
        return;
    }

    strncpy(path, new_path, sizeof(path));
    path[sizeof(path) - 1] = '\0';
}

void execute_command(char **args) {
    pid_t pid, wpid;
    int status;

    if ((pid = fork()) == 0) {
        // Child process
        if (custom_execvp(args[0], args) == -1) {
            perror("Command not found");
            exit(EXIT_FAILURE);
        }
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


    // Initial display with current working directory
    display_prompt(NULL);


    // ls < file1.txt

    while (1) {
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
                if (args[1] == NULL) {
                    printf("cd: Missing argument\n");
                } else {
                    if (chdir(args[1]) != 0) {
                        perror("chdir");
                    }
                }
            } else if (strncmp(args[0], "PS1=", 4) == 0) {
                // Setting the prompt here
                set_prompt(args[0] + 4);
                // Display the updated prompt
                display_prompt(prompt);
            } else if (strncmp(args[0], "PATH=", 5) == 0) {
                set_path(args[0] + 5);
            } else {
                execute_command(args);
            }
        }
    }

    return 0;
}
