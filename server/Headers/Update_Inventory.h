// Guards
#ifndef UPDATE_INVENTORY_H
#define UPDATE_INVENTORY_H

// Libraries
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

extern sqlite3 *db;

// Function Prototypes
int check_item_exists(const char* item);
int update_inventory(int itemId, int amount);
int create_inventory_item(const char* item, int amount);

// Main Function
const char* UpdateInventory(const char* item, int amount) {
    static char response[256];
    int check_item;
    pid_t pid = fork();
    fprintf(stderr, "Forked process with pid: %d\n", pid);

    if(pid < 0){
        snprintf(response, sizeof(response), "Fork failed\n");
        return response;
    } else if (pid == 0){
        check_item = check_item_exists(item);
        fprintf(stderr, "Child process with pid: %d\n", getpid());
        exit(check_item);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status)){
            check_item = WEXITSTATUS(status);
        } else {
            snprintf(response, sizeof(response), "Child process failed\n");
            return response;
        }

        if(check_item == 0){
            if (create_inventory_item(item, amount) == 0) {
                snprintf(response, sizeof(response), "Inventory item created successfully\n");
            } else {
                snprintf(response, sizeof(response), "Failed to create inventory item\n");
            }
        } else if(check_item == -1){
            snprintf(response, sizeof(response), "Failed to update inventory\n");
        } else {
            if (update_inventory(check_item, amount) == 0) {
                snprintf(response, sizeof(response), "Inventory updated successfully\n");
            } else {
                snprintf(response, sizeof(response), "Failed to update inventory\n");
            }
        }
    }
    
    return response;
}

// Function Definitions
int check_item_exists(const char* item){
    sqlite3_stmt *stmt;
    const char *sql = "SELECT Id FROM Inventory WHERE Item = ?";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, item, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    
    int item_id = 0;
    if(rc == SQLITE_ROW){
        item_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return item_id;
}

int update_inventory(int itemId, int amount){
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE Inventory SET Amount = Amount + ? WHERE Id = ?";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, amount);
    sqlite3_bind_int(stmt, 2, itemId);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        fprintf(stderr, "Failed to update inventory: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

int create_inventory_item(const char* item, int amount){
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Inventory (Item, Amount) VALUES (?, ?)";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, item, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, amount);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        fprintf(stderr, "Failed to create inventory item: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

#endif // UPDATE_INVENTORY_H