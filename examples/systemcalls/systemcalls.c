#include "systemcalls.h"
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
 */
bool do_system(const char *cmd)
{

    /*
     * TODO  add your code here
     *  Call the system() function with the command set in the cmd
     *   and return a boolean true if the system() call completed with success
     *   or false() if it returned a failure
     */

    if(system(cmd) == -1)
    {
        perror("DEBUG CODE- FGREEN: Student error code: system() launch "
                "failed.\n");

        return false;
    }
    return true;
}

/**
 * @param count -The numbers of variables passed to the function. The variables are command to execute.
 *   followed by arguments to pass to the command
 *   Since exec() does not perform path expansion, the command to execute needs
 *   to be an absolute path.
 * @param ... - A list of 1 or more arguments after the @param count argument.
 *   The first is always the full path to the command to execute with execv()
 *   The remaining arguments are a list of arguments to pass to the command in execv()
 * @return true if the command @param ... with arguments @param arguments were executed successfully
 *   using the execv() call, false if an error occurred, either in invocation of the
 *   fork, waitpid, or execv() command, or if a non-zero return value was returned
 *   by the command issued in @param arguments with the specified arguments.
 */

bool do_exec(int count, ...)
{
    // Local function return status
    int status = 0;

    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    /*
     * TODO:
     *   Execute a system command by calling fork, execv(),
     *   and wait instead of system (see LSP page 161).
     *   Use the command[0] as the full path to the command to execute
     *   (first argument to execv), and use the remaining arguments
     *   as second argument to the execv() command.
     *
     */

    pid_t process_id = fork();

    if(process_id == -1)
    {
        perror("DEBUG CODE- FGREEN: Student injected error code: Fork failed\n");
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE- FGREEN: Student injected "
                "error code: Fork failed");

        return false;
    }
    else if (process_id == 0)
    {

        status = execv(command[0], command);

        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN; student injected "
                "error code: If fork failed in function execv() things went "
                "pear-shaped.  Return code, if any is: %d", status);

        exit(1);
    }


    wait(&status);

    if(status != 0)
    {
        perror("DEBUG CODE- FGREEN: Student injected error code: wait() "
                "failed\n");

        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN; student injected "
                "error code: wait() failed with status code: %d.", status);

        return false;
    }


    // DEBUG CODE - FGREEN end of TODO

    va_end(args);

    return true;
}

/**
 * @param outputfile - The full path to the file to write with command output.
 *   This file will be closed at completion of the function call.
 * All other parameters, see do_exec above
 */
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    // Local function return status
    int status = 0;

    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


    /*
     * TODO
     *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
     *   redirect standard out to a file specified by outputfile.
     *   The rest of the behaviour is same as do_exec()
     *
     */

    int file_desciptor;

    pid_t process_id = fork();

    if(process_id == -1)
    {
        perror("DEBUG CODE- FGREEN: Student injected error code: "
                "do_exec_redirect() failed\n");
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE- FGREEN: Student injected "
                "error code: do_exec_redirect() failed");
    }
    else if (process_id == 0)
    {

        file_desciptor = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);

        if(file_desciptor == -1)
        {
            perror("DEBUG CODE - FGREEN; student injected error code: Failed "
                    "to open a file.\n");

            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN; student injected "
                    "error code: Failed to open a file.");

            return false;
        }


        if(dup2(file_desciptor, 1) == -1)
        {
            perror("DEBUG CODE - FGREEN; student injected error code: Failed "
                    "to duplicate a file file_desciptor.\n");

            syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN; student injected "
                    "error code: Failed to duplicate a file_desciptor.");

            return false;
        }

        status = execv(command[0], command);

        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN; student injected "
                "error code: If fork failed in function execv() things went "
                "pear-shaped.  Return code, if any is: %d", status);

        exit(1);
    }


    wait(&status);

    if(status != 0)
    {
        perror("DEBUG CODE- FGREEN: Student injected error code: wait() failed");

        syslog(LOG_USER | LOG_ERR, "DEBUG CODE - FGREEN; student injected "
                "error code: wait() failed with status code: %d.", status);

        return false;
    }


    // DEBUG CODE - FGREEN end of TODO



    va_end(args);

    return true;
}
