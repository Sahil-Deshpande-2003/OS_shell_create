#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#define MAX_STRING_SIZE 100
#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define MAX_PATH_SIZE 1024
#define MAX_CMD_SIZE 50
#define MAX_FILE_SIZE 50
char prompt[MAX_PATH_SIZE] = "\w$";
char path[MAX_PATH_SIZE] = "/usr/bin:/bin:/sbin";
 volatile sig_atomic_t foreground_pid = -1;
#define CMD_SIZE 1024
#define MAX_INPUT_SIZE 1024
#define MAX_HISTORY_SIZE 100
char history[MAX_HISTORY_SIZE][MAX_INPUT_SIZE];
int history_count = 0;
#define MAX_JOBS 100
#define MAX_LINE_LENGTH 1024
#define MAX_COMMAND_LEN 256
#define MAX_PID_LEN 10
int num_jobs = 0;
typedef struct jobs{
    long job_id; // is this correct format specifier
    pid_t pid;
    char command[MAX_INPUT_SIZE];
    int running_in_background;
}jobs;

jobs background_jobs[MAX_JOBS];

void add_to_history(const char *cmd) {
    if (history_count < MAX_HISTORY_SIZE) {
        strcpy(history[history_count], cmd);
        history_count++;
    } else {
        for (int i = 1; i < MAX_HISTORY_SIZE; i++) {
            strcpy(history[i - 1], history[i]); // delete 1st element from history
        }
        strcpy(history[MAX_HISTORY_SIZE - 1], cmd);
    }
}

void display_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
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



// check if useful

void grep(FILE *file, const char *pattern) {
    printf("Hey!\n");

    // char line[MAX_LINE_LENGTH];
    // int line_number = 0;

    // while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
    //     line_number++;
    //     if (strstr(line, pattern) != NULL) {
    //         printf("%s:%d:%s", pattern, line_number, line);
    //     }
    // }

    char line[MAX_LINE_LENGTH];
    int line_number = 0;

    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        line_number++;
        if (strstr(line, pattern) != NULL) {
            printf("%s:%d:%s", pattern, line_number, line);
        }
    }
}






int custom_execvp(char *cmd, char **args) {     

    // printf("Arguments:\n");
    int cnt = 0;
    for (int i = 0; args[i] != NULL; i++) {
        printf("args[i] = %s\n",args[i]);
        cnt++;
    }

    // printf("cnt = %d\n",cnt);

    args[cnt] = NULL;


     if (strcmp(args[0], "sleep") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "Usage: sleep <seconds>\n");
            exit(EXIT_FAILURE);
        }
        unsigned int seconds = atoi(args[1]);
        sleep(seconds);
        exit(EXIT_FAILURE);
    }






    if (cmd[0] == '/') {

        // printf("Inside if\n"); 

        execv(cmd, args);
    } else {
        // printf("Inside else\n");
        char *token;
        char *path_copy = strdup(path);

        token = strtok(path_copy, ":");

        while (token != NULL) {
            char full_path[MAX_PATH_SIZE];
            //  Constructs the full path of the executable file by combining the current token (representing a directory in the path) and the command (cmd).It uses snprintf() function to safely concatenate the strings, ensuring that it doesn't overflow the buffer size specified by MAX_PATH_SIZE.
            snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd);
            if (access(full_path, X_OK) == 0) {

                // Checks if the constructed full path is executable 

                removeSpacesAndNewlines(*args);
                removeSpacesAndNewlines(full_path);

                int exec_result = execv(full_path, args);

                 if (exec_result == -1) {
                    /*
                    perror() is a function in C that is used to print a descriptive error message to the standard error stream (stderr).
                    */
                    perror("execv");
                    exit(EXIT_FAILURE);
                }

            }

            /*
             When strtok() is called with a non-NULL first argument, it treats that argument as a pointer to the string to tokenize. However, when strtok() is called with a NULL first argument, it continues tokenizing the string that was previously passed to it. This allows strtok() to maintain state between calls, remembering where it left off in the string
            */

            token = strtok(NULL, ":");
        }
        free(path_copy);
        fprintf(stderr, "%s: No such file or directory\n", cmd);
        exit(EXIT_FAILURE);
    }
    perror("execv");
    exit(EXIT_FAILURE);
}


int isCommandValid(char *cmd) {
    char *token;
    char *path_copy = strdup(path);

    token = strtok(path_copy, ":");
    while (token != NULL) {
        char full_path[MAX_PATH_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd);

        if (access(full_path, X_OK) == 0) {

            /*
            Checks if the constructed full path is executable (X_OK permission). i.e. if it can find a file in the path which can be executed
            */
            free(path_copy);
            return 1;  // Command is valid
        }

        token = strtok(NULL, ":");
    }

    free(path_copy);
    return 0;  // Command is not valid
}

void execute_command(char **args,int is_background) {
    if (!isCommandValid(args[0])) {
        fprintf(stderr, "%s: Command not found\n", args[0]);
        return;
    }


    pid_t pid, wpid;
    int status;

    /*
    In the parent process, it waits for the child process to finish executing using waitpid().
    It continues waiting until the child process either exits normally, is terminated by a signal, or the command is supposed to run in the background (is_background is non-zero).
    During this waiting, it checks the status of the child process using macros like WIFEXITED and WIFSIGNALED to determine whether the child process exited normally or was terminated by a signal.
    */


    if ((pid = fork()) == 0) {
        if (!is_background && custom_execvp(args[0], args) == -1) {
            perror("Command not found");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("fork");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status) && !is_background);
    }
}






void execute_fg_command(){

    if (num_jobs == 0) {
        printf("No background jobs to bring to foreground.\n");
        return;
    }

    // execute the most recent command from jobs array by doing wait pid 

    char *command = background_jobs[num_jobs - 1].command;
    int is_background = 0;
    char **args = malloc(2 * sizeof(char *));
    if (args == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }

    // Set args[0] to command and args[1] to NULL
    args[0] = command;
    args[1] = NULL;
    printf("Bringing background job to foreground: %s\n", background_jobs[num_jobs - 1].command);
    removeSpacesAndNewlines(*args);
    execute_command(args, is_background);
    
}



void add_process_to_background(pid_t pid,char *command){

    if (num_jobs >= MAX_JOBS) {
        printf("Maximum number of background jobs reached.\n");
        return;
    }

    // printf("Printig command inside add_process_to_background = %s\n",command);

    background_jobs[num_jobs].job_id = num_jobs + 1;
    background_jobs[num_jobs].pid = pid;
    strcpy(background_jobs[num_jobs].command, command);
    num_jobs++;
    printf("Added to Background[%ld] %d %s\n", background_jobs[num_jobs - 1].job_id, background_jobs[num_jobs - 1].pid,background_jobs[num_jobs-1].command);
}








void Redirection(char *input) {
    // Tokenize the command to extract command, input file, and output file

    int i = 0;
    char command[MAX_STRING_SIZE] = "";
    char inputFile[MAX_STRING_SIZE] = "";
    char outputFile[MAX_STRING_SIZE] = "";

    while (*input != '\0') {
        while (*input != '\0' && *input != '<') {
            strncat(command, &input[i], 1); // apppend 1 char to command till you find < char 
            input++;
        }

        removeSpacesAndNewlines(command);

        while (*input != '\0' && *input != '>') {

            // '<' found so concat filename till you dont find '>'

            strncat(inputFile, &input[i], 1);
            input++;
        }
        removeSpacesAndNewlines(inputFile);

        while (*input != '\0') {
            // concat o/p filename till u dont find NULL char
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

    //    Parent Process waits for the child process to finish executing the command using

        wait(NULL);
    }
}




void performInputRedirection(char *input) {

    // printf("Inside performInputRedirection\n");
    // printf("HERE input = %s\n",input);
    int p;
    char cmd[MAX_CMD_SIZE];
    char file[MAX_FILE_SIZE];

    // The strchr() function finds the first occurrence of a character in a string. 

    char *redirection_symbol = strchr(input, '<');

    if (redirection_symbol != NULL) {

        // printf("Inside if\n");

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
        // printf("redirection_symbol = %s\n",redirection_symbol);
        strcpy(file, redirection_symbol);
    } else {
        // No input redirection, use the entire input as the command
        strcpy(cmd, input);
        file[0] = '\0';  // Empty string for file
    }

    p = fork();
    if (p == 0) {

        // printf("Inside child process\n");
        // Child process
        close(0);

        if (file[0] != '\0') {
            removeSpacesAndNewlines(file);
            int file_fd = open(file, O_RDONLY);
            if (file_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }

            dup2(file_fd, STDIN_FILENO); // read from the file instead of keyboard
            close(file_fd);
        }

        removeSpacesAndNewlines(cmd);

        /*
        execlp enables you to switch out the current process image for a different one that is specified by the given program path.
        */
    //    printf("Before execlp\n");
        execlp(cmd, cmd, file, NULL);
        // printf("execlp failed....");
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

            /*
            This line waits for the child process with the PID pid to complete. The parent process is blocked until the child process terminates. The status of the child process is stored in the variable status.
            */

            waitpid(pid, &status, 0);
            
            /*
            This line restores the original standard output by duplicating the file descriptor original_stdout onto the file descriptor for standard output (stdout). It essentially undoes any redirection that might have been done earlier in the program.
            */

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
            /*
            It reads each entry in the directory using readdir().
            For each entry, it prints its name using printf().
            Once all entries are read, it closes the directory using closedir().
            */
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

void execute_job_command(){

    for (int i = 0; i < num_jobs; i++)
    {
        printf("command : %s\n",background_jobs[i].command);
    }

    return;
    
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
    

    //  if new_prompt is equal to "\w$" it retrieves the current working directory and stores it in the variable prompt.

    if (strcmp(new_prompt, "\"\\w$\"") == 0) {
        getcwd(prompt, MAX_PATH_SIZE);

    } else {
         strcpy(prompt, new_prompt); // copy the string stored in the variable new_prompt into the character array prompt

    }

}

void set_path(char *new_path) {
    if (new_path == NULL || strlen(new_path) == 0) {
        printf("Invalid PATH: Missing path string\n");
        return;
    }

    printf("Curr path = %s\n",new_path);

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





int find_pid(char *command) {
    // printf("command received by find_pid = %s\n",command);
    removeSpacesAndNewlines(command);

    FILE *fp;
    char ps_command[MAX_COMMAND_LEN];
    char line[MAX_COMMAND_LEN];
    char pid[MAX_PID_LEN];
    int found = 0;

    snprintf(ps_command, sizeof(ps_command), "ps");

    fp = popen(ps_command, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    // Read the output of ps command line by line
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Check if the command matches the desired command
        // printf("line = %s\n",line);
        if (strstr(line, command) != NULL) {
            /*
            Checks if the command string matches any line of output obtained from the ps command. If a match is found, it implies that a process associated with that command is running.

            If a match is found, extracts the PID from the line of output obtained from the ps command
            */
            sscanf(line, "%s", pid);
            found = 1;
            break;
        }
    }

    pclose(fp);

    if (found) {
        return atoi(pid);
    } else {
        return -1; // Return -1 if the command is not found
    }
}

int execute_with_pipes(char *cmds[][20], int num_cmds) {
    // printf("num_cmds = %d\n",num_cmds);
    int pipes[num_cmds - 1][2]; // Array to store pipe file descriptors

    // Create pipes
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return -1;
        }
    }

    // Fork processes and set up pipes
    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            return -1;
        } else if (pid == 0) {
            // Child process

            // If it's not the first command, redirect stdin to read from the previous pipe
            
            if (i != 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // If it's not the last command, redirect stdout to write to the next pipe
            
            if (i != num_cmds - 1) {
                // if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe file descriptors
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            // printf("Before execp\n");
            execvp(cmds[i][0], cmds[i]);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    // Close pipe file descriptors in the parent process
    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < num_cmds; i++) {
        wait(NULL);
    }

    return 0;
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

        int is_background = 0;

        display_prompt(prompt);
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL || strcmp(input, "exit\n") == 0) {
            printf("Exiting shell...\n");
            break;
        }

   
        add_to_history(input);

        // 1st check for pipe


        char *pipe_pos = strchr(input,'|');

        if (pipe_pos!=NULL){


            input[strcspn(input, "\n")] = '\0';
            char *token;
            char *rest = input;
            char *cmds[20][20]; // Assuming maximum 10 commands with 10 arguments each
            
            int cmdIndex = 0;
            int argIndex = 0;
            int cmd_ct = 0;
        
            while ((token = strtok_r(rest, "|", &rest))) {
                argIndex = 0;
                cmd_ct++;
                char *arg = strtok(token, " ");
                while (arg != NULL) {
                    cmds[cmdIndex][argIndex] = strdup(arg); // Allocate memory for subpart and copy it
                    argIndex++;
                    arg = strtok(NULL, " ");
                }
                cmds[cmdIndex][argIndex] = NULL; // Mark the end of arguments with NULL
                cmdIndex++;

            }


       

 

            execute_with_pipes(cmds,cmd_ct);
    

    // Freeing dynamically allocated memory
            for (int i = 0; i < cmdIndex; i++) {
                for (int j = 0; cmds[i][j] != NULL; j++) {
                    free(cmds[i][j]);
                }
            }

            is_background = 1;


        }

        char output_string[100];

       char *ampersand_position = strchr(input, '&');

    //    2nd -> check for &

        if (ampersand_position != NULL) {
            // Calculate the length of the substring prior to '&'
            // Calculate the length of the substring prior to '&'
            int length = ampersand_position - input;

            // Overwrite the input string with the substring prior to '&'
            memmove(input, input, length);
            input[length] = '\0'; // Null-terminate the input string

            // printf("Modified input string: %s\n", input);

            is_background = 1;

            execute_command(&input,is_background);

            // printf("i/p being passed to find_pid = %s\n",input);

            int pid_passing  = find_pid(input);

            // printf("Passing pid = %d\n",pid_passing);

            add_process_to_background(pid_passing,input);
            
        } 

        //    3rd -> check for <


        char *redirection_symbol1 = strchr(input, '<');

         //    4th -> check for >

        char *redirection_symbol2 = strchr(input, '>');

        if (redirection_symbol1!=NULL && redirection_symbol2!=NULL){
            Redirection(input); // for input and output  redirection
        }


        if (redirection_symbol1 != NULL) {

            performInputRedirection(input);


            printf("Life after performInputRedirection");


        }


        if(redirection_symbol2 != NULL){
            performOutputRedirection(input);

        }

        char *command = "default_command";
        char *filename = "default_filename";
         
        char **commandAddress = &command;
        char **fileAddress = &filename;
        char *input_copy = strdup(input);
        extractCommandAndFilename(input_copy,commandAddress,fileAddress);
         
        //  using space, tab, and newline characters as delimiters tokenize the string to find command and filename





        int i = 0;

        token = strtok(input, " \t\n");

        // tokenize the input str based on  using space, tab, and newline characters as delimiters.

        while (token != NULL && i < MAX_ARG_SIZE - 1) {
            args[i] = token;
            //  printf("args!!!!! = %s\n",args[i]);

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
                

            }
            // check if this is useful!!

            else if (strcmp(args[0], "grep") == 0) {
                FILE *file = fopen(args[2], "r");
                if (file == NULL) {
                   perror("Error");
                    return 1;
    }
    // grep(stdin, args[1]);
                grep(file, args[1]);
            } 
             else if (strncmp(args[0], "PS1=", 4) == 0) {
                set_prompt(args[0] + 4);
            }else if (strcmp(args[0], "history") == 0) {
            display_history();
        } else if (strncmp(args[0], "PATH=", 5) == 0) {
            printf("Before calling set_path, path = %s\n",path);
                set_path(args[0] + 5);
                print_path_contents();
            }else if (strcmp(args[0], "fg") == 0){

                execute_fg_command();
            } 
            else if (strcmp(args[0], "jobs") == 0){
                execute_job_command();
            }
            else {

               
                if (!isCommandValid(command)) {
                        fprintf(stderr, "%s: Command not found\n", args[0]);
                        continue;
                    }
       
                execute_command(args,is_background);
   
            }
        } 
   

    }

    free(input);

        
    return 0;
}