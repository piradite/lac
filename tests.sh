#!/bin/bash

gcc -o lac lac.c util/utility.c bytecode.c executable.c

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

run_tests() {
    for file in "$1"/*.lac; do
        if [ -f "$file" ]; then
            file_name=$(basename "$file" .lac)
            echo "$file_name:"

            ./lac main "$file"
            ./main
            
            echo
        fi
    done

    for dir in "$1"/*; do
        if [ -d "$dir" ]; then
            run_tests "$dir"
        fi
    done
}

run_tests "tests"

rm -f lac main

echo "All tests executed."
