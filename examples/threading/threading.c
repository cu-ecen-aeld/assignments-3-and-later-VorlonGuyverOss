#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>

#define SPIN_TIME_MULTIPLIER 1000


#define __LOCAL_SUCCESS__ true
#define __LOCAL_FAIL__    false

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;


    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    usleep(thread_func_args->spin_waiting_for_lock_ms * SPIN_TIME_MULTIPLIER);

    // Default assumption is the thread will always succeed.
    thread_func_args->thread_complete_success = __LOCAL_SUCCESS__;

    int status = pthread_mutex_lock(thread_func_args->mutex);
    if (status !=0)
    {
        printf("DEBUG CODE FGREEN; Student injected error message: "
                "status FAILED from pthread_mutex_lock()");
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE FGREEN; Student injected error "
                "message: status FAILED from pthread_mutex_lock()");

        thread_func_args->thread_complete_success = __LOCAL_FAIL__;
    }
    else
    {
        usleep(thread_func_args->spin_waiting_for_unlock_ms * SPIN_TIME_MULTIPLIER);

        status = pthread_mutex_unlock(thread_func_args->mutex);
        if (status !=0)
        {
            printf("DEBUG CODE FGREEN; Student injected error message: "
                    "status FAILED from pthread_mutex_unlock()");
            syslog(LOG_USER | LOG_ERR, "DEBUG CODE FGREEN; Student injected "
                    "error message: status FAILED from pthread_mutex_unlock()");

            thread_func_args->thread_complete_success = __LOCAL_FAIL__;
        }
    }


    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    int function_status = __LOCAL_FAIL__;

    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    struct thread_data* thread_parameters = (struct thread_data *) malloc(sizeof(struct thread_data));
    thread_parameters->mutex = mutex;
    thread_parameters->spin_waiting_for_lock_ms = wait_to_obtain_ms;
    thread_parameters->spin_waiting_for_unlock_ms = wait_to_release_ms;

    int status = pthread_create(thread, NULL, threadfunc, thread_parameters);
    if (status !=0)
    {
        printf("DEBUG CODE FGREEN; Student injected error message: "
                "status FAILED from pthread_create()");
        syslog(LOG_USER | LOG_ERR, "DEBUG CODE FGREEN; Student injected error "
                "message: status FAILED from pthread_create()");

        printf("DEBUG CODE FGREEN; Student injected error message: "
                "%s:%d FAILED to create pthread %d\n", __FILE__, __LINE__, status);

        syslog(LOG_USER | LOG_ERR, "DEBUG CODE FGREEN; Student injected error "
                "%s:%d FAILED to create pthread %d\n", __FILE__, __LINE__, status);

        function_status = __LOCAL_FAIL__;
    }
    else
    {
        function_status = __LOCAL_SUCCESS__;
    }

    return function_status;
}

