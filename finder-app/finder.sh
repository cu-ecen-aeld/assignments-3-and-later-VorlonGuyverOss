#!/bin/sh


PWD=$(pwd)
TMPFILE_COUNT=0
TMPCOUNTER=0

# Handle script input
TOTAL_SCRIPT_INPUT=$#
FIRST_INPUT=$1
SECOND_INPUT=$2

# Local Data Manipulation Files
TEMP_TXT='.tmp.txt'

#Write a shell script finder-app/finder.sh as described below:

# Accepts the following runtime arguments: the first argument is a path to a
# directory on the filesystem, referred to below as <b>filesdir</b>; the second
# argument is a text string which will be searched within these files,
# referred to below as <b>searchstr</b>
#
# Exits with return value 1 error and print statements if any of the
# parameters above were not specified
#
# Exits with return value 1 error and print statements if <b>filesdir</b>
# does not represent a directory on the filesystem
#
# Prints a message "The number of files are X and the number of
# matching lines are Y" where X is the number of files in the
# directory and all subdirectories and Y is the number of matching
# lines found in respective files.
#
# Example invocation:
#
# finder.sh /tmp/aesd/assignment1 linux

input_error_check()
{
    if [ ${TOTAL_SCRIPT_INPUT} -ne 2 ] ; then
    {
        #FGREEN
        echo "ERROR: the script input requires two strings"
        echo "First input is the directory"
        echo "Second input is the string to search for"
        echo "EXAMPLE: finder.sh /tmp/aesd/assignment1 linux"
        echo "EXAMPLE: finder.sh ${PWD} \"the script input requires two strings\""
        echo "EXAMPLE: finder.sh then returns actual results such as:"
        echo
        echo "\<local_dir\> \$ ./finder.sh ${FIRST_INPUT} ${PWD} 'you found me'"
        echo
        $0 ${PWD} 'you found me'  #<-- :-)
        echo
        echo


        return 1
    }
    fi

    return 0
}

initSubshellDataStore()
{
    if [ ! -f ${TEMP_TXT} ]; then
    {
        touch ${TEMP_TXT}
    }
    fi
}

cleanUp()
{
    rm ${TEMP_TXT}
}


mainLoop()
{
    #echo "Looking for directory: ${FIRST_INPUT}"

    if [ ! -d ${FIRST_INPUT} ]; then
    {
        echo "ERROR: directory not found: ${FIRST_INPUT}"
        return 1
    }
    else
    {
        local file_count=0
        local counter=0

        #echo #echo "SUCCESS: directory found: ${FIRST_INPUT}"
        #echo "SUCCESS: finder.sh found the following:"
        #grep -Ric "${SECOND_INPUT}" ${FIRST_INPUT}
        #echo ${SECOND_INPUT}
        #echo ${FIRST_INPUT}
        grep -Ric "${SECOND_INPUT}" ${FIRST_INPUT} > ${TEMP_TXT}

        if [ ! -f ${TEMP_TXT} ]; then
        {
            echo "ERROR: unable to create temporary file '${TEMP_TXT}'"
        }
        else
        {
            #echo #echo "catting the file" #cat ${TEMP_TXT} #echo

            # Handle subshell variable calculation by writing to file
            # then reading to local variable
            cat ${TEMP_TXT} | while read line; do
            {
                file_count=$(( $file_count + 1 ))
                echo $file_count > file_count_tmp

                counter=$(( ${counter} + $(echo ${line} | cut -d ':' -f 2) ))
                echo $counter > counter_tmp
            }
            done
        }
        fi

        counter=$(( $(cat counter_tmp | cut -d ':' -f 1) ))
        file_count=$(( $(cat file_count_tmp | cut -d ':' -f 1) ))

        rm counter_tmp
        rm file_count_tmp

        echo "The number of files are ${file_count} and the number of matching lines are ${counter}"
        return 0
    }
    fi
}


#Call functions!
input_error_check
result=$?
if [ $result -ne 0 ] ; then
{
    echo "ERROR: Exiting now"
    exit 1
}
else
{
    initSubshellDataStore

    mainLoop
    result=$?
    if [ $result -ne 0 ] ; then
    {
        echo "ERROR: Exiting now"
        exit 1
    }
    fi

    cleanUp

    exit 0
}
fi
