#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

int main() {
    int p;
    char input[100];
    char cmd[50];
    char file[50];
    chdir("/home/sahil/Pictures/OS_assignment_clone/OS_shell_create");

    // Read the entire input as a single string until a newline character
    scanf(" %[^\n]", input);

    printf("input  = %s\n", input);

    // Find the position of '<'
    char *redirection_symbol = strchr(input, '<');

    printf("redirection_symbol = %p\n",redirection_symbol);

    if (redirection_symbol != NULL) {
        // Input redirection is present, extract the command and file
        *redirection_symbol = '\0';  // Separate the command
        strcpy(cmd, input);

        strcpy(file, redirection_symbol + 1);
        printf("file1 = %s\n", file);
    } else {
        // No input redirection, use the entire input as the command
        strcpy(cmd, input);
        file[0] = '\0';  // Empty string for file
    }

    p = fork();

    if (p == 0) {
        // Child process
        close(0);

        // Determine if input redirection is requested
        if (file[0] != '\0') {
            printf("file2 = %s\n", file);
            // int file_fd = open(file, O_RDONLY);
        printf("List the files in the current dir\n");
             DIR *dir;
    struct dirent *entry;

    // Open the current directory
    dir = opendir(".");

    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Read and print each entry in the directory
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    // Close the directory
    closedir(dir);

    printf("Over\n");
            int file_fd = open(file, O_RDONLY);

             if (file_fd!=-1) printf("File opened success\n");

            else printf("File opened failed\n");
            printf("hey!\n");
            if (file_fd == -1) {
                printf("hey1!\n");
                perror("open");
                exit(EXIT_FAILURE);
            }

            printf("he2!\n");

            dup2(file_fd, STDIN_FILENO);
            close(file_fd);
        }

        // Execute the command with proper arguments
        execlp(cmd, cmd, (char *)NULL);
        perror("execlp"); // Print error if execlp fails
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        wait(NULL);
    }

    return 0;
}
