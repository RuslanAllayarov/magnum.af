#!/bin/bash
# this scripts tests whether there is a main*.cpp file in /temp_main
# if so, it is moved to magnum.af/

# calling this scripts's directory
cd "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# relative path to magnum.af/
magafdir=../..


if [ -e $magafdir/temp_main/main*.cpp ]
then
    if [ -e $magafdir/src/main*.cpp ]
    then
        echo "Error in moving temp_main to /src, main in /src still exists!"
        exit 1
    fi
    echo "Moving temp_main to /src"
    mv $magafdir/temp_main/main*.cpp $magafdir/src
    rmdir $magafdir/temp_main
    exit 0
fi