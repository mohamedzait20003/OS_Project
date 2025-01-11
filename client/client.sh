#!/bin/bash

# Menu-driven system
echo "Welcome to the Retail Store Management System"
PS3="Please select an option: "

# Menu options
options=("View Inventory" "Update Inventory" "Process Order" "Customized Commands" "Exit")

view_inventory() {
    response=$(curl -s -X GET http://localhost:8880/view_inventory)
    echo "Response from server:"
    echo "$response"
}

update_inventory() {
    read -p "Enter Item Name: " item
    read -p "Enter Amount: " amount
    response=$(curl -s -X POST http://localhost:8880/update_inventory -d "{\"Item\":\"$item\", \"Amount\":$amount}" -H "Content-Type: application/json")
    echo "Response from server:"
    echo "$response"
}

process_orders() {
    read -p "Enter Client Name: " client
    read -p "Enter Number of Items: " num_items
    items=()

    for ((i=1; i<=num_items; i++)); do
        read -p "Enter Item $i Name: " item_name
        read -p "Enter Item $i Amount: " item_amount
        items+=("{\"Item\":\"$item_name\", \"Value\":$item_amount}")
    done

    items_json=$(printf ",%s" "${items[@]}")
    items_json="[${items_json:1}]"
    response=$(curl -s -X POST http://localhost:8880/process_orders -d "{\"Client\":\"$client\", \"Items\":$items_json}" -H "Content-Type: application/json")
    echo "Response from server:"
    echo "$response"
}

generate_report() {
     /usr/local/bin/generate_report
}

search_inventory() {
     read -p "Enter keyword to search in inventory: " keyword
    /usr/local/bin/search_inventory "$keyword"
}


customized_commands() {
    echo "Welcome to the Customized Commands Menu"
    while true; do
        echo "Available commands: generate_report and search_inventory"
        read -p "Enter a command or type 'exit' to return to the main menu: " user_command

        # handle the user input
        case $user_command in
            generate_report)
                generate_report
                ;;
            search_inventory)
                search_inventory
                ;;
            exit)
                echo "Returning to main menu..."
                return
                ;;
            *)
                echo "Invalid command. Please try again."
                ;;
        esac
    done
}


# Loop until the user chooses to exit
while true; do
    echo
    select opt in "${options[@]}"; do
        case $REPLY in
            1)  view_inventory
                break
                ;;
            2)  update_inventory
                break
                ;;
            3)  process_orders
                break
                ;;
            4)  customized_commands
                break
                ;;
            5)  echo "Exiting..."
                exit 0
                ;;
            *)  echo "Invalid option. Please try again."
                ;;
        esac
    done
done