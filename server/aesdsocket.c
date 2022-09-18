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
#define SYSTEM_TCP_IP_ADDRESS   "127.0.0.1"
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
extern void serve_clients();
extern void serve_clients_FGREEN();

static int server_sock, client_sock;
//static int fromlen, i, j, num_sets;
//static int i, j, num_sets;
static int j, num_sets;
static unsigned long i;
//static int j, num_sets;
static socklen_t fromlen;
static FILE *fp;
static struct sockaddr_in server_sockaddr, client_sockaddr;



typedef struct localCircularBuffer
{
    unsigned char * camera_pointer_buffer; // pointer to data
    unsigned int  * camera_bytesused_buffer; // pointer to data
    int head; // current head
    int tail; // current tail
    int size;  // current number of items
    int max_length_of_buffer; // Capacity of queue
}localCircularBuffer_t;

localCircularBuffer_t local_circular_buffer;

localCircularBuffer_t create_local_buffer (unsigned int _capacity)
{
    localCircularBuffer_t* myBuffer = (localCircularBuffer_t*)malloc(sizeof(localCircularBuffer_t)); // allocate memory of size

    if(myBuffer == NULL)
    {
        fprintf(stderr, "ERROR: failed to allocate 'myBuffer' local memory\n");
        printf("ERROR: failed to allocate 'myBuffer' local memory");
        syslog(LOG_USER | LOG_INFO, "ERROR: failed to allocate 'myBuffer' local memory");
        exit (EXIT_FAILURE);
    }
    else
    {
        // populate the variables of the queue ;
        myBuffer->head = 0;
        myBuffer->tail = 0;
        myBuffer->max_length_of_buffer = _capacity;
        myBuffer->camera_bytesused_buffer = (unsigned int *)malloc(_capacity * sizeof(unsigned int)); // allocate memory
        myBuffer->camera_pointer_buffer = (unsigned char *)malloc(_capacity * sizeof(unsigned char)); // allocate memory

        syslog(LOG_USER | LOG_INFO, "FGREEN - Successfully created myBuffer");
        return *myBuffer;
    }
}

int local_buff_push(localCircularBuffer_t *pointer, const void *p, unsigned int data)
{
    int next;
    unsigned char *pptr = (unsigned char *)p;

    next = pointer->head + 1; // next is where the head will pint to after write
    if (next >= (pointer->max_length_of_buffer))
    {
        next = 0;
    }

    if(next == (pointer->tail))
    {
        syslog(LOG_USER | LOG_INFO, "FGREEN - Exiting local_buff_push. next == (pointer->tail)");
        return -1;
    }

    pointer->camera_pointer_buffer[pointer->head] = *pptr; // Load data, then move
    pointer->camera_bytesused_buffer[pointer->head] = data; // Load data, then move
    pointer->head = next;    // head to the next data offset
    syslog(LOG_USER | LOG_INFO, "FGREEN - Exiting local_buff_push. Moving to next data");
    return 0;
}

int local_buff_pop(localCircularBuffer_t *pointer, const void *p, unsigned int *data)
{
    int next;
    unsigned char *pptr = (unsigned char *)p;

    if ((pointer->head) == (pointer->tail)) // if head == tail, there is no data
    {
        return -1;
    }

    next = pointer->tail + 1; // next is where tail will point after this read
    if(next >= (pointer->max_length_of_buffer))
    {
        syslog(LOG_USER | LOG_INFO, "FGREEN - Exiting local_buff_pop. next >= (pointer->max_length_of_buffer)");
        next = 0;
    }

    *data = pointer->camera_bytesused_buffer[pointer->tail]; // Read data and them move
    *pptr = pointer->camera_pointer_buffer[pointer->tail]; // Read data and them move
    pointer->tail = next; // tail to the next offset.
    syslog(LOG_USER | LOG_INFO, "FGREEN - Exiting local_buff_pop. Moving to next data");
    return 0; // return success
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

//int write_to_file_FGREEN(char * where, char *  what, unsigned long * num_bytes_written)
int write_to_file_FGREEN(char * where, unsigned long * num_bytes_written)
{

    int status = __LOCAL_FAIL__;

    // Always assume everything fails and check for success.
    FILE *file_descriptor = NULL;

    char *where_to_write = where;
//    char *what_to_write = what;

    static char system_input_character;

    int check = 0;
    int breaker = FILE_WRITE_TIMEOUT;

    size_t message_alloc = 256;
    char system_message [message_alloc];
    char where_to_write_was[message_alloc];


    memset(system_message, 0, sizeof(system_message));
    sprintf(where_to_write_was, "%s", where_to_write);

    //char *path = "./tmp/tmp/asdf";

    //char *path = reallocarray(where, sizeof(char),

    status = __LOCAL_SUCCESS__;
    // Pointer to the working file, open it, create if necessary, and append.
    if (status == __LOCAL_SUCCESS__)
    {
        file_descriptor = fopen (where_to_write_was, "a+");

        if(file_descriptor == NULL)
        {
            printf("DEBUG CODE - FGREEN: Student's error message: Failed to open "
                    "file.\n");
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Student's error "
                    "message: ERROR: Write failed to open or create file.");
            status = __LOCAL_FAIL__;
        }
        else
        {
            status = __LOCAL_SUCCESS__;
        }

        if (status != __LOCAL_FAIL__)
        {
//            *num_bytes_written = (unsigned long) fwrite (what_to_write, 1,
//                    strlen(what_to_write), file_descriptor);

                unsigned long count_counter = 0;
                fp = fdopen(client_sock, "r");

                while((system_input_character = fgetc(fp)) != EOF)
                {
                        putchar(system_input_character);
//                        *num_bytes_written = (unsigned long) fwrite (&system_input_character, 1,
//                              strlen(&system_input_character), file_descriptor);

                        count_counter = (unsigned long) fwrite (&system_input_character, 1,
                              strlen(&system_input_character), file_descriptor);

                        *num_bytes_written += count_counter;


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

        // CLOSE file and free pointer
        if(file_descriptor != NULL)
        {
            fflush(file_descriptor);
            fclose(file_descriptor);
        }
    }


    return status;
}




//int read_from_file_FGREEN(char * where, char *  what, unsigned long * num_bytes_read)
int read_from_file_FGREEN(char * where, char *  what,
        unsigned long * num_bytes_read, int position, int client_sock)
{

    int status = __LOCAL_FAIL__;

    // Always assume everything fails and check for success.
    FILE *file_descriptor = NULL;

    //char *read_buffer [num_bytes_written];
    //char what_to_read[position + 1];
    char *where_to_write = where;
    char *what_to_read = what;

//    int check = 0;

    size_t message_alloc = 256;
    char system_message [message_alloc];
    char where_to_write_was[message_alloc];


    memset(what_to_read, 0, sizeof(*what_to_read));
    memset(system_message, 0, sizeof(system_message));
    sprintf(where_to_write_was, "%s", where_to_write);

    //char *path = "./tmp/tmp/asdf";

    //char *path = reallocarray(where, sizeof(char),

//    char *parent_directory = dirname(where_to_write);
//    //char *parent_directory = dirname(path);
//    printf("parent_directory = %s\n", parent_directory);

//    sprintf(system_message, "mkdir -vp %s", parent_directory);
//    printf("system_message = %s\n", system_message);
//
//    check = system(system_message);
//
//    // check if directory is created or not
//    if (!check)
//    {
//        printf("Directory created\n");
//        status = __LOCAL_SUCCESS__;
//    }
//    else
//    {
//        printf("Unable to create directory\n");
//        status = __LOCAL_FAIL__;
//    }

//    struct stat st = {0};
//
//    if (stat(parent_directory, &st) == 0)
//    {
//        printf ("Parent directory exists: %s\n", parent_directory);
//        status = __LOCAL_SUCCESS__;
//    }
//    else
//    {
//        printf ("SYSTEM FAIL: Parent directory does NOT exist: %s\n", parent_directory);
//        status = __LOCAL_FAIL__;
//    }

    status = __LOCAL_SUCCESS__;
    // Pointer to the working file, open it, create if necessary, and append.
    if (status == __LOCAL_SUCCESS__)
    {
        file_descriptor = fopen (where_to_write_was, "r");

        if(file_descriptor == NULL)
        {
            printf("DEBUG CODE - FGREEN: Student's error message: Failed to open "
                    "file.\n");
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Student's error "
                    "message: ERROR: Failed to open or create file.");
            status = __LOCAL_FAIL__;
        }
        else
        {
            status = __LOCAL_SUCCESS__;
        }

        if (status != __LOCAL_FAIL__)
        {
            //fwrite (what_to_write, 1, strlen(what_to_write), file_descriptor);


//            *num_bytes_written = (unsigned long) fwrite (what_to_write, 1,
//                    strlen(what_to_write), file_descriptor);

//            *num_bytes_read = (unsigned long) fread(what_to_read, 1, strlen(what_to_read),
//                    file_descriptor);

            int i =0;
            unsigned long count_counter = 0;
#if 0       // READ BUFFER OVER FLOW TEST
            int file_size = 0;
            fseek(file_descriptor, 0L, SEEK_END);
            file_size = ftell(file_descriptor);
            fseek(file_descriptor, 0l, SEEK_SET);

            for (i=0; i< file_size; i++)
#endif
            for (i=0; i< position; i++)
            {
#if 0
                *num_bytes_read = (unsigned long) fread(&what_to_read[i],
                        sizeof(what_to_read[i]), 1, file_descriptor);
#endif

                count_counter = (unsigned long) fread(&what_to_read[i],
                        sizeof(what_to_read[i]), 1, file_descriptor);

                *num_bytes_read += count_counter;

            sprintf(system_message, "%s", &what_to_read[i]);

//            send(client_sock, &system_message, strlen(system_message), 0);
//            send(client_sock, &what_to_read, strlen(what_to_read), 0);
            }
//            *num_bytes_read = (unsigned long) fread(&what_to_read[position],
//                    sizeof(&what_to_read[position]), 1, file_descriptor);

//            sprintf(system_message, "%s", what_to_read);
//
//            send(client_sock, &system_message, strlen(system_message), 0);

            syslog(LOG_USER | LOG_DEBUG, "DEBUG CODE - FGREEN: Reading (%s) to (%s): returned %lu "
                    "num_bytes_read", what, where, *num_bytes_read);
        }

        // CLOSE file and free pointer
        if(file_descriptor != NULL)
        {
            fflush(file_descriptor);
            fclose(file_descriptor);
        }
    }


    return status;
}













/* Listen and accept loop function */
void serve_clients_FGREEN()
{
    //int breaker = FILE_WRITE_TIMEOUT;
    // DEBUG CODE BELOW - FGREEN
    char myIpv4[INET_ADDRSTRLEN]; // space to hold IPV4 Address.
    char myIpv6[INET6_ADDRSTRLEN]; // space to hold IPv6  Address.
    struct sockaddr_in experiment;
    struct sockaddr_storage client_address_storage;
    socklen_t client_address_length;
    client_address_length = sizeof(client_address_storage);
    int port = 0;
    char hostname[64];

    //char path_to_write [] = FILE_TO_WRITE_TO;
    char path_to_write [sizeof(FILE_TO_WRITE_TO)];
    //char path_to_write [] = "./tmp/tmp/aesdsocketdata";
    unsigned long num_bytes_read = 0;
    unsigned long num_bytes_written = 0;
    unsigned long num_bytes_written_total = 0;

    //    unsigned long *pointer_num_bytes_read = &num_bytes_read;
    unsigned long *pointer_num_bytes_written = &num_bytes_written;

    char *what_to_read;

    what_to_read = (char*)malloc(2048*sizeof(char));

    // Always assume everything fails and check for success.
    FILE *file_descriptor = NULL;

    static char system_input_character;
    static char system_output_character;

    char debug_array[2048];
    int debug_array_counter = 0;
    memset(debug_array, 0, sizeof(debug_array));

    int check = 0;
    int breaker = FILE_WRITE_TIMEOUT;
    int count_counter = 0;
    int file_amount_to_read = 0;

    size_t message_alloc = 256;
    char system_message [message_alloc];

    int i =0;
    unsigned long write_count_counter = 0;
    unsigned long read_count_counter = 0;
    int read_file_position_is = 0;
    int read_file_position_was = 0;
    int read_count = 0;

    memset(path_to_write, 0, sizeof(path_to_write));
    memset(hostname, 0, sizeof(hostname));
    memset(myIpv4, 0, sizeof(myIpv4));
    memset(myIpv6, 0, sizeof(myIpv6));


    gethostname(hostname, sizeof(hostname));
    inet_pton(AF_INET, SYSTEM_TCP_IP_ADDRESS, &(experiment.sin_addr));
    inet_ntop(AF_INET, &(experiment.sin_addr), myIpv4, INET_ADDRSTRLEN);
    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN - myIpv4 %s", myIpv4);
    // DEBUG CODE ABOVE - FGREEN

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
#if 0
            if(getpeername(server_sock,
                        (struct sockaddr *)&client_address_storage, &client_address_length))
            {
                perror("Server: getpeer() failed");
                syslog(LOG_INFO | LOG_ERR, "Server Socket 'getpeername()' "
                        "failed.");
                exit(-1);
            }
#endif

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





        memset(system_message, 0, sizeof(system_message));

        //char *path = "./tmp/tmp/asdf";

        //char *path = reallocarray(where, sizeof(char),

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

                debug_array[debug_array_counter] = system_input_character;

                debug_array_counter++;

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

        // CLOSE file and free pointer
        if(file_descriptor != NULL)
        {
     //       fflush(file_descriptor);
     //       fclose(file_descriptor);
     //       fflush(fp);
     //       fclose(fp);
        }







        /////////////////////////////////////////////////////
        //         Read file
        ////////////////////////////////////////////////////




    //    file_descriptor = fopen (FILE_TO_WRITE_TO, "r");

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
            //fseek(file_descriptor, 0L, SEEK_SET);
            fseek(file_descriptor, 0L, SEEK_END);
            read_file_position_is = ftell(file_descriptor);
            fseek(file_descriptor, 0L, SEEK_SET);
            // Total file size minus the number just written
            // should the the section needed to pas the test.

            what_to_read = realloc(what_to_read,
                    (read_file_position_is + 1));

            if (what_to_read == NULL)
            {
                printf("Error reallocating space for 'what_to_read'");
            }

            memset(system_message, 0, sizeof(system_message));
            memset(what_to_read, 0, sizeof(num_bytes_written + 1));


#if 0
            for (i = 0; i < (read_file_position_is); i++)
            {
                what_to_read[i] = 0;
            }

#endif

            read_count_counter = (unsigned long) fread(what_to_read,
                    num_bytes_written, 1, file_descriptor);

            sprintf(system_message, "%s", what_to_read);

            printf("DEBUG CODE- FGREEN: num_bytes_written: %lu\n", num_bytes_written);
            printf("DEBUG CODE - FGREEN num_bytes_written_total: %lu\n", num_bytes_written_total);
            printf("DEBUG CODE- FGREEN: read_file_position_is: %d\n", read_file_position_is);
            printf("DEBUG CODE - FGREEN: read_file_position_was: %d\n", read_file_position_was);

            if (what_to_read[num_bytes_written - 1] != '\n')
            {
                printf("DEBUG CODE - FGREE: no newline '\n'.  Continue\n");

            }


            send(client_sock, what_to_read, read_file_position_is, 0);


            //fseek(file_descriptor, read_file_position_was, SEEK_SET);
            //fseek(file_descriptor, (- num_bytes_written), SEEK_CUR);

            read_count_counter = 0;

//            system_output_character = getc(file_descriptor); This line causes
//            my computer to create a spaced input file.

            read_count = 0;
//            for (i = 0; i < read_file_position_is; i++)
//#endif
//                for (i=0; i< num_bytes_written; i++)
//                {
//#if 0
             //       num_bytes_read = (unsigned long) fread(&what_to_read[i],
              //              sizeof(what_to_read[i]), 1, file_descriptor);

            //sprintf(system_message, "%s", &what_to_read[i]);

            //send(client_sock, &system_message, strlen(system_message), 0);
//
//            while (system_output_character != '\n' && system_output_character != EOF)
//            {
//               if (count_counter == read_file_position_is) //               {
//                   what_to_read = realloc(what_to_read, read_file_position_is + 1);
//                   if (what_to_read == NULL)
//                   {
//                       printf("Error reallocating space for 'what_to_read'");
//                   }
//
//               }
//                   what_to_read[count_counter] = system_output_character;
//
//                   count_counter++;
//
//                   system_output_character = getc(file_descriptor);
//            }
//
//            what_to_read[count_counter] = '\0';
//
//            printf ("GRRRR: what_to_read[count_counter]: %s\n", what_to_read);
//
//
//            sprintf(system_message, "%s", what_to_read);
//
//            printf ("GRRRR: sprintf - what_to_read[count_counter]: %s\n",system_message);
//
//            //send(client_sock, &system_message, strlen(system_message), 0);
//            send(client_sock, &what_to_read, strlen(what_to_read), 0);









////
//                    //read_count++;
//
//                    //num_bytes_read += read_count_counter;
//
//                    *what_to_read++ = (char)fgetc(file_descriptor);
//
//                    //sprintf(system_message, "%s", what_to_read);
//                    //sprintf(system_message, "%c", system_output_character);
//
//                    //send(client_sock, &system_message, strlen(system_message), 0);
//
//
//                }
//
//                    sprintf(system_message, "%s", what_to_read);
//                    send(client_sock, &system_message, strlen(system_message), 0);
//
//                    printf("DEBUG CODE - FGREEN: reading file: %s\n",
//                            what_to_read);
//                    printf("DEUBG CODE - FGREEN: debug_array: %s\n",
//                            debug_array);







//       /* Send strings to the client */
////       for (i=0; i< (unsigned long) num_bytes_read; i++)
////       {
// //           sprintf(system_message, "%s", read_buffer);
//
////            send(client_sock, &system_message, strlen(system_message), 0);
////           send(client_sock, &read_buffer, strlen(read_buffer), 0);
//           syslog(LOG_USER | LOG_DEBUG, "DEBUG CODE - FGREEN: "
//                   "num_bytes_written_total = %lu, num_bytes_read = %lu",
//                   num_bytes_written_total, num_bytes_read);
////       }
//
////       if (num_bytes_read != num_bytes_written_total)
////       {
////            perror("Read file FAILED");
////            syslog(LOG_INFO | LOG_ERR, "DEBUG CODE - FGREEN: Read file FAILED "
////                    "num_bytes_written = %lu, num_bytes_read = %lu",
////                    num_bytes_written, num_bytes_read);
////            exit(-1);
////       }
//


#if 0
            for (i=0; i< position; i++)
            {
#if 0
                *num_bytes_read = (unsigned long) fread(&what_to_read[i],
                        sizeof(what_to_read[i]), 1, file_descriptor);
#endif

             aa   count_counter = (unsigned long) fread(&what_to_read[i],
                        sizeof(what_to_read[i]), 1, file_descriptor);

                *num_bytes_read += count_counter;

            sprintf(system_message, "%s", &what_to_read[i]);

            send(client_sock, &system_message, strlen(system_message), 0);
//            send(client_sock, &what_to_read, strlen(what_to_read), 0);
            }
#endif





                    printf("DEBUG CODE - FGREEN: reading file: %s\n",
                            what_to_read);

//            fseek(file_descriptor, 0, SEEK_SET);
            //fseek(file_descriptor, read_file_position_was, SEEK_SET);
//                    read_count_counter = (unsigned long) fread(what_to_read,
//                         (read_file_position_is + 1),
//                         1, file_descriptor);

//                    read_count_counter = (unsigned long) fread(what_to_read,
//                         read_file_position_is,
//                         1, file_descriptor);
//                   sprintf(system_message, "%s", what_to_read);
//
//                    send(client_sock, &system_message, strlen(system_message), 0);
////                    send(client_sock, &debug_array, strlen(debug_array), 0);
////                    send(client_sock, "TEST FGREEN", strlen("TEST FGREEN"), 0);

                    printf("DEBUG CODE - FGREEN: reading file: %s\n",
                            what_to_read);
                    printf("DEUBG CODE - FGREEN: debug_array: %s\n",
                            debug_array);


            syslog(LOG_USER | LOG_DEBUG, "DEBUG CODE - FGREEN: Reading (%s) to (%s): returned %lu "
                    "num_bytes_read", system_message, FILE_TO_WRITE_TO, num_bytes_read);

            read_file_position_was = read_file_position_is;
        }

        // CLOSE file and free pointer
        if(file_descriptor != NULL)
        {
            fflush(file_descriptor);
            fclose(file_descriptor);
            fflush(fp);
            fclose(fp);
        }

     //   free(what_to_read);








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




























































































































///* Listen and accept loop function */
//void serve_clients()
//{
//    //int breaker = FILE_WRITE_TIMEOUT;
//    // DEBUG CODE BELOW - FGREEN
//    char myIpv4[INET_ADDRSTRLEN]; // space to hold IPV4 Address.
//    char myIpv6[INET6_ADDRSTRLEN]; // space to hold IPv6  Address.
//    struct sockaddr_in experiment;
//    struct sockaddr_storage client_address_storage;
//    socklen_t client_address_length;
//    client_address_length = sizeof(client_address_storage);
//    int port = 0;
//    char hostname[64];
//
//    //char path_to_write [] = FILE_TO_WRITE_TO;
//    char path_to_write [sizeof(FILE_TO_WRITE_TO)];
//    //char path_to_write [] = "./tmp/tmp/aesdsocketdata";
//    unsigned long num_bytes_read = 0;
//    unsigned long num_bytes_written = 0;
//    unsigned long num_bytes_written_total = 0;
//
////    unsigned long *pointer_num_bytes_read = &num_bytes_read;
//    unsigned long *pointer_num_bytes_written = &num_bytes_written;
//
//
//    memset(path_to_write, 0, sizeof(path_to_write));
//    memset(hostname, 0, sizeof(hostname));
//    memset(myIpv4, 0, sizeof(myIpv4));
//    memset(myIpv6, 0, sizeof(myIpv6));
//
//
//    gethostname(hostname, sizeof(hostname));
//    inet_pton(AF_INET, SYSTEM_TCP_IP_ADDRESS, &(experiment.sin_addr));
//    inet_ntop(AF_INET, &(experiment.sin_addr), myIpv4, INET_ADDRSTRLEN);
//    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN - myIpv4 %s", myIpv4);
//    // DEBUG CODE ABOVE - FGREEN
//
//    sprintf(path_to_write, "%s", FILE_TO_WRITE_TO);
//    create_temporary_file(path_to_write);
//
//    for(;;)
//    {
//
//        /* Listen on the socket */
//        if(listen(server_sock, 5) < 0)
//        {
//            perror("Server: listen");
//            syslog(LOG_INFO | LOG_ERR, "DEBUG CODE - FGREEN: Server Socket "
//                    "'listen()' failed.");
//            exit(-1);
//        }
//
//        /* Accept connections */
//        if((client_sock=accept(server_sock,
//                        (struct sockaddr *)&client_sockaddr,
//                        &fromlen)) < 0)
//        {
//            perror("Server: accept");
//            syslog(LOG_INFO | LOG_ERR, "DEBUG CODE - FGREEN: Server Socket "
//                    "'accept()' failed.");
//            exit(-1);
//        }
//        else
//        {
//#if 0
//            if(getpeername(server_sock,
//                    (struct sockaddr *)&client_address_storage, &client_address_length))
//            {
//                perror("Server: getpeer() failed");
//                syslog(LOG_INFO | LOG_ERR, "Server Socket 'getpeername()' "
//                        "failed.");
//                exit(-1);
//            }
//#endif
//
//            getpeername(server_sock,
//                    (struct sockaddr *)&client_address_storage,
//                    &client_address_length);
//
//#if 0
//            syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: "
//                    "client_address_storage struct contains: "
//                    "ss_family = %d",
//                    client_address_storage.ss_family);
//            syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: "
//                    "client_address_storage struct contains: "
//                    "__ss_padding = %s",
//                    client_address_storage.__ss_padding);
//            syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: "
//                    "client_address_storage struct contains: "
//                    "ss_align = %ld",
//                    client_address_storage.__ss_align);
//#endif
//            // Convert system IP address to a string
//            // Deal with both IPv4 and IPv6
//            if (client_address_storage.ss_family == AF_INET)
//            {
//                struct sockaddr_in *s = (struct sockaddr_in *)
//                    &client_address_storage;
//
//                port = ntohs(s->sin_port);
//
//                inet_ntop(AF_INET, &(s->sin_addr), myIpv4,
//                        INET_ADDRSTRLEN);
//
//                syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server "
//                        "Socket 'accept()' accepted from IPv4 address: %s, "
//                        "port: %d", myIpv4, port);
//                syslog(LOG_INFO | LOG_INFO, "Accepted connection from %s"
//                        , myIpv4);
//            }
//            else
//            {
//                struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_address_storage;
//                port = ntohs(s->sin6_port);
//
//                inet_ntop(AF_INET6, &(s->sin6_addr), myIpv6,
//                        INET6_ADDRSTRLEN);
//
//                syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server "
//                        "Socket 'accept()' accepted from IPv6 address: %s, "
//                        "port: %d", myIpv6, port);
//                syslog(LOG_INFO | LOG_INFO, "Accepted connection from %s",
//                        myIpv6);
//            }
//        }
//
////        fp = fdopen(client_sock, "r");
//#if 0
//        recv(client_sock, (char *)&num_sets, sizeof(int), 0);
//        printf("number of sets = %d\n", num_sets);
//
//        for(j=0;j<num_sets;j++)
//        {
//#endif
//
//#if 0
//            /* Send strings to the client */
//            for (i=0; i<NSTRS; i++)
//                send(client_sock, test_strs[i], strlen(test_strs[i]), 0);
//
//#endif
//            /* Read client strings and print them out */
////            for (i=0; i<NSTRS; i++)
////            for (i=0; i<1; i++)     // <-- FORREST THIS change is due to test's small reaction time. KEEP
////            {
//                        sprintf(path_to_write, "%s", FILE_TO_WRITE_TO);
//
//                        write_to_file_FGREEN(path_to_write,
//                                pointer_num_bytes_written);
////                        write_to_file_FGREEN(path_to_write,
////                                &system_input_character,
////                                pointer_num_bytes_written);
////                while((system_input_character = fgetc(fp)) != EOF)
////                {
////                        sprintf(path_to_write, "%s", FILE_TO_WRITE_TO);
////                    //if(num_sets < 4)
////                        putchar(system_input_character);
////
//////                        write_to_file_FGREEN(FILE_TO_WRITE_TO,
//////                                &system_input_character);
//////                        write_to_file_FGREEN("tmp/tmp/aesdsocketdata",
//////                                &system_input_character);
////                        write_to_file_FGREEN(path_to_write,
////                                &system_input_character,
////                                pointer_num_bytes_written);
////
//                        num_bytes_written_total += num_bytes_written;
//////#if 0
////                    if(system_input_character == '\n')
////                    {
////                        printf("BREAKING '/\n'\n");
////
//////                        sprintf(path_to_write, "%s", FILE_TO_WRITE_TO);
////
//////                        write_to_file_FGREEN(path_to_write,
//////                                &system_input_character,
//////                                pointer_num_bytes_written);
////                        break;
////                    }
//////#endif
////
////                    if(breaker < 1)
////                    {
////                        //breaker = 10000;
////                        printf("BREAKING from breaker\n");
////                        breaker = FILE_WRITE_TIMEOUT;
////                        break;
////                    }
////
////                    breaker--;
////
////                } /* end while */
////#if 0
//
////                        printf("BREAKING i=0  %d\n", i);
////            } /* end for NSTRS */
//
////#endif
// //                       printf("BREAKING j=0  \n");
//#if 0
//        } /* end for num_sets */
//#endif
//
//  //                      printf("BREAKING socket\n");
//
//       //char *read_buffer [num_bytes_written];
//       char read_buffer[num_bytes_written_total + 1];
//
//       char *pointer_read_buffer;
//
//       pointer_read_buffer = read_buffer;
//
////       read_buffer = calloc(num_bytes_written_total, sizeof(unsigned long));
//
//       memset(&read_buffer, 0 , sizeof(read_buffer));
//
//       //num_bytes_read = num_bytes_written_total;
//
//       sprintf(path_to_write, "%s", FILE_TO_WRITE_TO);
//
////       /* Send strings to the client */
////       for (i=0; i<num_bytes_written_total; i++)
////       {
//           sprintf(path_to_write, "%s", FILE_TO_WRITE_TO);
//
////           read_from_file_FGREEN(path_to_write,
////                   pointer_read_buffer, &num_bytes_read, i);
//           read_from_file_FGREEN(path_to_write,
//                   pointer_read_buffer, &num_bytes_read, num_bytes_written_total, client_sock);
////           read_from_file_FGREEN(path_to_write,
////                   pointer_read_buffer, &num_bytes_read, num_bytes_written, client_sock);
////       }
//
//       /* Send strings to the client */
////       for (i=0; i< (unsigned long) num_bytes_read; i++)
////       {
// //           sprintf(system_message, "%s", read_buffer);
//
////            send(client_sock, &system_message, strlen(system_message), 0);
////           send(client_sock, &read_buffer, strlen(read_buffer), 0);
//           syslog(LOG_USER | LOG_DEBUG, "DEBUG CODE - FGREEN: "
//                   "num_bytes_written_total = %lu, num_bytes_read = %lu",
//                   num_bytes_written_total, num_bytes_read);
////       }
//
////       if (num_bytes_read != num_bytes_written_total)
////       {
////            perror("Read file FAILED");
////            syslog(LOG_INFO | LOG_ERR, "DEBUG CODE - FGREEN: Read file FAILED "
////                    "num_bytes_written = %lu, num_bytes_read = %lu",
////                    num_bytes_written, num_bytes_read);
////            exit(-1);
////       }
//
//
//
//
////       free(read_buffer);
//
//        close(client_sock);
//
//
////        num_bytes_read = 0;
////        num_bytes_written = 0;
////        num_bytes_written_total = 0;
//
//        // Convert system IP address to a string
//        // Deal with both IPv4 and IPv6
//        if (client_address_storage.ss_family == AF_INET)
//        {
//            struct sockaddr_in *s = (struct sockaddr_in *)
//                &client_address_storage;
//
//            port = ntohs(s->sin_port);
//
//            inet_ntop(AF_INET, &(s->sin_addr), myIpv4,
//                    INET_ADDRSTRLEN);
//
//            syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server "
//                    "Socket 'close()' closed connection from IPv4 address: %s, "
//                    "port: %d", myIpv4, port);
//            syslog(LOG_INFO | LOG_INFO, "Closed connection from %s"
//                    , myIpv4);
//
// //           remove_temporary_file();
//        }
//        else
//        {
//            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_address_storage;
//            port = ntohs(s->sin6_port);
//
//            inet_ntop(AF_INET6, &(s->sin6_addr), myIpv6,
//                    INET6_ADDRSTRLEN);
//
//            syslog(LOG_INFO | LOG_INFO, "DEBUG CODE - FGREEN: Server "
//                    "Socket 'close()' closed connection from IPv6 address: %s, "
//                    "port: %d", myIpv6, port);
//            syslog(LOG_INFO | LOG_INFO, "Closed connection from %s",
//                    myIpv6);
//
//  //          remove_temporary_file();
//        }
//
//    } /* end for ever */
//
//}


/* Close sockets after a Ctrl-C interrupt */

void external_interrupt_handler()
{
//    char system_input_character;
//
//    printf("Enter y to close sockets or n to keep open:");
//    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN, asking $USER for "
//            "input.");
//    scanf("%c", &system_input_character);
//
//    if(system_input_character == 'y')
//    {
        printf("\nsockets are being closed\n");
        syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN, Server: sockets "
                "are being closed.");
        close(client_sock);
        close(server_sock);

        remove_temporary_file();
//    }
//
//    printf("Server: Shutting down ...\n");
//    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN, Server: Shutting down.");
//
//    remove_temporary_file();
//
//
    exit(-1);

}


void broken_pipe_handler()
{
//    char system_input_character;
//
//    printf("Enter y to continue serving clients or n to halt:");
//    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: Broken Pipe: asking "
//            "$USER for input.");
//    scanf("%c", &system_input_character);
//
//    if(system_input_character == 'y')
//    {
//        printf("\nwill continue serving clients\n");
//        syslog(LOG_USER | LOG_INFO,"DEBUG CODE - FGREEN: broken_pipe_handler()"
//                " will continue serving clients.");
//        serve_clients();

        close(client_sock);
        close(server_sock);
        remove_temporary_file();
//    }
//
//    else
//    {
//        printf("Server: Shutting down ...\n");
//        syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: broken_pipe_handler"
//                "(): Server: Shutting down...");
//
//        remove_temporary_file();

        exit(-1);
//    }

}

void terminate_program_handler()
{
    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: "
            "terminate_program_handler(): terminating program");

    remove_temporary_file();

    exit(0);
}

int main(int argc, char ** argv)
{
    int function_status = __LOCAL_FAIL__;

    local_circular_buffer = create_local_buffer(SIZE_OF_LOCAL_BUFFER);

    char hostname[64];
    struct hostent *hp;
    struct linger opt;
    int sockarg;

//    char data_caracter;
//    FILE *file_pointer;
//    int client_sock;
//    int lenth;
//    int num_sets;
//    int counter_0;
//    int counter_1;


    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: setting up local "
            "socket");

    // DEBUG CODE BELOW - FGREEN
    char myIpv4[INET_ADDRSTRLEN]; // space to hold my designated IP Address.
    char myIpv6[INET6_ADDRSTRLEN]; // space to hold IPv6  Address.
    struct sockaddr_in experiment;

    //struct sockaddr_storage server_address_storage;
    //socklen_t server_address_length;
    //server_address_length = sizeof(server_address_storage);
    int port = 0;

    memset(hostname, 0, sizeof(hostname));
    memset(myIpv4, 0, sizeof(myIpv4));
    memset(myIpv6, 0, sizeof(myIpv6));



    inet_pton(AF_INET, SYSTEM_TCP_IP_ADDRESS, &(experiment.sin_addr));
    inet_ntop(AF_INET, &(experiment.sin_addr), myIpv4, INET_ADDRSTRLEN);
    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: myIpv4 %s", myIpv4);
    // DEBUG CODE ABOVE - FGREEN
    gethostname(hostname, sizeof(hostname));


    //socket();

    //bind();


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
    // DEBUG CODE BELOW - FGREEN
    //server_sockaddr.sin_addr = "10.0.10.10";
    // DEBUG CODE ABOVE - FGREEN
    bcopy (hp->h_addr, &server_sockaddr.sin_addr, hp->h_length);

    // DEBUG CODE BELOW - FGREEN
    //inet_pton(AF_INET, "10.0.10.10", &(server_sockaddr.sin_addr));
    inet_pton(AF_INET, SYSTEM_TCP_IP_ADDRESS, &(server_sockaddr.sin_addr));
    // DEBUG CODE ABOVE - FGREEN


    /* Bind address to the socket */
    if(bind(server_sock, (struct sockaddr *) &server_sockaddr,
                sizeof(server_sockaddr)) < 0)
    {
        perror("Server: bind");
        exit(-1);
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


    //serve_clients();
    serve_clients_FGREEN();



    return function_status;
}

