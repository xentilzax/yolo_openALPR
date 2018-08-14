#!/bin/bash
set -eux

cd ./darknet
make clean
make -j4
cd ../lprecognizer
make clean
make
cd ..

rm -rf build
mkdir -p build
cp -r ./darknet/weights ./build/
cp -r ./darknet/cfg ./build/
cp -r ./lprecognizer/cfg ./build/
cp -r ./lprecognizer/register_service.sh ./build/
cp -r ./lprecognizer/unregister_service.sh ./build/

cp ./darknet/nn.so ./build/
cp ./lprecognizer/lprecognizer ./build/
