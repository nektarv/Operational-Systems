#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>

// default IP and port
#define DEFAULT_IP "147.102.75.201"
#define DEFAULT_PORT 4217

// global variables
int sock_fd = -1;
char *ip = DEFAULT_IP;
int port = DEFAULT_PORT;
bool debug = false;

// variables to check for double argument passing
bool double_ip_arg = false;
bool double_port_arg = false;

// function declarations
void parse_args(int argc, char *argv[]);
void connect_to_server();
void run_loop();
void handle_command(char *cmd);
void cmd_get();
void cmd_permit(char *line);
void send_msg(const char *msg);
void recv_msg(char *buf, int size);

int main(int argc, char *argv[]){
    parse_args(argc, argv);
    connect_to_server();
    run_loop();
    close(sock_fd);
    return 0;
}

void parse_args(int argc, char *argv[]){
    // -------------------------------
    // parses terminal args on runtime
    // -------------------------------

    // no more than 6 args allowed
    if (argc > 6){
        printf("Usage: %s [--ip IP] [--port PORT] [--debug]\n", argv[0]);
        exit(1);
    }

    // check each argument
    for (int i=1; i<argc; i++){

        if (strcmp(argv[i], "--ip") == 0){
            // not enough arguements or wrong order ip
            if (i + 2 > argc || double_ip_arg == true){
                printf("Usage: %s [--ip IP] [--port PORT] [--debug]\n", argv[0]);
                exit(1);
            }
            // store user specified IP
            ip = argv[++i];

            // user can't input --ip twice
            double_ip_arg = true;
        }

        else if (strcmp(argv[i], "--port") == 0){
            // not enough arguements or wrong order port
            if (i + 2 > argc || double_port_arg == true){
                printf("Usage: %s [--ip IP] [--port PORT] [--debug]\n", argv[0]);
                exit(1);
            }
            // convert user specified port to int and store
            char *endptr;
            port = (int)strtol(argv[++i], &endptr, 10);
            if (*endptr != '\0'){
                printf("Error: port must be between 1 and 65535\n");
                exit(1);
            }
            if (port <= 0 || port > 65535){
                printf("Error: port must be between 1 and 65535\n");
                exit(1);
            }

            // user can't input --port twice
            double_port_arg = true;
        }

        else if (strcmp(argv[i], "--debug") == 0){
            // user gave debug argument twice
            if (debug == true){
                printf("Usage: %s [--ip IP] [--port PORT] [--debug]\n", argv[0]);
                exit(1);
            }
            debug = true;
        }

        else{
            printf("Usage: %s [--ip IP] [--port PORT] [--debug]\n", argv[0]);
            exit(1);
        }
    }
}

void connect_to_server(){
    // ---------------------------------
    // connects the client to the server
    // ---------------------------------

    printf("Connecting to %s:%d...\n", ip, port);

    // AF_INET for internet domain
    // SOCK_STREAM for connection oriented
    // 0 to let the system choose protocol
    int domain = AF_INET;
    int type = SOCK_STREAM;
    sock_fd = socket(domain, type, 0);
    if (sock_fd < 0) {
        perror("SOCKET");
        exit(1);
    }

    // sockaddr_in is just a struct that holds
    // the server's address info
    struct sockaddr_in s;
    // clean up any leftover data in the struct
    memset(&s, 0, sizeof(s));
    s.sin_family = AF_INET;
    s.sin_port = htons(port);
    // manually convert IPv4 string to bytes
    if (inet_aton(ip, &s.sin_addr) == 0){
        printf("Error: invalid IP address '%s'\n", ip);
        exit(1);
    }

    // need to cast to struct sockaddr* for connect()
    // pass our socket fd and the server's address info
    if (connect(sock_fd, (struct sockaddr *)&s, sizeof(s)) < 0){
        perror("CONNECT");
        exit(1);
    }

    printf("Connected to %s:%d\n", ip, port);
}

void run_loop(){
    // -----------------------------
    // runs main loop of the program
    // -----------------------------

    /*
    struct pollfd{
        int fd;         file descriptor to poll
        short events;   requested events
        short revents;  returned events
    }
    */

    struct pollfd fds[2];

    // stdin
    fds[0].fd = 0; // 0 is the file descriptor for stdin
    fds[0].events = POLLIN; // POLLIN means "there is data to read"

    // socket (server)
    fds[1].fd = sock_fd;
    fds[1].events = POLLIN;

    // buffer to hold user input
    char input[1024];

    while (1){
        printf("> ");
        fflush(stdout);

        // poll(file descriptors, number of fds, timeout in ms)
        // -1 for timeout means wait forever until an event happens on one of the fds
        // it returns the number of file descriptors that have events
        int ret = poll(fds, 2, -1);
        if (ret < 0){
            perror("POLL");
            break;
        }

        // server sent something
        if (fds[1].revents & POLLIN){
            char buf[1024];
            recv_msg(buf, sizeof(buf));
            printf("[SERVER] %s\n", buf);
        }

        // user typed something
        if (fds[0].revents & POLLIN){
            // fgets returns NULL if there was an error or EOF
            if (fgets(input, sizeof(input), stdin) == NULL) break;

            // remove newline and replace with null terminator
            input[strcspn(input, "\n")] = '\0';

            // user pressed enter without typing anything
            if (strlen(input) == 0) continue;

            // pass processed input to command handler function
            handle_command(input);
        }
    }
}

void handle_command(char *input){
    // --------------------------------
    // handles user commands from stdin
    // --------------------------------

    if (strcmp(input, "help") == 0){
        printf("Help message:\n");
        printf("    'help' prints this message\n");
        printf("    'exit' to exit the program\n");
        printf("    'get'  to get latest sensor data from server\n");
        printf("    'N name surname reason' to request movement permit\n");
    }

    else if (strcmp(input, "exit") == 0){
        printf("Exiting\n");
        close(sock_fd);
        exit(0);
    }
    
    else if (strcmp(input, "get") == 0){
        cmd_get();
    }

    else{
        // check if first token is a number (movement permit command)
        int N;
        // if sscanf returns a number, then we have a valid permit command
        if (sscanf(input, "%d", &N) == 1){
            cmd_permit(input);
        }
        else{
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }
}

void cmd_get(){
    // ------------------------------------------------------------
    // sends "get" command to the server and processes the response
    // ------------------------------------------------------------

    // send "get"
    send_msg("get");
    // receive response
    char buf[1024];
    recv_msg(buf, sizeof(buf));

    // parse X YYY ZZZZ WWWWWWWWWW
    // X is event type
    // YYY is brightness
    // ZZZZ is temp * 100
    // WWWWWWWWWW is unix timestamp
    int event_type, brightness, raw_temp;
    long unix_timestamp;

    // sscanf expects the format to have 4 values
    // it returns the number it successfully parsed
    if (sscanf(buf, "%d %d %d %ld", &event_type, &brightness, &raw_temp, &unix_timestamp) != 4){
        printf("Error: unexpected response format: '%s'\n", buf);
        return;
    }

    // convert temp
    double temperature = raw_temp / 100.0;

    // convert unix timestamp with localtime()
    time_t t = (time_t)unix_timestamp;
    struct tm *tm_info = localtime(&t);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // event type names
    const char *event_names[] = {"boot", "setup", "interval", "button", "motion"};
    // check if event type is valid
    if (event_type < 0 || event_type > 4){
        printf("Error: unknown event type %d\n", event_type);
        return;
    }
    // get event name from array
    const char *event_name = event_names[event_type];

    printf("-----------------------\n");
    printf("Latest event:\n");
    printf("%s (%d)\n", event_name, event_type);
    printf("Temperature is: %.2f\n", temperature);
    printf("Light level is: %d\n", brightness);
    printf("Timestamp is: %s\n", timestamp);
}

void cmd_permit(char *permit){
    // -------------------------------------------------------------
    // sends permit request to the server and processes the response
    // -------------------------------------------------------------

    // send permit message
    send_msg(permit);
    // receieve verification code
    char code[1024];
    recv_msg(code, sizeof(code));

    // check if server responed 'try again'
    if (strcmp(code, "try again") == 0){
        if (!debug) printf("Server rejected request\n");
        return;
    }

    // if we reached this point, we have a verification code
    printf("Send verification code: '%s'\n", code);

    // read the code from the user
    char user_input[1024];
    if (fgets(user_input, sizeof(user_input), stdin) == NULL) return;
    user_input[strcspn(user_input, "\n")] = '\0';

    // user wants to exit mid-permit
    if (strcmp(user_input, "exit") == 0){
        printf("Exiting\n");
        close(sock_fd);
        exit(0);
    }

    // send the code back to the server
    send_msg(user_input);
    // server responds with ACK
    char response[1024];
    recv_msg(response, sizeof(response));
    printf("Response: '%s'\n", response);
}

void send_msg(const char *msg){
    // ---------------------------
    // sends message to the server
    // ---------------------------

    // print what we send if debug mode is on
    if (debug) printf("[DEBUG] sent '%s'\n", msg);

    char buf[1024];
    // using snprintf to format the message with a newline
    snprintf(buf, sizeof(buf), "%s\n", msg);
    // send message to the server
    if (write(sock_fd, buf, strlen(buf)) < 0){
        perror("WRITE");
        exit(1);
    }
}

void recv_msg(char *buf, int size){
    // --------------------------------
    // receives message from the server
    // --------------------------------

    // read message from the server into buf
    // size-1 to leave room for null terminator
    int n = read(sock_fd, buf, size-1);
    if (n < 0){
        perror("READ");
        exit(1);
    }
    buf[n] = '\0';

    // remove newline and replace with null terminator
    buf[strcspn(buf, "\n")] = '\0';

    // print what we read if debug mode is on
    if (debug) printf("[DEBUG] read '%s'\n", buf);
}