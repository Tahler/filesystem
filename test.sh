#!/bin/bash

make -C mkfs run "ARGS=-b 1024 -s 131072 fs_file" > /dev/null
mv mkfs/fs_file .
echo "------------";
make -C rtfs > /dev/null
mv rtfs/target/fs .
./fs
make -C mkfs clean > /dev/null
make -C rtfs clean > /dev/null
rm fs
rm fs_file
