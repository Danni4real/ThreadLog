ThreadLog is a tool that can:

1. track threads' executing trajectory;
1. different thread use different color to print track and log;
2. use ++ when thread depth increased;
3. use -- when thread depth decreased;

mkdir build
cd build
cmake ..
make

./test
