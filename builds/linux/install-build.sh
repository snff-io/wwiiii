#!/bin/bash

BINARIES='
bbs/bbs
wwivd/wwivd
network/network
network1/network1
network2/network2
network3/network3
networkb/networkb
networkc/networkc
networkf/networkf
wwivutil/wwivutil
wwivconfig/wwivconfig
wwivfsed/wwivfsed
release/*
'

set -xe

# Navigate to the source directory
wwiv_source_root=$(find / -name "WWIV_SOURCE_ROOT" 2>/dev/null | head -n 1) 
wwiv_source_root="$(dirname "$(readlink -f "$wwiv_source_root")")"
cd "$wwiv_source_root/build"

sudo mkdir -p /opt/wwiv
sudo install -m 755 $BINARIES /opt/wwiv

