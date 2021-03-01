#!/bin/bash

CC=${CC:-clang}
PLATFORM=linux-x11
USE_LOCAL_GLFW=false

SRC=$(find src/ -name "*.c")
FLAGS=""

# glad
SRC+=" $(find vendor/glad/src/ -name "*.c")"
FLAGS+=" -Ivendor/glad/include"

# mathc
SRC+=" $(find vendor/mathc/ -name "*.c")"
FLAGS+=" -Ivendor/mathc"

# glfw
if [[ $USE_LOCAL_GLFW = true ]]; then
	FLAGS+=" -lglfw"
else
	# We patch some of the source files to suppress -Wpedantic errors.
	SRC+="
		vendor/glfw/src/context.c
		vendor/glfw/patches/init.c
		vendor/glfw/patches/input.c
		vendor/glfw/patches/monitor.c
		vendor/glfw/src/vulkan.c
		vendor/glfw/patches/window.c
	";
	# for $file in 'context.c init.c input.c monitor.c vulkan.c window.c'; do
	# 	SRC+=" vendor/glfw/src/$file";
	# done;
	FLAGS+=" -Ivendor/glfw/include"

	if [[ $PLATFORM == linux-x11 ]]; then
		SRC+="
			vendor/glfw/src/x11_init.c
			vendor/glfw/src/x11_monitor.c
			vendor/glfw/src/x11_window.c
			vendor/glfw/src/xkb_unicode.c
			vendor/glfw/src/posix_time.c
			vendor/glfw/src/posix_thread.c
			vendor/glfw/patches/glx_context.c
			vendor/glfw/src/egl_context.c
			vendor/glfw/src/osmesa_context.c
			vendor/glfw/src/linux_joystick.c
		";
		FLAGS+=" -D_GLFW_X11=1 -lX11"
	fi
fi

# ASan
# FLAGS="$FLAGS -fsanitize=address -fno-omit-frame-pointer"

echo "Compiling"
$CC -g -std=gnu11 -Wall -pedantic -lm -ldl -pthread $FLAGS ${SRC[*]} -o magic || exit
