# fakeIRC – a simple IRC-like server to use with netcat

fakeIRC is a simple IRC-like server, implementing a very limited protocol to be used to show participants in a workshop how they can chat using simple tools like netcat.

## Requirements

  * Build:
    * CMake
    * a C++ compiler
    * Linux
  * Runtime:
    * Linux
    * a network to chat on

## Building

First of all, install the requirements.
Then, run the following in the source directory:

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Running

```bash
cd build/src
./fakeirc
```

The help can be printed as follows:

```bash
./fakeirc -h
```
