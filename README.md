## collab-vm-server

The collab-vm-server powers CollabVM and other projects using the CollabVM Server.

**NOTE**: This repository is for a work-in-progress new version of collab-vm-server. Please use [this](https://github.com/computernewb/collab-vm-server) repository for the time being,
as it is supported by the Computernewb development team until Feburary 2021.


## Building

### Requirements
 - A *standards-compliant* C++17 compiler (Visual Studio 2019 on Windows, GCC 8.1+ on Linux, Clang 7 and up)
 - Git (to clone the repository)
 - CMake 3.2 and above

First, clone this repository with `--recursive` added to `git clone`. This will fetch all the repository dependencies for you.

## Linux

#### Debian/Ubuntu
`sudo apt install libboost-all-dev`

(You may need to use vcpkg if your boost version is too old to have a recent version of Boost.Beast.)

#### Arch
`sudo pacman -S boost boost-libs`

## Windows

Install vcpkg and run `vcpkg install boost:x64-windows` (example) or compile boost by hand.

on Windows, you can use the CMake tools Visual Studio provides with vcpkg integrations installed.


## CMake command line
Run the following commands:

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=(Release|Debug) (If you use vcpkg add -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake)
cmake --build . (or make -j$(nproc) on Linux)
```
