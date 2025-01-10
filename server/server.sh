#!/bin/bash

echo "Starting the daemon process"
nohup bash -c 'while true; do sleep 1; done' > /dev/null 2>&1 &

echo "Compiling the C program"
gcc -o server main.c -lmicrohttpd -lcjson -lpthread -lsqlite3
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

echo "Running the C program"
./server
if [ $? -ne 0 ]; then
    echo "Server failed to start"
    exit 1
fi
