#!/bin/bash
exec > output.txt 2>&1

gcc -o lac lac.c util/utility.c util/variables.c bytecode.c executable.c

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

run_tests() {
    for file in "$1"/*.lac; do
        if [ -f "$file" ]; then
            echo "$file:"

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
