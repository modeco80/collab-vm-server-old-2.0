## collab-vm-server

The collab-vm-server powers CollabVM and other projects using the CollabVM Server.

**NOTE:** This repository is for a early experimental version of collab-vm-server. Please use [this](https://github.com/computernewb/collab-vm-server) repository for the time being,
as it is supported by the Computernewb development team until Feburary 2021.


## Building

First, clone this repository with `--recursive`.

## Linux

#### Debian/Ubuntu
`sudo apt install libboost-all-dev` (You may need to use vcpkg if your boost is too old to have Beast.)

#### Arch
`sudo pacman -S boost boost-libs`

#### Windows

Install vcpkg and run `vcpkg install boost:x64-windows` (example) or compile boost by hand.

You can either use the below instructions (for basically all platforms) or on Windows,
you can use the CMake tools Visual Studio provides with vcpkg integrations installed.

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=(Release|Debug) (On windows if you use vcpkg add -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake)
cmake --build .
```
