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
        snprintf(prompt, sizeof(prompt), "%s$", cwd);  // Update both prompt and CWD
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
            // printf("command1 = %s\n",command);
            // printf("args[i] = %s\n",args[i]);
        }

        // problem in command

        printf("command = %s\n",command);

        char *token;
        token = strtok(command, " ");
        while (token != NULL) {
            char full_path[MAX_PATH_SIZE];
            printf("full_path1 = %s\n",full_path);
            snprintf(full_path, sizeof(full_path), "%s/%s", path, token);
            printf("full_path2 = %s\n",full_path);

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

// ... (rest of the code remains unchanged)

int main() {
    // ... (unchanged code)
    printf("I am here1\n");
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARG_SIZE];
    char *token;

    while (1) {
        display_prompt();
        // ... (unchanged code)
        printf("I am here3\n");
        if (fgets(input, sizeof(input), stdin) == NULL || strcmp(input, "exit\n") == 0) {
            printf("Exiting shell...\n");
            break;
        }
	printf("I am here4\n");
        // Tokenize input
        int i = 0;
        token = strtok(input, " \t\n");
        printf("token1 = %s\n",token);
        while (token != NULL && i < MAX_ARG_SIZE - 1) {
            args[i++] = token;
            token = strtok(NULL, " \t\n");
            printf("token2 = %s\n",token);
            // printf("args[i] = %s\n",args[i]);
        }
        args[i] = NULL;
        
        // Print individual elements of args
for (int j = 0; args[j] != NULL; j++) {
    printf("args[%d] = %s\n", j, args[j]);
}

        if (i > 0) {
            if (strcmp(args[0], "cd") == 0) {
                if (chdir(args[1]) != 0) {
                    perror("chdir");
                } else {
                    // Update both prompt and CWD after successful "cd" command
                    display_prompt();
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
