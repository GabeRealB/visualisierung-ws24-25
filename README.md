# Visualization 24/25

This repository contains the exercises and solutions for the scientific
visualization part of the visualization course.

## Building

The build instructions are platform dependent, we officially support Windows,
Linux (Ubuntu) and MacOS. Other Linux distributions are likely to work, but
may require different development packages.

### Windows

Prerequisites:

- [CMake](https://cmake.org/)
- A C++ compiler with C++-20 support, e.g.
[Microsoft Visual C++](https://visualstudio.microsoft.com/). Other compilers,
like [Clang](https://clang.llvm.org/) will also work.

Build instructions:

1. Install CMake and a suitable C++ compiler.

2. From the root directory of this repository, run CMake and create a `build` folder.

    ```sh
    cmake -B build
    ```

3. Compile the application.

    ```sh
    cmake -build build
    ```

4. Find the application `app.exe` in the `build` folder (or a subdirectory) and execute it.
Make sure that a copy of the `resources` folder is present in the same location as the application
binary, otherwise copy it from the root of the project.

### Ubuntu

Prerequisites:

- [CMake](https://cmake.org/)
- A C++ compiler with C++-20 support, e.g. [GCC](https://gcc.gnu.org/). Other compilers, like [Clang](https://clang.llvm.org/) will also work.
- Package libxrandr-dev
- Package libxinerama-dev
- Package libxcursor-dev
- Package libxi-dev
- Package xorg-dev

Build instructions:

1. Install the prerequisites:

    ```sh
    sudo apt update
    sudo apt install build-essential libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev xorg-dev
    ```

2. From the root directory of this repository, run CMake and create a `build` folder.

    ```sh
    cmake -B build
    ```

3. Compile the application.

    ```sh
    cmake -build build
    ```

4. Find the application `app` in the `build` folder (or a subdirectory) and execute it.
Make sure that a copy of the `resources` folder is present in the same location as the application
binary, otherwise copy it from the root of the project.

### MacOS

Prerequisites:

- [CMake](https://cmake.org/)
- A C++ compiler with C++-20 support, e.g. [Xcode](https://developer.apple.com/xcode/).
Other compilers, like [Clang](https://clang.llvm.org/) will also work.

Build instructions:

1. Install the compiler and CMake. If you use Xcode, you have to start Xcode once, to
accept the licensing agreement. As an alternative, you can install the Xcode
Command Line Tools from the command prompt.

    ```sh
    xcode-select --install
    ```

2. From the root directory of this repository, run CMake and create a `build` folder.

    ```sh
    cmake -B build
    ```

3. Compile the application.

    ```sh
    cmake -build build
    ```

4. Find the application `app` in the `build` folder (or a subdirectory) and execute it.
Make sure that a copy of the `resources` folder is present in the same location as the application
binary, otherwise copy it from the root of the project.

### Release Build

The project can optionally be built in release mode, which enables many optimizations, making the
application run faster.

To enable the release mode, replace the following command

```sh
cmake -B build
```

with

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

and build the project as usual.
