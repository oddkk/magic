#!/bin/bash

CC=${CC:-clang}

SRC=$(find src/ -name "*.c")
FLAGS=""

SRC+=" $(find vendor/glad/src/ -name "*.c")"
FLAGS+=" -Ivendor/glad/include"

SRC+=" $(find vendor/mathc/ -name "*.c")"
FLAGS+=" -Ivendor/mathc"

# ASan
# FLAGS="$FLAGS -fsanitize=address -fno-omit-frame-pointer"

echo "Compiling"
echo $CC -g -std=gnu11 -Wall -pedantic -lm -ldl -lglfw -pthread $FLAGS ${SRC[*]} -o magic
$CC -g -std=gnu11 -Wall -pedantic -lm -ldl -lglfw -pthread $FLAGS ${SRC[*]} -o magic || exit
