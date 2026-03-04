#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

    // check amount of arguments
    // one of them is ./a.out
    if (argc!=2){
        printf("Usage: %s filename\n", argv[0]);
        return 1;
    }

    // check for --help flag
    // do this before checking if the filename
    // exists, just in case a file named
    // "--help" is in the directory
    if (strcmp(argv[1], "--help") == 0){
        printf("Usage: %s filename\n", argv[0]);
        return 0;
    }

    // check if file already exists
    // stat() returns 0 on success and -1 on failure
    struct stat output_file;
    if (stat(argv[1], &output_file) == 0){
        printf("Error: %s already exists\n", argv[1]);
        return 1;
    }

    // we checked the input, let's create the file
    int fd = open(argv[1], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    // check if open() returned failure
    if (fd == -1){
        perror("OPEN");
        exit(1);
    }

    // create child process
    pid_t c;
    c = fork();

    if (c<0){
        perror("FORK");
        exit(1);
    }
    else if (c==0){
        // write to fd
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "[CHILD] getpid()=%d, getppid()=%d\n", getpid(), getppid());

        if(write(fd, buffer, strlen(buffer)) == -1){
            perror("WRITE");
            exit(1);
        }
    }
    else{
        // wait for child to finish
        wait(NULL);

        // write to fd as parent
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "[PARENT] getpid()=%d, getppid()=%d\n", getpid(), getppid());

        if(write(fd, buffer, strlen(buffer)) == -1){
            perror("WRITE");
            exit(1);
        }
        
        exit(0);
    }

    if (close(fd) == -1){
        perror("CLOSE");
        exit(1);
    }

    return 0;
}