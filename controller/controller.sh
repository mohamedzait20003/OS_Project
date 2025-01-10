#!/bin/bash
SERVER_PIDS=$(pgrep -f "./server")

# Check if the server is running
if [ -z "$SERVER_PIDS" ]; then
    echo "Server is not running."
else
    gcc -o controller main.c

    if [ $? -ne 0 ]; then
        echo "Failed to compile controller.c"
        exit 1
    fi
    ./controller $SERVER_PIDS
fi