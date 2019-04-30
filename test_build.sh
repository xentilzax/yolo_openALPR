cd lprecognizer
make -f Makefile_test clean
make -f Makefile_test
cp *.so ../build/
cp test ../build/
cd ..
