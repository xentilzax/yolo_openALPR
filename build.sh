#!/bin/bash
set -eux

cd ./darknet
make clean
make -j4
cp -f darknet.so ../lp_recognizer/
cd ../lp_recognizer
make clean
make
