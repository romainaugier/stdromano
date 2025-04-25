#!/bin/bash

if ! command -v clang-format &> /dev/null; then
    echo "Cannot find clang-format, exiting"
    exit 1
else
    echo "Found clang-format"
fi

shopt -s globstar

ROOT_DIR="$(dirname "$0")/.."

DIRS=(
    "$ROOT_DIR/src"
    "$ROOT_DIR/include/stdromano"
    "$ROOT_DIR/tests"
)

for dir in "${DIRS[@]}"; do
    if [[ -d "$dir" ]]; then
        echo "Processing directory: $dir"
        
        for file in "$dir"/**/*; do
            if [[ -f "$file" ]]; then
                case "$file" in
                    *.cpp|*.cc|*.c|*.h|*.hpp)
                        echo "Formatting $file"
                        clang-format -i -style=file "$file"
                        ;;
                esac
            fi
        done
    else
        echo "Directory $dir does not exist, skipping"
    fi
done

exit 0
