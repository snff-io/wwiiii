#!/bin/bash
set -xe

echo "Installing prerequistes for Debian/Ubuntu"

sudo apt-get update
sudo apt-get install -y \
    build-essential sudo git make libncurses5-dev \
    libcereal-dev libfmt-dev libgtest-dev libgmock-dev cmake libboost-all-dev \
    gcc g++ vim unzip zip \
    findutils iproute2 procps zlib1g-dev
