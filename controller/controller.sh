#!/bin/bash
SERVER_PIDS=$(pgrep -f "./server")

# Check if the server is running
if [ -z "$SERVER_PIDS" ]; then
    echo "Server is not running."
else
    
fi