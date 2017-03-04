#!/bin/bash

make -C mkfs run "ARGS=-b 1024 -s 131072 fs_file"
mv mkfs/fs_file .
echo "------------";
make -C rtfs
mv rtfs/target/fs .
./fs
