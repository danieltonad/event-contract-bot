#!/bin/bash

set -e

# Determine OS (Linux vs Windows-like)
case "$OSTYPE" in
  linux*)   OS="linux" ;;
  msys*)    OS="windows" ;;  # Git Bash, MSYS2
  cygwin*)  OS="windows" ;;
  win32*)   OS="windows" ;;
  *)        OS="unknown" ;;
esac

echo "Detected OS: $OS"

# Build output directory
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

# Select output name
if [ "$OS" = "windows" ]; then
    OUTPUT="$BUILD_DIR/event-contract-bot.exe"
else
    OUTPUT="$BUILD_DIR/event-contract-bot"
fi

ROOT_DIR=$(dirname "$0")

# Paths
CPP_SRC="$ROOT_DIR/app/*.cpp $ROOT_DIR/src/*.cpp"
C_SRC="$ROOT_DIR/vendor/sqlite/sqlite3.c"
OBJ="$BUILD_DIR/sqlite3.o"
INCLUDE_DIRS="-I./src -I./vendor/sqlite -I./vendor/httplib -I./vendor/json"

# Select compilers and platform libs
if [ "$OS" = "windows" ]; then
    C_COMPILER="x86_64-w64-mingw32-gcc"
    CPP_COMPILER="x86_64-w64-mingw32-g++"
    PLATFORM_LIBS="-lws2_32 -static"
    PLATFORM_DEFS="-D_WIN32_WINNT=0x0A00"
else
    C_COMPILER="gcc"
    CPP_COMPILER="g++"
    PLATFORM_LIBS=""            # Linux doesn't need ws2_32
    PLATFORM_DEFS=""
fi

echo "Using C compiler:   $C_COMPILER"
echo "Using C++ compiler: $CPP_COMPILER"
echo "Output:             $OUTPUT"

# Compile sqlite3.c
if [ ! -f "$OBJ" ] || [ "$C_SRC" -nt "$OBJ" ]; then
    echo "Compiling sqlite3.c..."
    $C_COMPILER -c "$C_SRC" -o "$OBJ" -O2
fi

# Compile + link
echo "Compiling C++ sources..."
$CPP_COMPILER $CPP_SRC -o "$OUTPUT" $OBJ -std=c++17 \
    $INCLUDE_DIRS $PLATFORM_DEFS $PLATFORM_LIBS -pthread

echo "Build successful."

# Execution step (safe cross-platform)
echo "Running: $OUTPUT"
if [ "$OS" = "windows" ]; then
    chmod +x "$OUTPUT"  # ensure Git Bash can run it
    "$OUTPUT"
else
    "$OUTPUT"
fi
