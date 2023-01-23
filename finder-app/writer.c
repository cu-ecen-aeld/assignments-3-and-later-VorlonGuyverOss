#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdbool.h>
#include <libgen.h>
#include <sys/syslog.h>


#define UNIVERSITY_OF_COLORADO_BOULDER_COURSEA "ECEA-5306 Assignment 6"

#define __SUCCESS__ false
#define __FAIL__    true

#define NUMBER_OF_WRITER_ARGS 3

// C program variant of writer.sh

int main (int argc, char * argv[])
{
    bool status = __FAIL__;


    // Always assume everything fails and check for success.
    FILE *file_descriptor = NULL;

    if (argc != NUMBER_OF_WRITER_ARGS)
    {
        int count = 0;

        printf("DEBUG CODE - FGREEN: Student's error message: Invalid number "
                "of arguments.  Need at %d passed to writer.c\n",
                NUMBER_OF_WRITER_ARGS);

        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Student's error "
                "message: ERROR: Invalid number of arguments.  "
               "Need at %d passed to writer.c", NUMBER_OF_WRITER_ARGS);

        printf("DEBUG CODE - FGREEN: Student's error message: Invalid number "
                "of arguments.  Passed argc: %d\n", argc);

        for (count = 0; count < NUMBER_OF_WRITER_ARGS; count++)
        {
            printf("DEBUG CODE - FGREEN: Student's error message: Invalid "
                    "number of arguments.  Passed argv[%d] (%s)\n",
                    count, argv[count]);

            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Student's error "
                    "message: Invalid number of arguments.  "
                    "Passed argv[%d], (%s)\n", count, argv[count]);
        }


        status = __FAIL__;
    }
    else
    {
        status = __SUCCESS__;
    }

    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: %s", UNIVERSITY_OF_COLORADO_BOULDER_COURSEA);

    char *where_to_write = argv[1];
    char *what_to_write = argv[2];

    int check = 0;

    size_t message_alloc = 256;
    char system_message [message_alloc];
    char where_to_write_was[message_alloc];


    memset(system_message, 0, sizeof(system_message));
    sprintf(where_to_write_was, "%s", where_to_write);

    char *parent_directory = dirname(where_to_write);
    printf("parent_directory = %s\n", parent_directory);

    sprintf(system_message, "mkdir -vp %s", parent_directory);
    printf("system_message = %s\n", system_message);

    check = system(system_message);

    // check if directory is created or not
    if (!check)
    {
        printf("Directory created\n");
        status = __SUCCESS__;
    }
    else
    {
        printf("Unable to create directory\n");
        status = __FAIL__;
    }

    struct stat st = {0};

    if (stat(parent_directory, &st) == 0)
    {
        printf ("Parent directory exists: %s\n", parent_directory);
        status = __SUCCESS__;
    }
    else
    {
        printf ("SYSTEM FAIL: Parent directory does NOT exist: %s\n", parent_directory);
        status = __FAIL__;
    }

    // Pointer to the working file, open it, create if necessary, and append.
    if (status == __SUCCESS__)
    {
        file_descriptor = fopen (where_to_write_was, "w+");

        if(file_descriptor == NULL)
        {
            printf("DEBUG CODE - FGREEN: Student's error message: Failed to open "
                    "file.\n");
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN: Student's error "
                    "message: ERROR: Failed to open or create file.");
            status = __FAIL__;
        }
        else
        {
            status = __SUCCESS__;
        }

        if (status != __FAIL__)
        {
            fwrite (what_to_write, 1, strlen(what_to_write), file_descriptor);
            syslog(LOG_USER | LOG_DEBUG, "Writing (%s) to (%s)",
                    argv[2], argv[1]);
        }

        // CLOSE file and free pointer
        if(file_descriptor != NULL)
        {
            fflush(file_descriptor);
            fclose(file_descriptor);
        }
    }

    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: %s", UNIVERSITY_OF_COLORADO_BOULDER_COURSEA);

    exit (status);
}
