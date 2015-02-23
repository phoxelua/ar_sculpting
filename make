#!/bin/bash

if [ $1 = "clean" ]; then
	echo "Cleaning..."
	cd src/ && rm *.class && make clean -f Makefile.man
else
	echo "Building..."
	cd src/ && javac -classpath "/usr/lib/Leap/LeapJava.jar:json-simple-1.1.1.jar" Sample.java && make -f Makefile.man
fi

