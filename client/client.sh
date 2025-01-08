#!/bin/bash

# Menu-driven system
echo "Welcome to the Retail Store Management System"
PS3="Please select an option: "

# Menu options
options=("Start Checkout Counters" "Update Inventory" "Generate Sales Report" "Process Order" "Stop Server" "Contunie Server" "Exit")

checkout_counters() {
   curl -X POST http://localhost:8880/checkout_counters -d '{"key":"value"}' -H "Content-Type: application/json"
}

update_inventory() {
    curl -X POST http://localhost:8880/update_inventory -d '{"key":"value"}' -H "Content-Type: application/json"
}

generate_report() {
    curl -X POST http://localhost:8880/generate_report -d '{"key":"value"}' -H "Content-Type: application/json"
}

process_orders() {
    curl -X POST http://localhost:8880/process_orders -d '{"key":"value"}' -H "Content-Type: application/json"
}

stop_server(){
    curl -X POST http://localhost:8880/stop_server -d '{"key":"value"}' -H "Content-Type: application/json"
}

continue_server(){
    curl -X POST http://localhost:8880/continue_server -d '{"key":"value"}' -H "Content-Type: application/json"
}

terminate_server(){
    curl -X POST http://localhost:8880/terminate -d '{"key":"value"}' -H "Content-Type: application/json"
}

# Loop until the user chooses to exit
while true; do
    echo
    select opt in "${options[@]}"; do
        case $REPLY in
            1)  checkout_counters
                break
                ;;

            2)  update_inventory
                break
                ;;

            3)  generate_report
                break
                ;;

            4)  process_orders
                break
                ;;

            5)  stop_server
                break
                ;;

            6)  continue_server
                break
                ;;

            7)  echo "Exiting the Retail Store Management System"
                terminate_server
                exit 0
                ;;

            *)  echo "Invalid option. Please try again."
                break
                ;;
        esac
    done
done
