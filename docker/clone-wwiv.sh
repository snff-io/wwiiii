#!/bin/sh

set -xe

srcdir=$1
branch=$2
refspec=$3

mkdir -p $srcdir
cd $srcdir
git clone --recurse-submodules -b $branch https://github.com/snff-io/wwiv/ wwiv
cd wwiv
git checkout $refspec
