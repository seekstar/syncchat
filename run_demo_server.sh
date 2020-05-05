#!/bin/bash

mkdir -p build
cd build
cmake .. && make all && mv server ../demo && cd ../demo && ./server 5188

