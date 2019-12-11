mkdir build
mkdir bin
cmake -S . -B ./build
mingw32-make -C ./build
cp ./build/src/c0-vm-cpp ./bin/