#!/bin/sh

# Write a shell script finder-app/writer.sh as described below
#
# Accepts the following arguments: the first argument is a full path to a
# file (including filename) on the filesystem, referred to below as
# writefile; the second argument is a text string which will be written
# within this file, referred to below as writestr
#
# Exits with value 1 error and print statements if any of the arguments
# above were not specified
#
# Creates a new file with name and path writefile with content
# writestr, overwriting any existing file and creating the path if it
# doesn’t exist. Exits with value 1 and error print statement if the file
# could not be created.
#
# Example:
#
# writer.sh /tmp/aesd/assignment1/sample.txt ios
#
# Creates file:
#
# /tmp/aesd/assignment1/sample.txt
#
# With content:
#
# ios

PWD=$(pwd)

TEXT_STRING='ios'

# Handle script input
TOTAL_SCRIPT_INPUT=$#
FIRST_INPUT=$1
SECOND_INPUT=$2



input_error_check()
{
    if [ ${TOTAL_SCRIPT_INPUT} -ne 2 ] ; then
    {
        #FGREEN
        echo "ERROR: the script input requires two strings"
        echo "First input is the directory"
        echo "Second input is the string to search for"
        echo "EXAMPLE: writer.sh /tmp/aesd/assignment1/sample.txt ios"
        echo "EXAMPLE: writer.sh ${PWD} \"the script input requires two strings\""
        echo "EXAMPLE: writer.sh then returns actual results such as:"
        echo
        echo "\<local_dir\> \$ writer.sh ${FIRST_INPUT} ${PWD} 'you wrote me'"
        echo


        return 1
    }
    fi

    return 0
}


writeFile()
{
    tmp_dir=''
    tmp_rev_tmp=''
    file_to_write=''

    echo ${FIRST_INPUT} > directory_tmp
    echo '' > rev_tmp
    echo '' > file_to_write_tmp

    tmp_dir=$(cat directory_tmp)


    # Reverse the input directory with filename
    (cat directory_tmp | rev | awk -F '/' '{print $1}' > rev_tmp)
    tmp_rev_tmp=$(cat rev_tmp)

    # Correct the direction of the file to write
    (cat rev_tmp | rev > file_to_write_tmp)

    # Cut the file to write out and then reverse back to a directory
    (cat directory_tmp | rev > rev_tmp)
    (cat rev_tmp | sed 's/'"${tmp_rev_tmp}"'//g' | rev > directory_tmp)
    tmp_dir=$(cat directory_tmp)

    # Maker directory
    mkdir -p ${tmp_dir}
    result=$?
    if [ $result -ne 0 ] ; then
    {
        echo "ERROR: Making the directory; exiting now"
        return 1
    }
    fi

    # Touch file in said directory
    file_to_write=$(cat file_to_write_tmp)
    touch ${tmp_dir}${file_to_write}
    result=$?
    if [ $result -ne 0 ] ; then
    {
        echo "ERROR: Touching the file; exiting now"
        return 1
    }
    fi

    # Write argument ${SECOND_INPUT} to the file
    echo ${SECOND_INPUT} > ${tmp_dir}${file_to_write}
    result=$?
    if [ $result -ne 0 ] ; then
    {
        echo "ERROR: Inserting information into file:exiting now"
        return 1
    }
    fi

    return 0
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
    writeFile

    result=$?
    if [ $result -ne 0 ] ; then
    {
        rm directory_tmp rev_tmp file_to_write_tmp
        echo "ERROR: Exiting now"
        exit 1
    }
    fi

    rm directory_tmp rev_tmp file_to_write_tmp

    exit 0
}
fi
