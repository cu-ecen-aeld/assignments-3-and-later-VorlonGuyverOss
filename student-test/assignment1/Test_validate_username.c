#include "unity.h"
#include <stdbool.h>
#include <stdlib.h>
#include "../../examples/autotest-validate/autotest-validate.h"
#include "../../assignment-autotest/test/assignment1/username-from-conf-file.h"

#include <syslog.h>
#include <string.h>
#include "../../assignment-autotest/Unity/src/unity.h"

/**
 * This function should:
 *   1) Call the my_username() function in autotest-validate.c to get your hard coded username.
 *   2) Obtain the value returned from function malloc_username_from_conf_file() in username-from-conf-file.h within
 *       the assignment autotest submodule at assignment-autotest/test/assignment1/
 *   3) Use unity assertion TEST_ASSERT_EQUAL_STRING_MESSAGE to verify the two strings are equal.  See
 *       the [unity assertion reference](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md)
 */
void test_validate_my_username()
{
    /**
     * TODO: Replace the line below with your code here as described above to verify your /conf/username.txt
     * config file and my_username() functions are setup properly
     */
    //TEST_ASSERT_TRUE_MESSAGE(false,"AESD students, please fix me!");

    char temp_buff [256];
    memset(temp_buff, 0, sizeof(temp_buff));

    my_username();

    sprintf(temp_buff, "%s",  malloc_username_from_conf_file());

    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: temp_buff[%s]", temp_buff);
    syslog(LOG_USER | LOG_INFO, "DEBUG CODE - FGREEN: malloc_username_from_conf_file[%s]", malloc_username_from_conf_file());

    TEST_ASSERT_EQUAL_STRING("VorlonGuyverOss", temp_buff);
}
