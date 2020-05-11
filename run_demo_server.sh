#!/bin/bash

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug && make all -j6 && mv server ../demo && cd ../demo && ./server 5188

