#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdbool.h>


#define __SUCCESS__ false
#define __FAIL__    true

#define NUMBER_OF_WRITER_ARGS 3

// C program variant of writer.sh

int main (int argc, char * argv[])
{
    bool status = __FAIL__;

    int file_descriptor;

    // Always assume everything fails and check for success.
    file_descriptor = -1;

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

    // Pointer to the working file, open it, create if necessary, and append.
    if (status == __SUCCESS__)
    {
        file_descriptor = open (argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
    }


    if(file_descriptor == -1)
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
        write (file_descriptor, argv[2], strlen(argv[2]));
        syslog(LOG_USER | LOG_DEBUG, "Writing (%s) to (%s)",
                argv[2], argv[1]);
    }

    // CLOSE file and free pointer
    if(file_descriptor != -1)
    {
        close(file_descriptor);
    }


    exit (status);
}
