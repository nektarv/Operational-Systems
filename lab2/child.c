#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>

static int child;
static int val = 0;
static time_t start_time;

void sig_handler(int sig){
    char buf[128];
    int len;

    if(sig == SIGUSR1){
        val++;
        len = snprintf(buf, sizeof(buf), "Child %d:    SIGUSR1 received    New val: %d\n", child, val);
        write(STDOUT_FILENO, buf, len);
    }
    else if(sig == SIGUSR2){
        val--;
        len = snprintf(buf, sizeof(buf), "Child %d:    SIGUSR2 received    New val: %d\n", child, val);
        write(STDOUT_FILENO, buf, len);
    }
    else if(sig == SIGTERM){
        len = snprintf(buf, sizeof(buf), "Child %d:    SIGTERM received    Exiting\n", child);
        write(STDOUT_FILENO, buf, len);
        exit(0);
    }
    else if(sig == SIGALRM){
        alarm(10);
        time_t total_time = time(NULL) - start_time;
        len = snprintf(buf, sizeof(buf), "Child %d:    Total time: %lds    Current val: %d\n", child, total_time, val);
        write(STDOUT_FILENO, buf, len);
    }
}

int main(int argc, char *argv[]){
    // stdout normally buffers output
    // setting to NULL so text is printed immediately
    setbuf(stdout, NULL);

    start_time = time(NULL);

    child = atoi(argv[1]);
    printf("Child %d ID: %d\n", child, getpid());

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
    sigaction(SIGALRM, &action, NULL);

    // initial alarm to enter the loop inside sig_handler()
    alarm(10);
    
    while(1){
        pause();
    }
}