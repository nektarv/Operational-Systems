#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>

// number of children
static int n_children;
// mode = 1 for round robin
// mode = 2 for random
static int mode;
// for round robin mode
static int next_child = 0;

// array to store IDs of children
pid_t *children_pids;
// arrays for file descriptors
int (*p2c)[2];
int (*c2p)[2];

void parse_args(int argc, char *argv[]);
void child(int index);
void define_fd_set(fd_set *descriptors);

int main(int argc, char *argv[]){
    // stdout normally buffers output
    // setting to NULL so text is printed immediately
    setbuf(stdout, NULL);

    // validate args
    parse_args(argc, argv);

    // allocate memory to arrays
    children_pids = malloc(n_children * sizeof(pid_t));
    p2c = malloc(n_children * sizeof(int[2]));
    c2p = malloc(n_children * sizeof(int[2]));
    
    // create n child processes
    for (int i=0; i<n_children; i++){
        child(i);
        printf("Child %d was created\n", i+1);
    }
    printf("\n");

    // close pipe ends the parent doesn't need AFTER forking
    // if we do it inside the child() function, all p2c
    // connections use the same file descriptor
    for (int i=0; i<n_children; i++){
        if (close(p2c[i][0]) < 0){
            perror("CLOSE");
            exit(1);
        }
        if (close(c2p[i][1]) < 0){
            perror("CLOSE");
            exit(1);
        }
    }

    // find biggest file descriptor for select()
    int highest_fd = 0;
    for (int i=0; i<n_children; i++){
        if (c2p[i][0] > highest_fd){
            highest_fd = c2p[i][0];
        }
    }
    // select() needs highest+1 because it scans
    // from 0 to highest_fd-1
    highest_fd++;

    // without the following, rand() always produces
    // the same sequence of selection between runs
    srand(time(NULL));

    // file descriptor set of the ones the parent reads
    fd_set descriptors_to_read;
    while(1){
        // reset file descriptor set
        define_fd_set(&descriptors_to_read);
        
        // run select()
        // syntax : (highest_fd, read set, write set, error set, timeout)
        // we only care about the read set and timeout means
        // "block forever until something is ready"
        select(highest_fd, &descriptors_to_read, NULL, NULL, NULL);
        
        // check if the user typed something in std input
        if (FD_ISSET(0, &descriptors_to_read)){
            char buffer[256];
            fgets(buffer, sizeof(buffer), stdin);

            if (strcmp(buffer, "help\n") == 0){
                printf("Type a number to send job to a child!\n");
            }
            else if (strcmp(buffer, "exit\n") == 0){
                // terminate all child processes
                for (int i=0; i<n_children; i++){
                    kill(children_pids[i], SIGTERM);
                }

                // put the parent to sleep till they terminate
                for (int i=0; i<n_children; i++){
                    waitpid(children_pids[i], NULL, 0);
                }

                // free memory
                free(children_pids);
                free(p2c);
                free(c2p);

                printf("Exiting\n");
                exit(0);
            }
            else{
                // strtol parses a string to a number (we use 10 for decimal)
                // it sets endptr at the first character it couldn't parse
                char *endptr;
                int job = (int)strtol(buffer, &endptr, 10);
                if (endptr == buffer || *endptr !='\n'){
                    // ==buffer means nothing was parsed
                    // !=\n means leftover non numeric vals
                    printf("Type a number to send job to a child!\n");
                }
                else{
                    // round robin mode path
                    if (mode == 1){
                        printf("[Parent] Assigned %d to child %d\n", job, next_child+1);

                        if (write(p2c[next_child][1], &job, sizeof(int)) < 0){
                            perror("WRITE");
                            exit(1);
                        }

                        if (++next_child == n_children){
                            next_child = 0;
                        }
                    }
                    // random mode path
                    else{
                        // rand() picks a random number between 0 and n_children-1
                        int random_child = rand()%n_children;
                        printf("[Parent] Assigned %d to child %d\n", job, random_child+1);

                        if (write(p2c[random_child][1], &job, sizeof(int)) < 0){
                            perror("WRITE");
                            exit(1);
                        }
                    }
                }
            }
        }

        for (int i=0; i<n_children; i++){
            if (FD_ISSET(c2p[i][0], &descriptors_to_read)){
                // a child process returned a number from a job
                int result;
                if (read(c2p[i][0], &result, sizeof(int)) < 0){
                    perror("READ");
                    exit(1);
                }
                printf("[Parent] Child %d returned %d\n", i+1, result);
            }
        }
    }
}

void parse_args(int argc, char *argv[]){
    // -------------------------------
    // parses terminal args on runtime
    // -------------------------------
    
    // make sure args are valid count-wise
    if (argc<2 || argc>3){
        printf("Usage: %s <nChildren> [--random] [--round-robin]\n", argv[0]);
        exit(1);
    }

    // validate number of children
    // strtol parses a string to a number (we use 10 for decimal)
    // it sets endptr at the first character it couldn't parse
    char *endptr;
    n_children = (int)strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr !='\0' || n_children <= 0){
        // ==argv[1] means nothing was parsed
        // !=\0 means leftover non numeric vals
        printf("Usage: %s <nChildren> [--random] [--round-robin]\n", argv[0]);
        exit(1);
    }

    // if mode wasn't specified, choose round robin and return
    if (argc == 2){
        mode = 1;
        return;
    }

    // we have 3 args total, check the chosen mode
    if (strcmp(argv[2], "--round-robin") == 0){
        mode = 1;
        return;
    } else if(strcmp(argv[2], "--random") == 0){
        mode = 2;
        return;
    } else{
        printf("Usage: %s <nChildren> [--random] [--round-robin]\n", argv[0]);
        exit(1);
    }
}

void child(int index){
    // ----------------------------------------------
    // create a new child process and handle its work
    // ----------------------------------------------

    // create pipes
    if (pipe(p2c[index]) < 0){
        perror("PIPE");
        exit(1);
    }
    if (pipe(c2p[index]) < 0){
        perror("PIPE");
        exit(1);
    }

    // fork
    pid_t child;
    child = fork();

    if (child<0){
        perror("FORK");
        exit(1);
    }

    if (child==0){
        // close unused pipe ends
        for (int i=0; i<index; i++){
            if (close(p2c[i][0]) < 0){
                perror("CLOSE");
                exit(1);
            }
            if (close(p2c[i][1]) < 0){
                perror("CLOSE");
                exit(1);
            }
            if (close(c2p[i][0]) < 0){
                perror("CLOSE");
                exit(1);
            }
            if (close(c2p[i][1]) < 0){
                perror("CLOSE");
                exit(1);
            }                        
        }
        if (close(c2p[index][0]) < 0){
            perror("CLOSE");
            exit(1);
        }
        if (close(p2c[index][1]) < 0){
            perror("CLOSE");
            exit(1);
        }

        // number the parents sends
        int job;

        // double the number and wait 5sec before sending it back
        while(1){
            // receive number
            if (read(p2c[index][0], &job, sizeof(int)) < 0){
                perror("READ");
                exit(1);
            }

            printf("[Child %d] [PID %d]: Received %d!\n", index+1, getpid(), job);

            // double and sleep 5sec
            job *= 2;
            sleep(5);

            // send it back
            if (write(c2p[index][1], &job, sizeof(int)) < 0){
                perror("WRITE");
                exit(1);
            }

            printf("[Child %d] [PID %d]: Finished hard work, writing back %d\n", index+1, getpid(), job);
        }
    }

    if (child>0){
        // save child PID
        children_pids[index] = child; 
    }
}

void define_fd_set(fd_set *descriptors){
    // ---------------------------------------------------------
    // rebuild the set every iteration since select() changes it
    // ---------------------------------------------------------

    // empty
    FD_ZERO(descriptors);

    // add std input
    FD_SET(0, descriptors);

    // add read ends from children
    for (int i=0; i<n_children; i++){
        FD_SET(c2p[i][0], descriptors);
    }

    return;
}