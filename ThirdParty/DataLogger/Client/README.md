
Compiling the postflight client requires having `g++-16` installed (personnally version 16.0.1 20260315 experimental). It should work on later versions of `g++-16` but if it doesn't, try using that one.

The compilation command is the following :
```sh
g++-16 --std=c++26 main.cpp ../../../Application/Data/**.cpp ../../../Application/Data/**/*.cpp -o out -freflection -Wattributes -I../../../ -I../../SignalUtils/include/ -Wl,--demangle
```
