#!/usr/bin/env bash

if ! gcc --version &> /dev/null
then
	echo "GCC does not appear to be installed."
	echo "Try 'sudo apt install gcc'"
	exit 0
fi

if ! cmake --version &> /dev/null
then
	echo "CMake does not appear to be installed."
	echo "Try 'sudo apt install cmake'"
	exit 0
fi

if CMAKE_BUILD_TYPE=Debug cmake -B build -S . && cmake --build ./build
then

	if ! ./build/wormwood $1
	then
		echo
		echo
		echo "The reactor failed catastrophically, causing an environmental disaster!"
		echo "If only they would have used better programming practices..."
		echo "... and a safer language than C ..."
	fi

else
	echo "wormwood.c failed to compile."
	exit 1
fi

