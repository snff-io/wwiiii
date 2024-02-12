#!/bin/sh

set -xe

cd $1

(
cd deps/cl345
make
)

rm -rf _build
mkdir _build
cd _build

cmake -S ../ -B ./  \
	-DCMAKE_INSTALL_PREFIX:PATH=/opt/wwiv \
	-DCMAKE_BUILD_TYPE:STRING=Debug
make

cd ..
