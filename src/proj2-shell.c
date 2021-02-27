/*  
    Justin Stitt
    2/24/2021

    UNIX Shell Project

    to-do: 
    * & (no-wait)
    * parent/child pipe communication ??? (piping)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


char** tokparse(char* input, char* cargs[]){
    const char delim[2] = {' ', '\t'};// delimiters (space and tab)
    char* token;
    int num_args = 0;

    token = strtok(input, delim);
    cargs[0] = token;

    char** REDIR = malloc(2 * sizeof(char*));
    for(int x = 0; x < 2; ++x)
        REDIR[x] = malloc(BUFSIZ * sizeof(char));
    REDIR[0] = "";// i/o
    REDIR[1] = "";// path

    while(token != NULL){
        // add token to cargs
        //printf("token %d: %s\n", num_args, token);
        token = strtok(NULL, delim);
        if(token == NULL) break;
        if(!strncmp(token, ">", 1)){
            token = strtok(NULL, delim);
            REDIR[0] = "o";
            REDIR[1] = token;
            return REDIR;
        } 
        else if(!strncmp(token, "<", 1)){
            token = strtok(NULL, delim);
            REDIR[0] = "i";
            REDIR[1] = token;
            return REDIR;
        }
        cargs[++num_args] = token;
    }
    return REDIR;
}

int main(int argc, const char* argv[]){
    char input [BUFSIZ];
    char MRU   [BUFSIZ]; // most-recently used cache

    // clear buffers
    memset(input, 0, BUFSIZ * sizeof(char));
    memset(MRU,   0, BUFSIZ * sizeof(char));

    while(1){
        printf("osh> ");  // command line prefix
        fflush(stdout);   // force flush to console
        // read into input from stdin
        fgets(input, BUFSIZ, stdin);
        input[strlen(input) - 1] = '\0'; // replace newline with null
        // edge cases
        if(strncmp(input, "exit", 4) == 0)// quit
            return 0;
        if(strncmp(input, "!!", 2)) // history (MRU cache)
            strcpy(MRU, input);      

        pid_t pid = fork();
        if(pid < 0){   // failed to create child process
            fprintf(stderr, "fork failed...\n");
            return -1; // exit code
        }
        if(pid != 0){  // parent
            //do parent stuff like wait, etc.
            wait(NULL);// turn off if used with '&'
            //printf("child completed...\n");
        }
        else{          // child
            //printf("child process start...\n");
            char* cargs[BUFSIZ];    // buffer to hold command line arguments
            memset(cargs, 0, BUFSIZ * sizeof(char));

            int history = 0;
            // if we use '!!' we want to read from MRU cache
            if(!strncmp(input, "!!", 2)) history = 1;
            // ternary statement to pick correct buffer
            char** redirect = tokparse( (history ? MRU : input), cargs);
            // edge case (no command entered before history func)
            if(history && MRU[0] == '\0'){
                printf("No recently used command.\n");
                exit(0);
            } 
            // local parsing flags (redir i/o to file descriptor)
            if(!strncmp(redirect[0], "o", 1)){// output redir
                printf("output saved to ./%s\n", redirect[1]);
                int fd = open(redirect[1], O_TRUNC | O_CREAT | O_RDWR);
                dup2(fd, STDOUT_FILENO); // redirect stdout to file descriptor 
            }else if(!strncmp(redirect[0], "i", 1)){// input redir
                printf("reading from file: ./%s\n", redirect[1]);
                int fd = open(redirect[1], O_RDONLY);// read-only
                memset(input, 0, BUFSIZ * sizeof(char));
                read(fd, input,  BUFSIZ * sizeof(char));
                memset(cargs, 0, BUFSIZ * sizeof(char));
                input[strlen(input) - 1]  = '\0';
                tokparse(input, cargs);
            }
            execvp(cargs[0], cargs);
        }
    }

    return 0;
}