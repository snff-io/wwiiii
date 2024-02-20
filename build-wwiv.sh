#!/bin/sh


cd $1

(
cd deps/cl345
make
)

cmake \
	-DCMAKE_INSTALL_PREFIX:PATH=/opt/wwiv \
	-DCMAKE_BUILD_TYPE:STRING=Debug
make
