#!/bin/sh

set -xe

cd $1

git config --global user.name wwiv
git config --global user.email wwiv
git am $2/*
