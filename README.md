# bmmo-protocol-compiler

Yet Another Protobuf.  
It just my exercise for Flex/Bison. It should not be used in any production environment, however I will keep it as stable as I can.  
This compiler is written in pure C (I hate C++).

## Introduction

Imagine following circumstance, You have developed a new network application, you want its network protocol:

* Easy to read and debug.
* Do not use plain text.
* Use binary format.
* Just for a demo.

Congratulation! This project is very suit for you.

This project is mainly served for [Swung0x48/BallanceMMO](https://github.com/Swung0x48/BallanceMMO), because I want to create a standalone protocol parser to erase the dependency to original BallanceMMO project. Original BallanceMMO protocol become more and more complex. It is hard to be written by hand, so I create this compiler. Now, I can get effective code in any language via just writing a tiny protocol prototype file like Google Protobuf proto file. However, it is okay that you want to use this project in any other applications, because what your demo is facing  is usually the same as current BMMO's circumstance.

Considering Google Protobuf, this project is not suit for production environment, just like I said previously. If your application become more and more strong and ready for release, I suggest you switch to more strong serializer, for example, Google Protobuf. This project also provide the function that convert bp file used by this program to proto file used by Google Protobuf. It is convenient for migration.

## Features

### Supported Language

* Python
* C\# (on the way)
* C++ (on the way)
* Google Protobuf 3 (on the way)

### Supported Features

* Enum
* Struct
* Message
* Version Check
* Namespace

## Usage

Syntax: `bmmo_protocol_compiler [switches]`

### Basic Switches
* `-v --version`: Show compiler version.
* `-h --help`: Print help page.
* `-i --input PATH`: Specific input file in `PATH`.

### Output Switches
* `-p --python PATH`: Generate a Python code file in `PATH`.
* `-c --cs PATH`: Generate a C\# code file in `PATH`.
* `-d --cpp-header PATH`: Generate a C++ header file in `PATH`.
* `-s --cpp-source PATH`: Generate a C++ source file in `PATH`.
* `-t --proto PATH`: Generate a Proto3 file in `PATH`.

### Annotations

`-d` and `-s` both are usually used together to generate full C++ code file.  
If there are no any generation switches chosen, the compiler will only check the syntax of the input file.  
For the syntax of input file, aka `bp` file, please view `examples/example.bp` to know in detail.

## Compile

### Requirements

* Flex 2.5.6(at least)
* Bison 3.2(at least)
* GLib 2.6x(at least)

Tips: The version of required Flex and Bison could not be fetched from GnuWin32 project. I suggest you use MSYS2 or any other Linux distro, such as WSL etc, to get correct version of Flex and Bison. Also, required GLib can not be gotten from official GLib Windows binary release. I suggest you compile GLib by yourself or use any other pre-compiled GLib Windows binary release.

### Steps

* Modify `LibRef.props` and point macro to your GLib binary path.
* Open Visual Studio solution.
* Compile this project with your favorite architecture, such as Debug, Release, Win32 and x64.

CMake and Linux support are on the way. I do not have any plan about supporting any Apple platform.

## Related project:

* [Swung0x48/BallanceMMO](https://github.com/Swung0x48/BallanceMMO)
* [yyc12345/WhispersAbyss](https://github.com/yyc12345/WhispersAbyss)
* [yyc12345/BallanceStalker](https://code.blumia.cn/yyc12345/BallanceStalker)
