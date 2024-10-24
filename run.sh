#!/usr/bin/env bash

usage() {
    echo "Usage: $0 [--compile-only]"
    exit 1
}

COMPILE_ONLY=false

if [ "$1" == "--compile-only" ]; then
    COMPILE_ONLY=true
    shift  # Remove the first argument so that "$@" contains the remaining args
elif [ "$#" -gt 0 ]; then
    usage
fi

if ! gcc --version &> /dev/null
then
    echo "GCC does not appear to be installed."
    echo "Try 'sudo apt install gcc'"
    exit 1
fi

if ! cmake --version &> /dev/null
then
    echo "CMake does not appear to be installed."
    echo "Try 'sudo apt install cmake'"
    exit 1
fi

export CMAKE_BUILD_TYPE=Debug

if cmake -B build -S . && cmake --build build
then
    echo "Compilation succeeded."
    
    # Only execute the program if not in compile-only mode.
    if [ "$COMPILE_ONLY" = false ]; then
        # Execute the compiled program, passing any additional arguments.
        if ! ./build/wormwood "$@"
        then
            echo
            echo "The reactor failed catastrophically, causing an environmental disaster!"
            echo "If only they would have used better programming practices..."
            echo "... and a safer language than C ..."
        fi
    fi
else
    echo "wormwood.c failed to compile."
    exit 1
fi
