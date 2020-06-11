#!/bin/bash

# peek for 10 seconds on the specified name ( or /ndn by default)
if [ "$1" == "" ]
then
    # no arg specified
    ndnpeek -pf -l 10000 /ndn
else
    ndnpeek -pf -l 10000 $1
fi

