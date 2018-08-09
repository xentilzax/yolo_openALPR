#!/bin/bash
set -eux

cd ./darknet
make clean
make -j4
cp -f darknet.so ../lp_recognizer/
cd ../lp_recognizer
make clean
make
cd ..

rm -rf build
mkdir -p build
cp -r ./darknet/weights ./build/
cp -r ./darknet/cfg ./build/
cp -r ./lp_recognizer/cfg ./build/
cp ./lp_recognizer/darknet.so ./build/
cp ./lp_recognizer/lprecognazer ./build/
