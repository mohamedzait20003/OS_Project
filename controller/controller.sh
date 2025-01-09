#!/bin/bash

SERVER_PID=$(pgrep -f "./server")

# Check if the server is running
if [ -z "$SERVER_PID" ]; then
    echo "Server is not running."
else
    
fi