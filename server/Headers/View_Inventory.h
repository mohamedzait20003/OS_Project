// Guards
#ifndef VIEW_INVENTORY_H
#define VIEW_INVENTORY_H

// Libraries
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>

extern sqlite3 *db;

const char* view_inventory() {
    static char response[4096];
    char buffer[1024];
    sqlite3_stmt *stmt;

    const char *sql = "SELECT Item, Amount FROM Inventory";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        snprintf(response, sizeof(response), "Failed to fetch inventory data: %s\n", sqlite3_errmsg(db));
        return response;
    }

    snprintf(response, sizeof(response), "Inventory:\nId\tItem\tAmount\n");

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *item = sqlite3_column_text(stmt, 1);
        int amount = sqlite3_column_int(stmt, 2);

        snprintf(buffer, sizeof(buffer), "%d\t%s\t%d\n", id, item, amount);
        strncat(response, buffer, sizeof(response) - strlen(response) - 1);
    }

    sqlite3_finalize(stmt);
    return response;
}

#endif // VIEW_INVENTORY_H