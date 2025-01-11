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
    static char response[4096];
    char buffer[1024];

    sqlite3_stmt *stmt;
    const char *sql = "SELECT Inventory.Item AS ItemName, SUM(OrderItems.Amount) AS AmountSold FROM OrderItems JOIN Orders ON OrderItems.OrderId = Orders.Id JOIN Inventory ON OrderItems.ItemId = Inventory.Id WHERE DATE(Orders.OrderDate) = DATE('now') GROUP BY Inventory.Item ORDER BY Inventory.Item";
    
    int rc;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        snprintf(response, sizeof(response), "Internal Server Error \n");
        return response;
    }

    snprintf(response, sizeof(response), "Report: (Sold Items in the day)\n\nItem\tAmount\n");

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *item_name = sqlite3_column_text(stmt, 0);
        int amount_sold = sqlite3_column_int(stmt, 1);

        snprintf(buffer, sizeof(buffer), "%s\t%d\n", item_name, amount_sold);
        strncat(response, buffer, sizeof(response) - strlen(response) - 1);
    }

    if (rc != SQLITE_DONE) {
        snprintf(response, sizeof(response), "Failed to execute query: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return response;
}

#endif // GENERATE_REPORT_H