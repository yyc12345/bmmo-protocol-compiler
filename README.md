# bmmo-protocol-compiler

Yet Another Protobuf / Flatbuffers.  
It just my exercise for Flex/Bison. It should not be used in any production environment, however I will keep it as stable as I can.  
This compiler is written in pure C (I hate C++).

## Introduction

Imagine following circumstance, You have developed a new network application, you want its network protocol:

* Easy to read and debug.
* Do not use plain text.
* Use serialization friendly binary format.
* Just for a demo.

Congratulation! This project is very suit for you.

This project is originally served for [Swung0x48/BallanceMMO](https://github.com/Swung0x48/BallanceMMO), because I want to create a standalone protocol parser to erase the dependency to original BallanceMMO project. Original BallanceMMO protocol become more and more complex. It is hard to be written by hand, so I create this compiler. Now, I can get effective code in any language via just writing a tiny protocol prototype file like Google Flatbuffers fbs file. However, it is okay that you want to use this project in any other applications, because what your demo is facing is usually the same as current BMMO's circumstance.  
Now, because the protocol of BMMO no longer follow some crucial syntax. This project is not focus on BMMO protocol anymore. However I don't want to change its name. I just want keep its name to show where it comes from.

Considering Google Flatbuffers, this project is not suit for production environment, just like I said previously. If your application become more and more strong and ready for release, I suggest you switch to more strong serializer, for example, Google Flatbuffers. This project also provide the function that convert bp file used by this program to fbs file used by Google Flatbuffers. It is convenient for migration.

## Features

### Supported Language

* Python (at least 3.5.3 in theory. 3.7 higher suggested.)
* C\#
  - Legacy Mode (For Unity / Godot user. Low performance.): at least .Net Framework 4.6. `unsafe` switch required.
  - Modern Mode (High performance): at least .Net Core 2.1
* C++ (at least C++ 17. Optimized for C++ 20 and 23)
* Google Flatbuffers

### Supported Features

* Enum
* Struct
* Message
* Version Check
* Namespace

## CLI Usage

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
* `-b --flatbuffers PATH`: Generate a Flatbuffers file in `PATH`.

`-d` and `-s` both are usually used together to generate full C++ code file.  
If there are no any generation switches chosen, the compiler will only check the syntax of the input file.  

## Bp File Usage

Bp file is the protocol definition file accepted by this compiler. Read [Bp File Manual](docs/BpFile.md) to learn more.

## Generated Code Usage

For how to use generated code files, please read [Generated Code Manual](docs/GenCode.md) in detail.

For the developer or anyone else who want to know the structure of generated code in detail, read following articles (Chinese only. Translation is on the way).

* [General Generation](docs/GenFields_ZH.html): The general document to describe how we generate statement for different data structures.
* [Python Generation](docs/GenPython_ZH.md)
* [C\# Generation](docs/GenCSharp_ZH.md)
* [C++ Generation](docs/GenCpp_ZH.md)

## Compile

This compiler support Windows and Linux platform and use different compile steps. I do not have any plan about supporting any Apple platform.

### Project Constitution

* `bmmo-protocol-compiler`: The core compiler. All code are placed in `src`.
* `bpc_test`: The test of compiler. All code are placed in `tests`.

### Requirements

* Flex 2.5.6 (at least)
* Bison 3.2 (at least)
* GLib 2.7x (at least)
* Executable [adobe/bin2c](https://github.com/adobe/bin2c) (Commit `4300880a350679a808dc05bdc2840368f5c24d9a`)

Please make sure you have built `adobe/bin2c` and put it into system path, otherwise the compiler can not find a proper program to generate `snippets.h`.

### Windows Build

* Modify `LibRef.props` and point macro to your GLib binary path.
* Open Visual Studio solution.
* Compile this project with your favorite architecture, such as Debug, Release, Win32 and x64.

Tips: The version of required Flex and Bison could not be fetched from GnuWin32 project. I suggest you use MSYS2 or any other Linux distro, such as WSL etc, to get correct version of Flex and Bison. Also, required GLib can not be gotten from official GLib Windows binary release. I suggest you compile GLib by yourself or use any other pre-compiled GLib Windows binary release.

### Linux Build

Navigate to root directory of this project and execute following command.

```bash
mkdir out
cd out
cmake ..
make
```

Tips: Also works on Termux.

## Related project:

* [Swung0x48/BallanceMMO](https://github.com/Swung0x48/BallanceMMO)
* [yyc12345/WhispersAbyss](https://github.com/yyc12345/WhispersAbyss)
* [yyc12345/BallanceStalker](https://code.blumia.cn/yyc12345/BallanceStalker)
