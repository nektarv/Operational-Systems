#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>

static int n_children;
pid_t children_pids[1024];

void healthcheck(int argc, char *argv[]);
void sig_handler(int sig);
void create_child(int index);
void print_tree(void);

int main(int argc, char *argv[]){
    // stdout normally buffers output
    // setting to NULL so text is printed immediately
    setbuf(stdout, NULL);

    // validate command line args
    healthcheck(argc, argv);
    
    // declare sigaction config
    struct sigaction action;
    // we call sig_handler() when a signal is triggered
    action.sa_handler = sig_handler;
    // block no signals while the handler is running
    sigemptyset(&action.sa_mask);
    // if a sys call gets interrupted by a signal
    // then resume the sys call
    action.sa_flags = SA_RESTART;
    // apply config
    sigaction(SIGUSR1, &action, NULL);
    sigaction(SIGUSR2, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGCHLD, &action, NULL);

    // create n cildren
    for (int i=0; i<n_children; i++) create_child(i);
    // print process id tree
    print_tree();

    while(1){
        pause();
    }
}

void healthcheck(int argc, char *argv[]){
    if (argc!=2){
        printf("Usage: 2 arguments (filename and amount of children)\n");
        exit(1);
    }

    n_children = atoi(argv[1]);
    if (n_children<=0){
        printf("Amount of children must be a positive integer\n");
        exit(1);
    }
}

void sig_handler(int sig){
    char buf[128];
    int len;

    if(sig == SIGUSR1){
        for (int i=0; i<n_children; i++){
            kill(children_pids[i], SIGUSR1);
        }
    }
    else if(sig == SIGUSR2){
        for (int i=0; i<n_children; i++){
            kill(children_pids[i], SIGUSR2);
        }
    }
    else if(sig == SIGTERM){
        for (int i=0; i<n_children; i++){
            kill(children_pids[i], SIGTERM);
        }
        exit(0);
    }
    else if(sig == SIGCHLD){
        // when i SIGTERM or SIGSTOP a child
        // the OS sends SIGCHLD on its own
        int status;
        pid_t pid;

        // WNOHANG returns immediately instead of waiting if no child has an issue
        // WUNTRACED is for warnings about children that have been stopped
        // waitpid() returns the PID of the child that changed state
        // we have a while loop in case muliple children change states simultaneously
        while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){
            // if child stopped
            if (WIFSTOPPED(status)){
                // find which one
                for (int i=0; i<n_children; i++){
                    if (pid == children_pids[i]){
                        kill(pid, SIGCONT);
                        len = snprintf(buf, sizeof(buf), "Child %d (ID %d) had stopped, parent resumed it\n", i+1, children_pids[i]);
                        write(STDOUT_FILENO, buf, len);
                    }
                }
            }
            // if child died
            else if (WIFEXITED(status) || WIFSIGNALED(status)){
                // find which one
                for (int i=0; i<n_children; i++){
                    if (pid == children_pids[i]){
                        len = snprintf(buf, sizeof(buf), "Child %d died, creating a new one\n", i+1);
                        write(STDOUT_FILENO, buf, len);
                        create_child(i);
                    }
                }
            }
        }
    }
}

void create_child(int index){
    pid_t child;
    child = fork();

    if (child<0){
        perror("FORK");
        exit(1);
    }

    if (child==0){
        char child_index[16];
        snprintf(child_index, sizeof(child_index), "%d", index+1);
        char *child_argv[] = {"./child.exe", child_index, NULL};
        execv("./child.exe", child_argv);
        perror("execv");
        exit(1);
    }

    if (child>0){
        children_pids[index] = child;
    }
}

void print_tree(void){
    printf("\nProcess Tree\n");
    printf("    Parent : %d\n", getpid());
    for (int i=0; i<n_children; i++){
        printf("        Child %d : %d\n", i+1, children_pids[i]);
    }
    printf("\n\n");
}