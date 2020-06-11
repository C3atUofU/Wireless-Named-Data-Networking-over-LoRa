#!/bin/bash

# regex for getting the face id later
re="id=([0-9]+)"

# create the lora face and get the face id with the two cut commands
faceOut=$(nfdc face create lora://1)
echo $faceOut

# get the face id
[[ $faceOut =~ $re ]] && faceId="${BASH_REMATCH[1]}"

# create the route for the specified route (or /ndn by default)
if [ "$1" == "" ]
then
    # no route specified
    nfdc route add /ndn $faceId
else
    nfdc route add $1 $faceId
fi

