// Guards
#ifndef PROCESS_ORDERS_H
#define PROCESS_ORDERS_H

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

// Function Prototypes
int create_order(const char *client, int item_count);
int adjust_order_items(int Id, const char **items, const int *amounts, int item_count);
void recover_error(int ID);

// Main Function
const char* process_order(const char *client, const char **item_names, const int *item_values, int item_count) {
    static char response[256];
    int order_id;
    pid_t pid = fork();
    fprintf(stderr, "Forked process with pid: %d\n", pid);

    if(pid < 0){
        snprintf(response, sizeof(response), "Fork failed\n");
        return response;
    } else if (pid == 0){
        order_id = create_order(client, item_count);
        fprintf(stderr, "Child process with pid: %d\n", getpid());
        exit(order_id);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status)){
            order_id = WEXITSTATUS(status);
        } else {
            snprintf(response, sizeof(response), "Child process failed\n");
            return response;
        }

        if(order_id == -1){
            snprintf(response, sizeof(response), "Failed to create order\n");
        } else {
            if (adjust_order_items(order_id, item_names, item_values, item_count) == 0) {
                snprintf(response, sizeof(response), "Order processed successfully\n");
            } else {
                recover_error(order_id);
                snprintf(response, sizeof(response), "Failed to process order items.\n");
            }
        }
    }

    return response;
};

// Function Definitions
int create_order(const char *client, int item_count){
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Orders (ClientName, TotalAmount) VALUES (?, ?)";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, client, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, item_count);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        fprintf(stderr, "Failed to create order: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    int order_id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return order_id;
}

int adjust_order_items(int Id, const char **items, const int *amounts, int item_count){
    for (int i = 0; i < item_count; i++) {
        sqlite3_stmt *stmt;
        const char *query_select = "SELECT Id FROM Inventory WHERE Item = ?";
        const char *query_insert = "INSERT INTO OrderItems (OrderId, ItemId, Amount) VALUES (?, ?, ?)";
        const char *query_update = "UPDATE Inventory SET Amount = Amount - ? WHERE Id = ?";

        // Prepare the SELECT statement
        int rc = sqlite3_prepare_v2(db, query_select, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
            return -1;
        }

        // Bind the item name to the SELECT statement
        sqlite3_bind_text(stmt, 1, items[i], -1, SQLITE_STATIC);

        // Execute the SELECT statement
        rc = sqlite3_step(stmt);
        int item_id = 0;
        if (rc == SQLITE_ROW) {
            item_id = sqlite3_column_int(stmt, 0);
        } else {
            fprintf(stderr, "Item not found in inventory: %s\n", items[i]);
            sqlite3_finalize(stmt);
            return -1;
        }
        sqlite3_finalize(stmt);

        // Prepare the INSERT statement
        rc = sqlite3_prepare_v2(db, query_insert, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare INSERT statement: %s\n", sqlite3_errmsg(db));
            return -1;
        }

        // Bind the order ID, item ID, and amount to the INSERT statement
        sqlite3_bind_int(stmt, 1, Id);
        sqlite3_bind_int(stmt, 2, item_id);
        sqlite3_bind_int(stmt, 3, amounts[i]);

        // Execute the INSERT statement
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed to create order item: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return -1;
        }

        rc = sqlite3_prepare_v2(db, query_update, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare UPDATE statement: %s\n", sqlite3_errmsg(db));
            return -1;
        }

        // Bind the amount and item ID to the UPDATE statement
        sqlite3_bind_int(stmt, 1, amounts[i]);
        sqlite3_bind_int(stmt, 2, item_id);

        // Execute the UPDATE statement
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed to update inventory: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return -1;
        }

        sqlite3_finalize(stmt);
    }

    return 0;
}

// Error Recovery
typedef struct {
    int ID;
    const char *query;
    const char *error_message;
} ThreadData;

void *execute_query(void* arg){
    ThreadData *data = (ThreadData*)arg;
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db, data->query, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, data->ID);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "%s: %s\n", data->error_message, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return NULL;
}

void recover_error(int ID){
    pthread_t thread1, thread2;
    ThreadData data1 = {ID, "DELETE FROM Orders WHERE Id = ?", "Failed to delete order"};
    ThreadData data2 = {ID, "DELETE FROM OrderItems WHERE OrderId = ?", "Failed to delete order items"};

    // Create threads to execute the queries concurrently
    pthread_create(&thread1, NULL, execute_query, &data1);
    pthread_create(&thread2, NULL, execute_query, &data2);

    // Wait for both threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
}

#endif // PROCESS_ORDERS_H
