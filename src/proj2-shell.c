#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void tokparse(char* input, char* cargs[]){
    const char delim[2] = {' ', '\t'};// delimiters (space and tab)
    char* token;
    int num_args = 0;

    token = strtok(input, delim);
    cargs[0] = token;

    while(token != NULL){
        // add token to cargs
        printf("token %d: %s\n", num_args, token);
        token = strtok(NULL, delim);
        cargs[++num_args] = token;
    }
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
            wait(NULL);
            printf("child completed...\n");
        }
        else{          // child
            printf("child process start...\n");
            char* cargs[BUFSIZ];    // buffer to hold command line arguments
            memset(cargs, 0, BUFSIZ * sizeof(char));

            // if we use '!!' we want to read from MRU cache
            if(strncmp(input, "!!", 2) == 0)
                tokparse(MRU, cargs);
            else
                tokparse(input, cargs); 
            execvp(cargs[0], cargs);
        }

    }

    return 0;
}