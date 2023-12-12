/*
    COMP3511 Fall 2023
    PA1: Simplified Linux Shell (MyShell)

    Your name: CHLOE HU
    Your ITSC email: chuap@connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

/*
    Header files for MyShell
    Necessary header files are included.
    Do not include extra header files
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls
#include <signal.h>   // For signal handling

// Define template strings so that they can be easily used in printf
//
// Usage: assume pid is the process ID
//
//  printf(TEMPLATE_MYSHELL_START, pid);
//
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"
#define TEMPLATE_MYSHELL_TERMINATE "Myshell (pid=%d) terminates by Ctrl-C\n"

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LENGTH 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
// We also need to add an extra NULL item to be used in execvp
// Thus: 8 + 1 = 9
//
// Example:
//   echo a1 a2 a3 a4 a5 a6 a7
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the standard file descriptor IDs here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

// This function will be invoked by main()
// This function is given
int get_cmd_line(char *command_line)
{
    int i, n;
    if (!fgets(command_line, MAX_CMDLINE_LENGTH, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(command_line);
    command_line[--n] = '\0';
    i = 0;
    while (i < n && command_line[i] == ' ')
    {
        ++i;
    }
    if (i == n)
    {
        // Empty command
        return -1;
    }
    return 0;
}

// this function interrupts ctrl-c
void signalHandler_Int(int sig) {
    pid_t pid = getpid();
    printf(TEMPLATE_MYSHELL_TERMINATE, pid);
    exit(0);
}
// read_tokens function is given
// This function helps you parse the command line
//
// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char command_line[MAX_CMDLINE_LENGTH]; // The input command line
//
// Sample usage:
//
//  read_tokens(pipe_segments, command_line, &num_pipe_segments, "|");
//
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

void process_cmd(char *command_line)
{
    // Uncomment this line to check the cmdline content
    //printf("Debug: The command line is [%s]\n", command_line);

    char *pipe_segments[MAX_PIPE_SEGMENTS];
    int num_pipe_segments;

    char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
    int num_arguments;
    // declare file descriptors
    int pfds[2];

    read_tokens(pipe_segments, command_line, &num_pipe_segments, "|");
    for (int i = 0; i < num_pipe_segments; i++) {
        // read args from each pipe seg
        read_tokens(arguments, pipe_segments[i], &num_arguments, SPACE_CHARS);
        //needed for execvp
        arguments[num_arguments] = NULL;

        //redirection
        for (int j = 0; j < num_arguments; j++) {
            
            if (strcmp(arguments[j], "<") == 0 && arguments[j+1] != NULL) {
                // j + 1 gets the file name
                int file_descriptor = open(arguments[j+1], O_RDONLY, S_IRUSR | S_IWUSR);
                dup2(file_descriptor, STDIN_FILENO);
                close(file_descriptor);
                arguments[j] = NULL;
            } else if (strcmp(arguments[j], ">") == 0 && arguments[j+1] != NULL) { // output redirection
                int file_descriptor = open(arguments[j+1], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
                dup2(file_descriptor, STDOUT_FILENO);
                close(file_descriptor);
                arguments[j] = NULL;
            }
        }

        if (i + 1 >= num_pipe_segments) {
            // execute commands
            execvp(arguments[0], arguments);
        } else {
            // create fds
            pipe(pfds);
            pid_t pid = fork();
            // child process, open only the read
            if (pid == 0) {
                close(pfds[0]);
                dup2(pfds[1], 1);
                execvp(arguments[0], arguments);
            // parent process
            } else {
                close(pfds[1]);
                dup2(pfds[0], 0);
                // wait for child
                wait(NULL);
            }
        }
    }
}

/* The main function implementation */
int main()
{
    char *prompt = "chuap";
    char command_line[MAX_CMDLINE_LENGTH];
    printf(TEMPLATE_MYSHELL_START, getpid());
    //ctrl-c, overwrite to sigint handler
    signal(SIGINT, signalHandler_Int);
    // The main event loop
    while (1)
    {

      printf("%s> ", prompt);
      if (get_cmd_line(command_line) == -1)
          continue; /* empty line handling */
        
      //exit 
      if(strcmp(command_line, "exit") == 0) {
        pid_t pid = getpid();
        printf(TEMPLATE_MYSHELL_END, pid);
        exit(0);
      }
      pid_t pid = fork();
      if (pid < 0) {
        printf("Error\n");
      }
      else if (pid == 0)
      {
          // the child process handles the command
          process_cmd(command_line);
      }
      else
      {
          // the parent process simply wait for the child and do nothing
          wait(0);
      }

    }

    return 0;
}