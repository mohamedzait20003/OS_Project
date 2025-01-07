#!/bin/bash

# Menu-driven system
echo "Welcome to the Retail Store Management System"
PS3="Please select an option: "

# Menu options
options=("Start Checkout Counters" "Update Inventory" "Generate Sales Report" "Process Order" "Exit")

# Loop until the user chooses to exit
while true; do
    echo
    select opt in "${options[@]}"; do
        case $REPLY in
            1)  gcc checkout_counters.c -o checkout_counters -lpthread
                ./checkout_counters # Call the compiled C program for checkout counters
                break
                ;;
            2)
                gcc update_inventory.c -o update_inventory
                ./update_inventory # Call the compiled C program for inventory updates
                break
                ;;
            3)
                gcc generate_report.c -o generate_report
                ./generate_report # Call the compiled C program for sales reporting
                break
                ;;
            4)
                gcc process_orders.c -o process_orders -lpthread
                ./process_orders # Call the compiled C program for order processing
                break
                ;;
            5)
                echo "Exiting the system. Goodbye!"
                exit 0
                ;;
            *)
                echo "Invalid option. Please try again."
                break
                ;;
        esac
    done
done
