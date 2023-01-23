#include <stdlib.h>
#include <stdio.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
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
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

#define UNIVERSITY_OF_COLORADO_BOULDER_COURSERA "ECEA-5306 Assignment 6"

#define NSTRS                   4
#define MAX_IT                  1
#define LOCAL_PORT              54321
#define DEFAULT_PORT            9000
//#define SIZE_OF_LOCAL_BUFFER 256
#define SIZE_OF_LOCAL_BUFFER    1024
//#define SYSTEM_TCP_IP_ADDRESS   "10.0.10.30"
//#define SYSTEM_TCP_IP_ADDRESS   "localhost" // SCHOOL DEFINED
#define SYSTEM_TCP_IP_ADDRESS   "127.0.0.1" //STUDENT EQUIPMENT also required for test assignment to work
//USE THIS ONE#define SYSTEM_TCP_IP_ADDRESS   "10.0.2.15"
//#define SYSTEM_SOCK_OPTION_VAL  "localhost"; // SCHOOL DEFINED
#define SYSTEM_SOCK_OPTION_VAL  "eth0"; // STUDENT EQUIPMENT also required for test assignment to work
//#define SYSTEM_SOCK_OPTION_VAL  "eno1"; // STUDENT EQUIPMENT
#define FILE_WRITE_TIMEOUT      10000000
#define FILE_TO_WRITE_TO        "/var/tmp/aesdsocketdata" //SCHOOL DEFINED
//#define FILE_TO_WRITE_TO        "/tmp/tmp/aesdsocketdata"
//#define LOCAL_PORT 3
#define ADD_BUSYBOX_IP "ip address add 10.0.10.90/24 brd 10.0.10.255 dev eth0"
#define BACKLOG 5

char *test_strs[NSTRS] = {
    "This is the first server string.\n",
    "This is the second server string.\n",
    "This is the third server string.\n",
    "Server sends: \"This has been the an assignment of ECEA 5306 Coursera "
        "edition.\"\n"
};

#define __LOCAL_SUCCESS__ 0
#define __LOCAL_FAIL__ 1

extern int errno;
extern void broken_pipe_handler();
extern void terminate_program_handler();
extern void alarm_program_handler();
extern void external_interrupt_handler();
extern void serve_clients_FGREEN();

pthread_mutex_t sharedMemMutexSemaphore;
pthread_mutexattr_t rt_safet;

static int server_sock, client_sock; //, new_sockfd;
static FILE *fp;
static struct sockaddr_in server_sockaddr, client_sockaddr;
bool interrupted = false;

struct addrinfo *server_info = NULL;
int total_connections = 0;
bool alarm_timer = false;

// Data thread structure
struct data_thread
{
    pthread_t thread_id;
    int counter;
    int socketfd;
    struct sockaddr_in * client;
    bool connection_completion_success;
    struct sockaddr_storage client_address_storage;
    SLIST_ENTRY(data_thread) entries; 
};

SLIST_HEAD(slisthead, data_thread) head=SLIST_HEAD_INITIALIZER(&head);


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

// Set timer 
int set_timer(void)
{
    int time_value = 0;
    
    struct itimerval timer_0;
    getitimer(ITIMER_REAL, &timer_0);
    timer_0.it_value.tv_sec = 10;
    timer_0.it_value.tv_usec = 0;
    timer_0.it_interval.tv_sec = 10;
    timer_0.it_interval.tv_usec = 0;
    time_value = setitimer(ITIMER_REAL, &timer_0, NULL);
    
    return (time_value);
}


// Clear timer
int clear_timer(void)
{
    int time_value = 0;
    
    struct itimerval timer_0;
    getitimer(ITIMER_REAL, &timer_0);
    timer_0.it_value.tv_sec = 0;
    timer_0.it_value.tv_usec = 0;
    timer_0.it_interval.tv_sec = 0;
    timer_0.it_interval.tv_usec = 0;
    time_value = setitimer(ITIMER_REAL, &timer_0, NULL);
    
    return (time_value);
}


void remove_temporary_file(void)
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

// DEBUG CODE BELOW - FGREEN WAS NOT HERE
/* Required timer tracking */
int write_timer(FILE *file_descriptor)
{
   syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Entering write_timer()");
   char buffer [256];
   char local_string [256] = {};
   int num_bytes_written = 0;
   const char* time_format = "%a, %d %b %Y %T %z";
   time_t local_time;
   struct tm *timestamp;
   
   local_time =  time(NULL);
   
   timestamp = localtime(&local_time); 
   
   if (timestamp == NULL)
   {
       perror("DEBUG CODE - FGREEN: localtime() failed");
       syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: write_timer() FAIL ");

       return (-1);
   }  

   if(strftime(local_string, sizeof(local_string), time_format, timestamp) == 0)
   { 
       return (-1);
   }

   strncpy(buffer, "timestamp:", sizeof(buffer));
   strncat(buffer, local_string, sizeof(buffer-1));
   strncat(buffer, "\n", sizeof(buffer-1));

   num_bytes_written = fwrite (buffer, 1, strlen(buffer), file_descriptor);
   
   if (num_bytes_written != 0)
   {
       syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: write_timer() - "
              "fwrite() SUCCESS.  num_bytes_written = {%d}, buffer: {%s}"
              , num_bytes_written , buffer);
   }
   else
   {
       syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: write_timer() - "
              "fwrite() FAILED.  num_bytes_written = {%d}, buffer: {%s}"
              , num_bytes_written, buffer);
   }

   syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Exiting write_timer()");
   return (0); 
}


/* Thread function */
void* thread_function(void* thread_param)
{
//   int nr;
//   int ret;
//   int size_received = 0;
//   int s_receive; 
//   int s_send;
//   int current_size = 0;
//////// MOVED BELOW 
    int function_return_status = 0;

    unsigned long num_bytes_read = 0;
    unsigned long num_bytes_written = 0;

    char debug_array[2048];
    memset(debug_array, 0, sizeof(debug_array));

    // Always assume everything fails and check for success.
    FILE *file_descriptor = NULL;

    static char system_input_character;
    int breaker = FILE_WRITE_TIMEOUT;

    unsigned long write_count_counter = 0;
    int read_file_position_is = 0;


//////// MOVED ABOVE   
   int port = 0;
   struct data_thread* thread_info = (struct data_thread *)thread_param;

   // Deal with both IPv4 and IPv6
   if (thread_info->client_address_storage.ss_family == AF_INET)
   {
       char clientIP[INET_ADDRSTRLEN]; // space to hold IPV4 Address.

       struct sockaddr_in *s = (struct sockaddr_in *)
           &thread_info->client_address_storage;

       port = ntohs(s->sin_port);

       inet_ntop(AF_INET, &(s->sin_addr), clientIP,
               INET_ADDRSTRLEN);
       

       syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Server "
               "Socket 'close()' closed connection from IPv4 address: %s, "
               "port: %d", clientIP, port);
       syslog(LOG_USER | LOG_INFO, "Closed connection from %s"
               , clientIP);

   }
   else
   {
       char clientIP[INET6_ADDRSTRLEN]; // space to hold IPv6  Address.
       struct sockaddr_in6 *s = (struct sockaddr_in6 *)&thread_info->client_address_storage;
       port = ntohs(s->sin6_port);

       inet_ntop(AF_INET6, &(s->sin6_addr), clientIP,
               INET6_ADDRSTRLEN);


       syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Server "
               "Socket 'close()' closed connection from IPv6 address: %s, "
               "port: %d", clientIP, port);
       syslog(LOG_USER | LOG_INFO, "Closed connection from %s",
               clientIP);

   }

// DEBUG CODE BELOW - FGREEN moved to thread_function() 
//#if 0
        /////////////////////////////////////////////////////
        //         Write file
        ////////////////////////////////////////////////////

        function_return_status = pthread_mutex_lock(&sharedMemMutexSemaphore);
        if(function_return_status != 0)
        {
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: pthread_mutex_lock() FAILED ");
            
        }


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
            syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Evaluating alarm_timer for writing time to log.  alarm_timer: {%d}", alarm_timer);
            if(alarm_timer)
            {
               write_timer(file_descriptor);
               alarm_timer = false;
            }

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

        function_return_status = pthread_mutex_unlock(&sharedMemMutexSemaphore);
        if(function_return_status != 0)
        {
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: pthread_mutex_unlock() FAILED ");
            
        }

        thread_info->connection_completion_success = true;
//#endif
// DEBUG CODE ABOVE - FGREEN moved to thread_function() 
   return 0;
}
// DEBUG CODE ABOVE - FGREEN WAS NOT HERE


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

    int function_return_status = 0;

    char path_to_write [sizeof(FILE_TO_WRITE_TO)];
//    unsigned long num_bytes_read = 0;
//    unsigned long num_bytes_written = 0;
//
//
//    // Always assume everything fails and check for success.
//    FILE *file_descriptor = NULL;
//
//    static char system_input_character;
    static socklen_t fromlen;

//    char debug_array[2048];
//    memset(debug_array, 0, sizeof(debug_array));

//    int breaker = FILE_WRITE_TIMEOUT;
//
//    unsigned long write_count_counter = 0;
//    int read_file_position_is = 0;

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

    int local_counter = 0; 

    for(;;)
    {

        /* Listen on the socket */
        if(listen(server_sock, 5) < 0)
        {
            perror("Server: listen");
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Server Socket "
                    "'listen()' failed.");
            exit(-1);
        }

        /* Accept connections */
        if((client_sock=accept(server_sock,
                        (struct sockaddr *)&client_sockaddr,
                        &fromlen)) < 0)
        {
            perror("Server: accept");
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Server Socket "
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

                syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Server "
                        "Socket 'accept()' accepted from IPv4 address: %s, "
                        "port: %d", myIpv4, port);
                syslog(LOG_USER | LOG_INFO, "Accepted connection from %s"
                        , myIpv4);
            }
            else
            {
                struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_address_storage;
                port = ntohs(s->sin6_port);

                inet_ntop(AF_INET6, &(s->sin6_addr), myIpv6,
                        INET6_ADDRSTRLEN);

                syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Server "
                        "Socket 'accept()' accepted from IPv6 address: %s, "
                        "port: %d", myIpv6, port);
                syslog(LOG_USER | LOG_INFO, "Accepted connection from %s",
                        myIpv6);
            }
 
            // DEBUG CODE BELOW - FGREEN WAS NOT HERE

            /////////////////////////////////////////////////////
            //         Client Data
            ////////////////////////////////////////////////////

            local_counter++;

            struct data_thread* client_data = malloc(sizeof(struct data_thread));
            client_data->socketfd = client_sock;
            client_data->client_address_storage = client_address_storage;
            client_data->client = &client_sockaddr;
            client_data->connection_completion_success = false;
            client_data->counter = local_counter;


            // DEBUG CODE ABOVE - FGREEN WAS NOT HERE
           function_return_status = pthread_create(&(client_data->thread_id), NULL, thread_function, client_data);
  
           if (function_return_status == 0 )
           {
                syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: "
                       "thread_funtion creation FAILED thread_id {%d}"
                       , (int)client_data->counter);
                
                exit (-1);
           }
           else
           {
                syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: "
                       "thread_funtion created successfully thread_id {%d}"
                       , (int)client_data->counter);
           }

           SLIST_INSERT_HEAD(&head, client_data, entries);
           if(client_data->connection_completion_success)
           {
                syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: "
                       "Completed connection for thread_id {%d}"
                       , (int)client_data->counter);
               
                close(client_data->socketfd);
                pthread_join(client_data->thread_id, NULL);
                SLIST_REMOVE(&head, client_data, data_thread, entries);
           }

           // DEBUG CODE ABOVE - FGREEN WAS NOT HERE

        }

// DEBUG CODE BELOW - FGREEN moved to thread_function() 
#if 0
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

#endif
// DEBUG CODE ABOVE - FGREEN moved to thread_function() 


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

            syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Server "
                    "Socket 'close()' closed connection from IPv4 address: %s, "
                    "port: %d", myIpv4, port);
            syslog(LOG_USER | LOG_INFO, "Closed connection from %s"
                    , myIpv4);

        }
        else
        {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_address_storage;
            port = ntohs(s->sin6_port);

            inet_ntop(AF_INET6, &(s->sin6_addr), myIpv6,
                    INET6_ADDRSTRLEN);

            syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Server "
                    "Socket 'close()' closed connection from IPv6 address: %s, "
                    "port: %d", myIpv6, port);
            syslog(LOG_USER | LOG_INFO, "Closed connection from %s",
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


void alarm_program_handler()
{
    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: "
            "alarm_program_handler(): terminating program");

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
    int function_return_status = 0;

    interrupted = false;

    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: setting up local "
            "socket");
    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: %s", UNIVERSITY_OF_COLORADO_BOULDER_COURSERA);

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


    sock_option_val = SYSTEM_SOCK_OPTION_VAL;
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

//    setsockopt(server_sock, SOL_SOCKET, SO_LINGER, (char*) &opt, sizeof(opt));
//    setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&sockarg,
//            sizeof(int));
   
    if(setsockopt(server_sock, SOL_SOCKET, SO_LINGER, (char*) &opt, sizeof(opt))== -1)
    {
        perror("setsockopt(server_sock)");
        return -1;
    }
    if(setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&sockarg, sizeof(int)) == -1)
    {
        perror("setsockopt(server_sock)");
        return -1;
    }

    signal(SIGINT, external_interrupt_handler);
    signal(SIGPIPE, broken_pipe_handler);
    signal(SIGTERM, terminate_program_handler);
    signal(SIGALRM, alarm_program_handler);

    port = ntohs(server_sockaddr.sin_port);

    inet_ntop(AF_INET, &(server_sockaddr.sin_addr), myIpv4,
            INET_ADDRSTRLEN);

    syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server Socket IPv4 is "
            "to address: %s, port: %d", myIpv4, port);

// DEBUG CODE BELOW - FGREEN WAS NOT HERE
    function_return_status = pthread_mutex_init(&sharedMemMutexSemaphore, NULL);
    if(function_return_status !=0)
    {
        perror("pthread_mutex_init: sharedMemMutexSemaphore");
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: pthread_mutex_init failed with code: %d.", function_return_status);
    }
 
    SLIST_INIT(&head);
    listen(client_sock, BACKLOG);
    if(set_timer() != 0)
    {
       perror("set_timer failed"); 
    }

//    int local_counter = 0; 
//    while(!interrupted)
//    {
//       new_sockfd=accept(socketfd);
//    }
    

// DEBUG CODE ABOVE - FGREEN WAS NOT HERE


    serve_clients_FGREEN();

    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: %s", UNIVERSITY_OF_COLORADO_BOULDER_COURSERA);
    printf("DEBUG CODE - FGREEN: %s", UNIVERSITY_OF_COLORADO_BOULDER_COURSERA);

    return function_status;
}

