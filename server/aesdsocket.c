#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <libgen.h>


#define NSTRS                   4
#define MAX_IT                  1
#define LOCAL_PORT              54321
#define DEFAULT_PORT            9000
#define SIZE_OF_LOCAL_BUFFER 256
//#define SYSTEM_TCP_IP_ADDRESS   "10.0.10.30"
//#define SYSTEM_TCP_IP_ADDRESS   "localhost"
//#define SYSTEM_TCP_IP_ADDRESS   "127.0.0.1"
#define SYSTEM_TCP_IP_ADDRESS   "10.0.2.15"
#define FILE_WRITE_TIMEOUT      10000000
#define FILE_TO_WRITE_TO        "/var/tmp/aesdsocketdata"
//#define FILE_TO_WRITE_TO        "tmp/tmp/aesdsocketdata"
//#define LOCAL_PORT 3
#define ADD_BUSYBOX_IP "ip address add 10.0.10.90/24 brd 10.0.10.255 dev eth0"

char *test_strs[NSTRS] = {
    "This is the first server string.\n",
    "This is the second server string.\n",
    "This is the third server string.\n",
    "Server sends: \"This has been the an assignment of ECEA 5305 Coursera "
        "edition.\"\n"
};

#define __LOCAL_SUCCESS__ 0
#define __LOCAL_FAIL__ 1

extern int errno;
extern void broken_pipe_handler();
extern void terminate_program_handler();
extern void external_interrupt_handler();
extern void serve_clients_FGREEN();

static int server_sock, client_sock;
static FILE *fp;
static struct sockaddr_in server_sockaddr, client_sockaddr;


// The function is to turn the process of calling the function into a daemon.
void create_daemon(void)
{
    pid_t pid = 0;

    pid = fork();

    if (pid < 0)
    {
        perror("Program 'aesdsocket' FAILED to fork.");
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Program 'aesdsocket'"
            " FAILED to fork.");

        exit(-1);
    }

    if (pid > 0)
    {
        syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Program 'aesdsocket'"
            " SUCCESSFULLY forked.  The parent program exited. The dameon"
            " PID is %d", pid);

        exit(0);
    }

}

void external_process_daemon_kill_function(void)
{
    // Always assume everything fails and check for success.
    FILE *file_descriptor = NULL;

    int read_file_position_is = 0;

    system("touch /tmp/aesdsocketKillMe.txt");

    // Pointer to the working file, open it, create if necessary, and append.
    file_descriptor = fopen ("/tmp/aesdsocketKillMe.txt", "a+");

    if(file_descriptor == NULL)
    {
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Student's error "
                "message: ERROR: Opening file failed to open or create file.");
    }
    else
    {
        //fseek(file_descriptor, 0L, SEEK_SET);
        fseek(file_descriptor, 0L, SEEK_END);

        read_file_position_is = ftell(file_descriptor);

        fseek(file_descriptor, 0L, SEEK_SET);

        char *what_to_read;

        what_to_read = (char*)malloc(read_file_position_is + 1);

        if (what_to_read == NULL)
        {
            printf("Error reallocating space for 'what_to_read'");
        }

        memset(what_to_read, 0, sizeof(read_file_position_is + 1));


        fread(what_to_read, read_file_position_is, 1, file_descriptor);

        if(strcmp(what_to_read, "true") == 0)
        {
            fflush(file_descriptor);
            fclose(file_descriptor);

            terminate_program_handler();

        }

        free(what_to_read);

        if(file_descriptor != NULL)
        {
            fflush(file_descriptor);
            fclose(file_descriptor);
        }
    }
}


void remove_temporary_file()
{
    int check = 0;

    size_t message_alloc = 256;
    char system_message [message_alloc];

    memset(system_message, 0, sizeof(system_message));

    sprintf(system_message, "rm -fr %s", FILE_TO_WRITE_TO);
    printf("system_message = %s\n", system_message);
    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: removing temporary "
            "file with command '$ %s'", system_message);
    check = system(system_message);

    // check if directory is created or not
    if (!check)
    {
        printf("Directory deleted\n");
    }
    else
    {
        printf("Unable to delete directory\n");
    }
}


int create_temporary_file(char * where)
{
    int status = __LOCAL_FAIL__;

    char *where_to_write = where;

    int check = 0;

    size_t message_alloc = 256;
    char system_message [message_alloc];
    char where_to_write_was[message_alloc];


    memset(system_message, 0, sizeof(system_message));
    sprintf(where_to_write_was, "%s", where_to_write);

    //char *path = "./tmp/tmp/asdf";

    //char *path = reallocarray(where, sizeof(char),

    char *parent_directory = dirname(where_to_write);
    //char *parent_directory = dirname(path);
    printf("parent_directory = %s\n", parent_directory);

    sprintf(system_message, "mkdir -vp %s", parent_directory);
    printf("system_message = %s\n", system_message);

    check = system(system_message);

    // check if directory is created or not
    if (!check)
    {
        printf("Directory created\n");
        status = __LOCAL_SUCCESS__;
    }
    else
    {
        printf("Unable to create directory\n");
        status = __LOCAL_FAIL__;
    }

    struct stat st = {0};

    if (stat(parent_directory, &st) == 0)
    {
        printf ("Parent directory exists: %s\n", parent_directory);
        status = __LOCAL_SUCCESS__;
    }
    else
    {
        printf ("SYSTEM FAIL: Parent directory does NOT exist: %s\n", parent_directory);
        status = __LOCAL_FAIL__;
    }



    return status;
}




/* Listen and accept loop function */
void serve_clients_FGREEN()
{
    char myIpv4[INET_ADDRSTRLEN]; // space to hold IPV4 Address.
    char myIpv6[INET6_ADDRSTRLEN]; // space to hold IPv6  Address.
    struct sockaddr_in experiment;
    struct sockaddr_storage client_address_storage;
    socklen_t client_address_length;
    client_address_length = sizeof(client_address_storage);
    int port = 0;
    char hostname[64];

    char path_to_write [sizeof(FILE_TO_WRITE_TO)];
    unsigned long num_bytes_read = 0;
    unsigned long num_bytes_written = 0;

    // Always assume everything fails and check for success.
    FILE *file_descriptor = NULL;

    static char system_input_character;
    static socklen_t fromlen;

    char debug_array[2048];
    memset(debug_array, 0, sizeof(debug_array));

    int breaker = FILE_WRITE_TIMEOUT;

    unsigned long write_count_counter = 0;
    int read_file_position_is = 0;

    memset(path_to_write, 0, sizeof(path_to_write));
    memset(hostname, 0, sizeof(hostname));
    memset(myIpv4, 0, sizeof(myIpv4));
    memset(myIpv6, 0, sizeof(myIpv6));


    gethostname(hostname, sizeof(hostname));
    inet_pton(AF_INET, SYSTEM_TCP_IP_ADDRESS, &(experiment.sin_addr));
    inet_ntop(AF_INET, &(experiment.sin_addr), myIpv4, INET_ADDRSTRLEN);
    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN - myIpv4 %s", myIpv4);

    sprintf(path_to_write, "%s", FILE_TO_WRITE_TO);
    create_temporary_file(path_to_write);

    for(;;)
    {

        /* Listen on the socket */
        if(listen(server_sock, 5) < 0)
        {
            perror("Server: listen");
            syslog(LOG_INFO | LOG_ERR, "DEBUG CODE - FGREEN: Server Socket "
                    "'listen()' failed.");
            exit(-1);
        }

        /* Accept connections */
        if((client_sock=accept(server_sock,
                        (struct sockaddr *)&client_sockaddr,
                        &fromlen)) < 0)
        {
            perror("Server: accept");
            syslog(LOG_INFO | LOG_ERR, "DEBUG CODE - FGREEN: Server Socket "
                    "'accept()' failed.");
            exit(-1);
        }
        else
        {
            getpeername(server_sock,
                    (struct sockaddr *)&client_address_storage,
                    &client_address_length);

            // Convert system IP address to a string
            // Deal with both IPv4 and IPv6
            if (client_address_storage.ss_family == AF_INET)
            {
                struct sockaddr_in *s = (struct sockaddr_in *)
                    &client_address_storage;

                port = ntohs(s->sin_port);

                inet_ntop(AF_INET, &(s->sin_addr), myIpv4,
                        INET_ADDRSTRLEN);

                syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server "
                        "Socket 'accept()' accepted from IPv4 address: %s, "
                        "port: %d", myIpv4, port);
                syslog(LOG_INFO | LOG_INFO, "Accepted connection from %s"
                        , myIpv4);
            }
            else
            {
                struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_address_storage;
                port = ntohs(s->sin6_port);

                inet_ntop(AF_INET6, &(s->sin6_addr), myIpv6,
                        INET6_ADDRSTRLEN);

                syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server "
                        "Socket 'accept()' accepted from IPv6 address: %s, "
                        "port: %d", myIpv6, port);
                syslog(LOG_INFO | LOG_INFO, "Accepted connection from %s",
                        myIpv6);
            }
        }


        /////////////////////////////////////////////////////
        //         Write file
        ////////////////////////////////////////////////////


        // Pointer to the working file, open it, create if necessary, and append.
        file_descriptor = fopen (FILE_TO_WRITE_TO, "a+");

        if(file_descriptor == NULL)
        {
            printf("DEBUG CODE - FGREEN: Student's error message: Failed to open "
                    "file.\n");
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Student's error "
                    "message: ERROR: Write failed to open or create file.");
        }
        else
        {

            write_count_counter = 0;

            fp = fdopen(client_sock, "r");

            // Place File Descriptor at the end of the file to append
            fseek(file_descriptor, 0L, SEEK_END);

            while((system_input_character = fgetc(fp)) != EOF)
            {
                putchar(system_input_character);

                write_count_counter = (unsigned long) fwrite (&system_input_character, 1,
                        strlen(&system_input_character), file_descriptor);

                num_bytes_written += write_count_counter;

                if(system_input_character == '\n')
                {
                    printf("BREAKING '/\n'\n");

                    break;
                }

                if(breaker < 1)
                {
                    //breaker = 10000;
                    printf("BREAKING from breaker\n");
                    breaker = FILE_WRITE_TIMEOUT;
                    break;
                }

                breaker--;

            } /* end while */
#if 0
            syslog(LOG_USER | LOG_DEBUG, "DEBUG CODE - FGREEN: Writing (%s) "
                    "to (%s); num_bytes_written = %lu", what, where,
                    (unsigned long) *num_bytes_written);
#endif
        }


        /////////////////////////////////////////////////////
        //         Read file
        ////////////////////////////////////////////////////


        if(file_descriptor == NULL)
        {
            printf("DEBUG CODE - FGREEN: Student's error message: Failed to open "
                    "file.\n");
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Student's error "
                    "message: ERROR: Failed to open or create file.");
        }
        else
        {


//#if 0       // READ BUFFER OVER FLOW TEST
            fseek(file_descriptor, 0L, SEEK_END);
            read_file_position_is = ftell(file_descriptor);
            fseek(file_descriptor, 0L, SEEK_SET);
            // Total file size minus the number just written
            // will be the section needed to pas the test.


            char *what_to_read;

            what_to_read = (char*)malloc(read_file_position_is + 1);
            if (what_to_read == NULL)
            {
                printf("Error reallocating space for 'what_to_read'");
            }

            memset(what_to_read, 0, sizeof(num_bytes_written + 1));

            fread(what_to_read, num_bytes_written, 1, file_descriptor);

            send(client_sock, what_to_read, read_file_position_is, 0);


                    printf("DEBUG CODE - FGREEN: reading file: %s\n",
                            what_to_read);

                    printf("DEBUG CODE - FGREEN: reading file: %s\n",
                            what_to_read);
                    printf("DEUBG CODE - FGREEN: debug_array: %s\n",
                            debug_array);


            syslog(LOG_USER | LOG_DEBUG, "DEBUG CODE - FGREEN: Reading (%s) to (%s): returned %lu "
                    "num_bytes_read", what_to_read, FILE_TO_WRITE_TO, num_bytes_read);



            // Free pointer
            free(what_to_read);

            external_process_daemon_kill_function();
        }

        if(file_descriptor != NULL)
        {
            fflush(file_descriptor);
            fclose(file_descriptor);
            fflush(fp);
            fclose(fp);
        }


        /////////////////////////////////////////////////////
        //         Clean Up Socket
        ////////////////////////////////////////////////////


        close(client_sock);

        // Convert system IP address to a string
        // Deal with both IPv4 and IPv6
        if (client_address_storage.ss_family == AF_INET)
        {
            struct sockaddr_in *s = (struct sockaddr_in *)
                &client_address_storage;

            port = ntohs(s->sin_port);

            inet_ntop(AF_INET, &(s->sin_addr), myIpv4,
                    INET_ADDRSTRLEN);

            syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server "
                    "Socket 'close()' closed connection from IPv4 address: %s, "
                    "port: %d", myIpv4, port);
            syslog(LOG_INFO | LOG_INFO, "Closed connection from %s"
                    , myIpv4);

        }
        else
        {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_address_storage;
            port = ntohs(s->sin6_port);

            inet_ntop(AF_INET6, &(s->sin6_addr), myIpv6,
                    INET6_ADDRSTRLEN);

            syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server "
                    "Socket 'close()' closed connection from IPv6 address: %s, "
                    "port: %d", myIpv6, port);
            syslog(LOG_INFO | LOG_INFO, "Closed connection from %s",
                    myIpv6);

        }

    } /* end for ever */

}

/* Close sockets after a Ctrl-C interrupt */

void external_interrupt_handler()
{
        printf("\nsockets are being closed\n");
        syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN, Server: sockets "
                "are being closed.");
        close(client_sock);
        close(server_sock);

        remove_temporary_file();

    exit(-1);

}


void broken_pipe_handler()
{
        close(client_sock);
        close(server_sock);
        remove_temporary_file();

        exit(-1);
}

void terminate_program_handler()
{
    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: "
            "terminate_program_handler(): terminating program");

    remove_temporary_file();
    close(client_sock);
    close(server_sock);

    exit(0);
}

int main(int argc, char ** argv)
{
    int function_status = __LOCAL_FAIL__;

    if (argc >= 2 && strcmp(argv[1], "-delete_working_file") == 0)
    {
        remove_temporary_file();
    }

    char hostname[64];
    struct hostent *hp;
    struct linger opt;
    int sockarg;
    unsigned int sock_option = 1;
    unsigned int sock_length = 0;
    char *sock_option_val;

    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: setting up local "
            "socket");

    // DEBUG CODE BELOW - FGREEN
    char myIpv4[INET_ADDRSTRLEN]; // space to hold my designated IP Address.
    char myIpv6[INET6_ADDRSTRLEN]; // space to hold IPv6  Address.
    struct sockaddr_in experiment;

    int port = 0;

    memset(hostname, 0, sizeof(hostname));
    memset(myIpv4, 0, sizeof(myIpv4));
    memset(myIpv6, 0, sizeof(myIpv6));


    inet_pton(AF_INET, SYSTEM_TCP_IP_ADDRESS, &(experiment.sin_addr));
    inet_ntop(AF_INET, &(experiment.sin_addr), myIpv4, INET_ADDRSTRLEN);

    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: myIpv4 %s", myIpv4);

    gethostname(hostname, sizeof(hostname));

    if((hp = (struct hostent*) gethostbyname(hostname)) == NULL)
    {
        size_t message_alloc = 256;
        char system_message [message_alloc];

        memset(system_message, 0, sizeof(system_message));

        sprintf(system_message, "%s", ADD_BUSYBOX_IP );
        printf("system_message = %s\n", system_message);
        syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: BusyBox does not have an"
                "IP address. Adding: '$ %s'", system_message);


        if((hp = (struct hostent*) gethostbyname("localhost")) == NULL)
        {
            fprintf(stderr, "Error: %s host unknown.\n", hp->h_name);
            exit(-1);
        }
        {
            printf("DEBUG CODE - FGREEN: successfully retrieved host name: "
                    "%s", hp->h_name);
        }
    }

    if((server_sock=socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Server: socket");
        exit(-1);
    }

    bzero((char*) &server_sockaddr, sizeof(server_sockaddr));
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(DEFAULT_PORT);
    bcopy (hp->h_addr, &server_sockaddr.sin_addr, hp->h_length);

    //inet_pton(AF_INET, "10.0.10.10", &(server_sockaddr.sin_addr));
    inet_pton(AF_INET, SYSTEM_TCP_IP_ADDRESS, &(server_sockaddr.sin_addr));

    if(getsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &sock_option, &sock_length) < 0)
    {
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: getsockopt() failed."
                " sock_option = %d, sock_length = %d", sock_option, sock_length);
    }
    else
    {
        syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: getsockopt() success"
                ".  sock_option = %d, sock_length = %d", sock_option, sock_length);
    }


    sock_option_val = "localhost";
    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, sock_option_val, 4) < 0)
    {
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: setsockopt() failed."
                " sock_option = %d, sock_option_val = %c", sock_option,
                *sock_option_val);
        exit(-1);
    }
    else
    {
        syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: setsockopt() succes"
                " sock_option = %d, sock_option_val = %s", sock_option,
                sock_option_val);
    }


    /* Bind address to the socket */
    if(bind(server_sock, (struct sockaddr *) &server_sockaddr,
                sizeof(server_sockaddr)) < 0)
    {
        perror("Server: bind");
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: bind() failed.");
        exit(-1);
    }
    else
    {
        if (argc >= 2 && strcmp(argv[1], "-d") == 0)
        {
            syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: User passed in "
                    " the create dameon argurment '-d'.  "
                    "Entering 'create_daemon ()");

            create_daemon();
        }
    }

    /* turn on zero linger time so that undelivered data is discarded when
       socket is closed
       */
    opt.l_onoff = 1;
    opt.l_linger = 0;

    sockarg = 1;

    setsockopt(server_sock, SOL_SOCKET, SO_LINGER, (char*) &opt, sizeof(opt));
    setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&sockarg,
            sizeof(int));
    signal(SIGINT, external_interrupt_handler);
    signal(SIGPIPE, broken_pipe_handler);
    signal(SIGTERM, terminate_program_handler);

    port = ntohs(server_sockaddr.sin_port);

    inet_ntop(AF_INET, &(server_sockaddr.sin_addr), myIpv4,
            INET_ADDRSTRLEN);

    syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server Socket IPv4 is "
            "to address: %s, port: %d", myIpv4, port);


    serve_clients_FGREEN();

    return function_status;
}

