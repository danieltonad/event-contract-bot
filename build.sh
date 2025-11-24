#!/bin/bash

# Build output directory
BUILD_DIR="build"
mkdir -p $BUILD_DIR

OUTPUT="$BUILD_DIR/event-contract-bot"

# Paths
CPP_SRC="app/*.cpp src/*.cpp"
C_SRC="vendor/sqlite/sqlite3.c"
OBJ="$BUILD_DIR/sqlite3.o"
INCLUDE_DIRS="-I./src -I./vendor/sqlite"

# Compile sqlite.c if needed
if [ ! -f $OBJ ] || [ $C_SRC -nt $OBJ ]; then
    echo "Compiling sqlite.c..."
    gcc -c $C_SRC -o $OBJ -O2
    if [ $? -ne 0 ]; then
        echo "C compilation of sqlite.c failed."
        exit 1
    fi
else
    echo "sqlite.o is up-to-date."
fi

# Compile and link C++ sources
echo "Compiling C++ sources..."
g++ $CPP_SRC $OBJ -o $OUTPUT -std=c++17 $INCLUDE_DIRS
if [ $? -ne 0 ]; then
    echo "C++ compilation/linking failed."
    exit 1
fi

# Run executable
echo "Build successful. Running ./$OUTPUT ..."
./$OUTPUT
