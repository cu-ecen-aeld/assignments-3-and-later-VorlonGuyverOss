#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

set -e
set -u

NUMFILES=10
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data
#username=$(cat conf/username.txt)


if [ -d /etc/finder-app/conf ] ; then
{
    echo "Finder App Config file username.txt found!"
    echo "Found in: /etc/finder-app/conf/username.txt"
    ls /etc/finder-app/conf
    username=$(cat /etc/finder-app/conf/username.txt)
    echo "DEBUG CODE - FGREEN: username = ${username}"
}
elif [ -d conf ] ; then
{
    echo "Finder App Config file username.txt found!"
    echo "Found in: conf/username.txt"
    ls conf/
    username=$(cat conf/username.txt)
    echo "DEBUG CODE - FGREEN: username = ${username}"

}
else
{
    username="ERROR: file 'username.txt' not found"
}
fi

if [ $# -lt 2 ]
then
    echo "Using default value ${WRITESTR} for string to write"
    if [ $# -lt 1 ]
    then
        echo "Using default value ${NUMFILES} for number of files to write"
    else
        NUMFILES=$1
    fi
else
    NUMFILES=$1
    WRITESTR=$2
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

rm -rf "${WRITEDIR}"
mkdir -p "$WRITEDIR"

#The WRITEDIR is in quotes because if the directory path consists of spaces, then variable substitution will consider it as multiple argument.
#The quotes signify that the entire string in WRITEDIR is a single string.
#This issue can also be resolved by using double square brackets i.e [[ ]] instead of using quotes.
if [ -d "$WRITEDIR" ]
then
    echo "$WRITEDIR created"
else
    exit 1
fi

#echo "Removing the old writer utility and compiling as a native application"
#make clean
#make

echo "Result of command '$ which ls' is: " $(which ls)
which ls
SHELL_RETURNED=333
SHELL_RETURNED=$(echo $?)
echo "SHELL_RETURNED = ${SHELL_RETURNED}"


echo "Result of command '$ which writer' is: " $(which writer)
echo $(which writer)
#SHELL_RETURNED=$($(which writer ; echo $?))
SHELL_RETURNED=$(echo $(which ls ; echo $?))
echo ${?}
#SHELL_RETURNED=$(which writer)
echo "SHELL_RETURNED = ${SHELL_RETURNED}"

if  $(! which writer > /dev/null) ; then
    # Empty path
    SHELL_RETURNED=0
else
    SHELL_RETURNED=1
fi

for i in $( seq 1 $NUMFILES) ; do
{
#   ./writer.sh "$WRITEDIR/${username}$i.txt" "$WRITESTR"
#   ./writer "$WRITEDIR/${username}$i.txt" "$WRITESTR"

echo "FGREEN_0 = ${SHELL_RETURNED}"
    if [ ! ${SHELL_RETURNED} -eq 0 ] ; then
#    if [ ${SHELL_RETURNED} == "" ] ; then
    {
        echo ""$WRITEDIR/${username}$i.txt" "$WRITESTR""
        writer "$WRITEDIR/${username}$i.txt" "$WRITESTR"
    }
    elif [ ${SHELL_RETURNED} -eq 0 ] ; then
#    else
    {
        echo ""$WRITEDIR/${username}$i.txt" "$WRITESTR""
        ./writer "$WRITEDIR/${username}$i.txt" "$WRITESTR"
    }
    else
    {
        echo "ERROR: Outside of expected range SHELL_RETURNED = ${SHELL_RETURNED}"
    }
    fi
}
done

OUTPUTSTRING=$(./finder.sh "$WRITEDIR" "$WRITESTR")

echo "FGREEN_1 = ${SHELL_RETURNED}"

# Modify your finder-test.sh script to write a file with output of the finder
# command to /tmp/assignment-4-result.txt
    if [ ! ${SHELL_RETURNED} -eq 0 ] ; then
    {
        echo ""/tmp/assignment-4-result.txt" "${OUTPUTSTRING}""
        writer "/tmp/assignment-4-result.txt" "${OUTPUTSTRING}"
    }
    elif [ ${SHELL_RETURNED} -eq 0 ] ; then
#    else
    {
        echo ""/tmp/assignment-4-result.txt" "${OUTPUTSTRING}""
        ./writer "/tmp/assignment-4-result.txt" "${OUTPUTSTRING}"
    }
    else
    {
        echo "ERROR: Outside of expected range SHELL_RETURNED = ${SHELL_RETURNED}"
    }
    fi

set +e
echo ${OUTPUTSTRING} | grep "${MATCHSTR}"
if [ $? -eq 0 ]; then
    echo "success"
    exit 0
else
    echo "failed: expected  ${MATCHSTR} in ${OUTPUTSTRING} but instead found"
    exit 1
fi
