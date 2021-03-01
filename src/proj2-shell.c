/*  
    Justin Stitt
    2/24/2021 - 2/28/2021

    UNIX Shell Project

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

#define READ_END 0 
#define WRITE_END 1

int find_pipe_rhs(char** cargs);

char** tokparse(char* input, char* cargs[]){
    const char delim[2] = {' ', '\t'};// delimiters (space and tab)
    char* token;
    int num_args = 0;

    token = strtok(input, delim);
    cargs[0] = token;

    char** REDIR = malloc(2 * sizeof(char*));
    for(int x = 0; x < 2; ++x)
        REDIR[x] = malloc(BUFSIZ * sizeof(char));
    REDIR[0] = "";// i/o/p
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
        else if(!strncmp(token, "|", 1)){// pipe
            REDIR[0] = "p";        
        }
        cargs[++num_args] = token;
    }
    return REDIR;
}

int main(int argc, const char* argv[]){
    char input [BUFSIZ];
    char MRU   [BUFSIZ]; // most-recently used cache
    int pipefd[2];// file descriptor

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
            wait(NULL);
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
            }else if(!strncmp(redirect[0], "p", 1)){// found a pipe
                pid_t pidc;// hierachical child
                int pipe_rhs_offset = find_pipe_rhs(cargs);
                cargs[pipe_rhs_offset] = "\0";
                int e = pipe(pipefd);
                if(e < 0){
                    fprintf(stderr, "pipe creation failed...\n");
                    return 1;
                }

                char* lhs[BUFSIZ], *rhs[BUFSIZ];
                memset(lhs, 0, BUFSIZ*sizeof(char));
                memset(rhs, 0, BUFSIZ*sizeof(char));
                for(int x = 0; x < pipe_rhs_offset; ++x){
                    lhs[x] = cargs[x];
                }
                for(int x = 0; x < BUFSIZ; ++x){
                    int idx = x + pipe_rhs_offset + 1;
                    if(cargs[idx] == 0) break;
                    rhs[x] = cargs[idx];
                }
                pidc = fork();// create child to handle pipe's rhs
                if(pidc < 0){
                    fprintf(stderr, "fork failed...\n");
                    return 1;
                }
                if(pidc != 0){// parent process 
                    dup2(pipefd[WRITE_END], STDOUT_FILENO);// duplicate stdout to write end of file descriptor
                    close(pipefd[WRITE_END]);
                    execvp(lhs[0], lhs);
                    exit(0); 
                }else{// child process
                    dup2(pipefd[READ_END], STDIN_FILENO);
                    close(pipefd[READ_END]);
                    execvp(rhs[0], rhs);
                    exit(0); 
                }
                wait(NULL);
            }
            execvp(cargs[0], cargs);
            exit(0);  
        }
    }

    return 0;
}

int find_pipe_rhs(char** cargs){
    int idx = 0;
    while(cargs[idx] != '\0'){
        if(!strncmp(cargs[idx], "|", 1)){// found pipe
            return idx;// new cargs starting at offset
        }
        ++idx;
    }
    return -1;// superfluous (we know there is a pipe)
}