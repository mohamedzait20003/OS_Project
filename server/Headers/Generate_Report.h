// Guards
#ifndef GENERATE_REPORT_H
#define GENERATE_REPORT_H

// Libraries
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

extern sqlite3 *db;

const char* generate_report() {
    static char response[256];

    sqlite3_stmt *stmt;
    const char *sql = "SELECT FROM OrderItems WHERE OrderId = ? IN ";

    

    return response;
}

#endif // GENERATE_REPORT_H