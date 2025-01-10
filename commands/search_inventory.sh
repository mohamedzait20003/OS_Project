#!/bin/bash

#Checking if the keyword is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <keyword>"
    exit 1
fi

keyword=$1

#Fetching inventory and filtering with grep
curl -s -X GET http://localhost:8880/view_inventory | grep --color=auto -i "$keyword"
