#ifndef DATABASE_H
#define DATABASE_H

#include <xlsxwriter.h>
#include <stdio.h>

#define EXCEL_FILENAME "Database.xlsx"

void write_excel_file(const char *text, double number) {
    // Create a new workbook
    lxw_workbook  *workbook  = workbook_new(EXCEL_FILENAME);
    // Add a worksheet
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, NULL);

    // Write some data to the worksheet
    worksheet_write_string(worksheet, 0, 0, text, NULL);
    worksheet_write_number(worksheet, 1, 0, number, NULL);

    // Close the workbook
    workbook_close(workbook);
}

void read_excel_file() {
    // Reading from an Excel file is more complex and typically requires a different library.
    // For simplicity, this function will just print a message.
    printf("Reading from Excel file: %s\n", EXCEL_FILENAME);
    // Implement reading logic here using a suitable library like libxls or libxl.
}

#endif // DATABASE_H