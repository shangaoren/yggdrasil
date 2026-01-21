#!/bin/zsh

cd test/build
rm -rf ./*
cmake -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ ..
cmake --build .
./yggdrasil_test