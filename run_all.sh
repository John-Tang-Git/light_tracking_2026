#!/usr/bin/env bash

cmake -B build
cmake --build build

# Run all testcases. 
# You can comment some lines to disable the run of specific examples.
mkdir -p output
build/PA1 testcases/scene08_ultra_sphere.txt output/scene08.bmp
build/PA1 testcases/scene09_simple_sphere.txt output/scene09.bmp
