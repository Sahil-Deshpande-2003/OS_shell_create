#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define MAX_PATH_SIZE 1024

char prompt[MAX_PATH_SIZE] = "\w$";
char path[MAX_PATH_SIZE] = "/usr/bin:/bin:/sbin";


void print_path_contents() {
    char *token;
    char *path_copy = strdup(path);

    token = strtok(path_copy, ":");
    while (token != NULL) {
        printf("Contents of %s:\n", token);

        DIR *dir = opendir(token);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                printf("%s\n", entry->d_name);
            }

            closedir(dir);
        } else {
            perror("opendir");
        }

        token = strtok(NULL, ":");
    }

    free(path_copy);
}


int custom_execvp(char *cmd, char **args) {
    if (cmd[0] == '/') {
        execv(cmd, args);
    } else {
        char *token;
        char *path_copy = strdup(path);

        token = strtok(path_copy, ":");
        while (token != NULL) {
            char full_path[MAX_PATH_SIZE];
            snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd);

            if (access(full_path, X_OK) == 0) {
                execv(full_path, args);
            }

            token = strtok(NULL, ":");
        }

        free(path_copy);

        fprintf(stderr, "%s: No such file or directory\n", cmd);
        exit(EXIT_FAILURE);
    }

    perror("execv");
    exit(EXIT_FAILURE);
}

void display_prompt(const char *custom_prompt) {
    char prompt_to_display[MAX_PATH_SIZE];

    if (custom_prompt != NULL && strcmp(custom_prompt, "\\w$") == 0) {
        display_prompt(NULL);
        return;
    }

    if (custom_prompt != NULL) {
        snprintf(prompt_to_display, sizeof(prompt_to_display), "%s", custom_prompt);
        if (prompt_to_display[strlen(prompt_to_display) - 1] != '$') {
            strcat(prompt_to_display, "$");
        }
    } else {
        if (getcwd(prompt_to_display, sizeof(prompt_to_display)) == NULL) {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }
        strcat(prompt_to_display, "$");
    }

    printf("%s ", prompt_to_display);
}

void set_prompt(char *new_prompt) {
    if (new_prompt == NULL || strlen(new_prompt) == 0) {
        printf("Invalid PS1: Missing prompt string\n");
        return;
    }

    if (strcmp(new_prompt, "\"\\w$\"") == 0) {
        display_prompt(NULL);
    } else {
        display_prompt(new_prompt);
    }

    snprintf(prompt, sizeof(prompt), "%s", new_prompt);
}
void set_path(char *new_path) {
    if (new_path == NULL || strlen(new_path) == 0) {
        printf("Invalid PATH: Missing path string\n");
        return;
    }

    char *token;
    char *new_path_copy = strdup(new_path);

    // Clear the existing path
    path[0] = '\0';

    token = strtok(new_path_copy, ":");
    while (token != NULL) {
        // Concatenate the token to the existing path with ":"
        strcat(path, token);
        strcat(path, ":");

        printf("Updated PATH (set_path): %s\n", path);

        token = strtok(NULL, ":");
    }

    // Remove the trailing ":"
    if (strlen(path) > 0) {
        path[strlen(path) - 1] = '\0';
    }

    free(new_path_copy);
}

void execute_command(char **args) {
    pid_t pid, wpid;
    int status;

    printf("Executing command with PATH: %s\n", path);

    if ((pid = fork()) == 0) {
        if (custom_execvp(args[0], args) == -1) {
            perror("Command not found");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("fork");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARG_SIZE];
    char *token;

    display_prompt(NULL);

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
                set_prompt(args[0] + 4);
            } else if (strncmp(args[0], "PATH=", 5) == 0) {
                set_path(args[0] + 5);
                print_path_contents(); 
            } else {
                execute_command(args);
                display_prompt(prompt);
            }
        } else {
            display_prompt(prompt);
        }
    }

    return 0;
}
