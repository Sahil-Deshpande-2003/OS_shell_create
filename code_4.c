#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#define MAX_STRING_SIZE 100
#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define MAX_PATH_SIZE 1024
#define MAX_CMD_SIZE 50
#define MAX_FILE_SIZE 50
char prompt[MAX_PATH_SIZE] = "\w$";
char path[MAX_PATH_SIZE] = "/usr/bin:/bin:/sbin";

void removeSpacesAndNewlines_modified(char *str) {
    int length = strlen(str);
    int i, j = 0;
    int spaceCount = 0;

    for (i = 0; i < length; i++) {
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '<' && str[i] != '>') {
            str[j++] = str[i];
            spaceCount = 0;  // Reset space count when a non-space character is encountered.
        } else if (str[i] == ' ') {
            spaceCount++;
            if (spaceCount == 1) {
                str[j++] = ' ';  // Add a single space after the first space.
            }
        }
    }

    str[j] = '\0';
}



void removeSpacesAndNewlines(char *str) {
    int length = strlen(str);
    int i, j = 0;

    for (i = 0; i < length; i++) {
        if (str[i] != ' ' && str[i] != '\n'  && str[i]!='<' && str[i]!='>') {
            str[j++] = str[i];
        }
    }

    str[j] = '\0';
}

void Redirection(char *input) {
    // Tokenize the command to extract command, input file, and output file

    int i = 0;
    char command[MAX_STRING_SIZE] = "";
    char inputFile[MAX_STRING_SIZE] = "";
    char outputFile[MAX_STRING_SIZE] = "";

    while (*input != '\0') {
        while (*input != '\0' && *input != '<') {
            strncat(command, &input[i], 1);
            input++;
        }

        removeSpacesAndNewlines(command);

        while (*input != '\0' && *input != '>') {
            strncat(inputFile, &input[i], 1);
            input++;
        }
        removeSpacesAndNewlines(inputFile);

        while (*input != '\0') {
            strncat(outputFile, &input[i], 1);
            input++;
        }
        removeSpacesAndNewlines(outputFile);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        int inputFd, outputFd;

        if (*inputFile != '\0') {
            inputFd = open(inputFile, O_RDONLY);
            if (inputFd == -1) {
                perror("Error opening input file");
                exit(EXIT_FAILURE);
            }
            dup2(inputFd, STDIN_FILENO);
            close(inputFd);
        }

        if (*outputFile != '\0') {
            outputFd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (outputFd == -1) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
            dup2(outputFd, STDOUT_FILENO);
            close(outputFd);
        }

        char *args[] = {command, NULL};
        execvp(command, args);

        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        wait(NULL);
    }
}


int isCommandValid(char *cmd) {
    char *token;
    char *path_copy = strdup(path);

    token = strtok(path_copy, ":");
    while (token != NULL) {
        char full_path[MAX_PATH_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd);

        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return 1;  // Command is valid
        }

        token = strtok(NULL, ":");
    }

    free(path_copy);
    return 0;  // Command is not valid
}


void performInputRedirection(char *input) {
    int p;
    char cmd[MAX_CMD_SIZE];
    char file[MAX_FILE_SIZE];

    char *redirection_symbol = strchr(input, '<');

    if (redirection_symbol != NULL) {
        // Input redirection is present, extract the command and file
        *redirection_symbol = '\0';  // Separate the command

        // Skip leading whitespaces in the command
        while (*input == ' ' || *input == '\t') {
            input++;
        }
        strcpy(cmd, input);
        redirection_symbol++; // Move to the character after '<'
        while (*redirection_symbol == ' ' || *redirection_symbol == '\t') {
            redirection_symbol++;
        }
        strcpy(file, redirection_symbol);
    } else {
        // No input redirection, use the entire input as the command
        strcpy(cmd, input);
        file[0] = '\0';  // Empty string for file
    }

    p = fork();
    if (p == 0) {
        // Child process
        close(0);

        if (file[0] != '\0') {
            removeSpacesAndNewlines(file);
            int file_fd = open(file, O_RDONLY);
            if (file_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }

            dup2(file_fd, STDIN_FILENO);
            close(file_fd);
        }

        removeSpacesAndNewlines(cmd);

        // printf("cmd=%s\n", cmd);
        // printf("file=%s\n", file);
        execlp(cmd, cmd, file, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

void performOutputRedirection(char *input) {
    char cmd[50];
    char file[50];

    int original_stdout = dup(fileno(stdout));

    char *redirection_symbol = strchr(input, '>');

    if (redirection_symbol != NULL) {
        // Output redirection is present, extract the command and file
        *redirection_symbol = '\0';  // Separate the command
        strcpy(cmd, input); // combine karte waqt cmd has to be cat file1.txt
   
        // Remove spaces from the command
        removeSpacesAndNewlines(cmd); // UNCOMMENT KAR!!!
        // removeSpacesAndNewlines_modified(cmd);

        // printf("cmd=%s\n",cmd);
        strcpy(file, redirection_symbol + 1);
        removeSpacesAndNewlines(file);
        // printf("file=%s\n",file);

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process

            // Open a file for writing
  
            int file_descriptor = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (file_descriptor == -1) {
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }

            // Redirect stdout to the file
            // printf("Before dup2\n");
            if (dup2(file_descriptor, fileno(stdout)) == -1) {
                perror("Error redirecting stdout");
                close(file_descriptor);  // Close the file descriptor before exiting
                exit(EXIT_FAILURE);
            }

            // printf("After dup2\n");
            // Close the file descriptor to allow the command to write to the redirected stdout
            close(file_descriptor);

            // Now, execute the command, which will write to the redirected stdout
            execlp(cmd, cmd, (char *)NULL);

            // If execlp fails, print an error message
            perror("Error running command");

            // Exit the child process
            exit(EXIT_FAILURE);
        } else {
            // Parent process

            int status;
            waitpid(pid, &status, 0);
            // printf("I am here1\n");
            // Restore the original stdout
            dup2(original_stdout, fileno(stdout));
            // printf("I am here2\n");

            // Close the original stdout file descriptor
            close(original_stdout);
        }
    }

    // The parent process should continue here
}

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


    if (custom_prompt[strlen(custom_prompt) - 2] != '$') {
        printf("%s $ ", custom_prompt);
    } else {
        printf("%s ", custom_prompt);
    }
}


void set_prompt(char *new_prompt) {
    if (new_prompt == NULL || strlen(new_prompt) == 0) {
        printf("Invalid PS1: Missing prompt string\n");
        return;
    }

    if (strcmp(new_prompt, "\"\\w$\"") == 0) {
        getcwd(prompt, MAX_PATH_SIZE);

    } else {
         strcpy(prompt, new_prompt);

    }

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

        // printf("Updated PATH (set_path): %s\n", path);

        token = strtok(NULL, ":");
    }

    // Remove the trailing ":"
    if (strlen(path) > 0) {
        path[strlen(path) - 1] = '\0';
    }

    free(new_path_copy);
}

void execute_command(char **args) {
    if (!isCommandValid(args[0])) {
        fprintf(stderr, "%s: Command not found\n", args[0]);
        return;
    }

    pid_t pid, wpid;
    int status;


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

void extractCommandAndFilename(char *input, char **command, char **filename) {
    // Tokenize the input to extract command and filename

    char *token = strtok(input, " \t\n");

    if (token != NULL) {

        *command = strdup(token);

        token = strtok(NULL, " \t\n");
        if (token != NULL) {
            *filename = strdup(token);
        } else {
            *filename = NULL;
        }
    } else {
        *command = NULL;
        *filename = NULL;
    }
}

int main() {
    int p;
      char *input = malloc(MAX_INPUT_SIZE);  // Dynamically allocate memory
    char *args[MAX_ARG_SIZE];
    char *token;
    getcwd(prompt, MAX_PATH_SIZE);

    char cmd[MAX_CMD_SIZE];
    char file[MAX_FILE_SIZE];
    while (1) {

        display_prompt(prompt);
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL || strcmp(input, "exit\n") == 0) {
            printf("Exiting shell...\n");
            break;
        }

        char *redirection_symbol1 = strchr(input, '<');
        char *redirection_symbol2 = strchr(input, '>');

        if (redirection_symbol1!=NULL && redirection_symbol2!=NULL){
            Redirection(input);
            continue;
        }


        if (redirection_symbol1 != NULL) {

            performInputRedirection(input);

            continue;
        }


        if(redirection_symbol2 != NULL){
            performOutputRedirection(input);

            continue;

        }

        char *command = "default_command";
        char *filename = "default_filename";
         
        char **commandAddress = &command;
        char **fileAddress = &filename;
        char *input_copy = strdup(input);
        extractCommandAndFilename(input_copy,commandAddress,fileAddress);
         
        




        int i = 0;

        token = strtok(input, " \t\n");

        while (token != NULL && i < MAX_ARG_SIZE - 1) {
            args[i] = token;

            i++;
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
                strcpy(prompt,args[1]);
                

            } else if (strncmp(args[0], "PS1=", 4) == 0) {
                set_prompt(args[0] + 4);
            } else if (strncmp(args[0], "PATH=", 5) == 0) {
                set_path(args[0] + 5);
                // print_path_contents();
            } else {

                if (!isCommandValid(command)) {
                        fprintf(stderr, "%s: Command not found\n", args[0]);
                        continue;
                    }

       
                execute_command(args);
   
            }
        } 
   

    }

    free(input);

        
    return 0;
}
