#!/bin/bash

response=$(curl -s -X GET http://localhost:8880/generate_report)
echo "Response from server:"
echo "$response"
