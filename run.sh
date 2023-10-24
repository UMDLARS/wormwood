#!/usr/bin/env bash

if gcc -O0 wormwood.c -o wormwood &> /dev/null
then

	if ! ./wormwood
	then
		echo
		echo
		echo "The reactor failed catastrophically, causing an environmental disaster!"
		echo "If only they would have used better programming practices..."
		echo "... and a safer language than C ..."
	fi

fi

