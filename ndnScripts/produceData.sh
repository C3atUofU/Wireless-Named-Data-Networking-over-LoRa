#!/bin/bash

# produce test data with a freshness of 10 seconds on the specified name ( or /ndn by default)
if [ "$1" == "" ]
then
    # no arg specified
    echo "test" | ndnpoke -x 10000 /ndn
else
    echo "test" | ndnpoke -x 10000 $1
fi

