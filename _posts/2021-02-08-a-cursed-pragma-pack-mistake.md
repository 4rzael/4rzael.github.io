---
layout: post
title: "A cursed #pragma pack mistake"
date: 2021-02-08 16:40 +0000
tags: [c++, bug]
---

## Using structures to represent byte-data

There are many cases in c/c++ when you want to be able to read or write data in an exact memory format. Either to write drivers in embedded, do some *basic* **[1]** serialization / deserialization, etc...

A very crude way to do so is by using a byte array which you manually fill. However, this is very cumbersome and prone to error.

A more convenient way to so is to use structs. For example (taken from the [related snippet](code/a-cursed-pragma-pack-mistake)):
```cpp
struct Frame {
  enum class Type : uint8_t {
    REQUEST = 0,
    RESPONSE = 1
  } type;

  uint16_t length;
  char content[32];
};
```

## Padding

This structure has an issue, though: `sizeof(Frame)` equals `36`, instead of the expected `35`.

This is due to the fact that the compiler is allowed to add padding bytes inside the structure so that variables end up aligned in memory, leading to very fast access by the CPU (less instructions).

In this case, the compiler added a byte between `type` and `length`

However, when building a structure for serialization, we absolutely want to avoid this behaviour, and thus tell the compiler to *pack* the structure.

## Packing the structure

So, how do we tell the compiler to pack the structure ? That's the fun part: there are no standard way to do it. However, there are 2 main ways:

The first one is supported by **gcc** and **clang**, but, if I'm not mistaken, is not supported by VC++, and a few other compilers:
```cpp
struct __attribute__((packed)) Frame {
  enum class Type : uint8_t {
    REQUEST = 0,
    RESPONSE = 1
  } type;

  uint16_t length;
  char content[32];
};
```

The second is supported by most compilers:
```cpp
#pragma pack(1)
struct Frame {
  enum class Type : uint8_t {
    REQUEST = 0,
    RESPONSE = 1
  } type;

  uint16_t length;
  char content[32];
};
```

To me, that second one looks much better, and having more compatibility sounds great. However, there's a catch.

*Note:**[2]** While packing is cool, it can **seriously** impact code performance & size.*

## When shit hits the fan

To understand how a problem can arise, I've setup a small code example [here](code/a-cursed-pragma-pack-mistake).

In this (very incomplete) code, we have:
* A `Frame.hpp` that defines the packed structure
* A `Protocol.hpp` & `Protocol.cpp` that (would) take care of encapsulating communication, using the `Frame` structure to send & receive data
* A `main.cpp` that tries to send data

The code itself has no goal, and doesn't do much. It is just a simplified example of how a mistake can happen.

If we run `make build_gcc && ./a.out`, we can see the following output:
```
g++ main.cpp Protocol.cpp -Wall -pedantic --std=c++20 -Werror
Success: 0
[1]    122595 segmentation fault (core dumped)  ./a.out
```
What is happening ? Our code is super simple !

## Headers & Compilation units

To understand what's happening, it is important to know how a c++ code is compiled into a binary format.

For performance reasons (being able to run on multiple threads, and avoiding to recompile everything when there's a change), c++ compilation is performed through *compilation units*.

In general, each `.cpp` file has it's own compilation unit, and can thus be run concurrently.

For each compilation unit (`.cpp` file), the precompiler will perform text replacements on macros and includes.
This means that, for example, `#include "foo.hpp"` will be replaced by the content of the `.hpp` file, leading to a very large `.cpp` file containing all of the headers it needs.

Once this is done, the compiler can compile the `cpp` file into an object (`.o`). It is able to use functions, classes, etc from other files, as the are **declared** in the `hpp`s, essentially telling the compiler *don't worry, this function exist, and has the following prototype. You can use it*.

Most compilation errors are thrown at this stage of the compilation.

Once all `.cpp` files are turned into `.o` files, it is necessary to put them all together into a binary one.

This step is called the linking. One of its role is to find **definitions** for every symbol (function, class, etc) that has been declared in a `.hpp` and used in a `.cpp`.

When it cannot find the **definition** in any compilation units, you'll get the `undefined reference to ...` error.
If multiple compilation units define the same symbol, you'll get the `multiple definitions of ...` error.

While this system works fairly well, some informations can be lost in the translation between tħe source files and the object files that are given to the linker. And when that happens, you get weird bugs.

## Finding our problem

## Avoiding the problem

There are multiple ways to avoid the problem

### Use pragma pack properly

Either change
```cpp
#pragma pack(1)
struct Frame {
  enum class Type : uint8_t {
    REQUEST = 0,
    RESPONSE = 1
  } type;

  uint16_t length;
  char content[32];
};
```
to
```cpp
#pragma pack(1)
struct Frame {
  enum class Type : uint8_t {
    REQUEST = 0,
    RESPONSE = 1
  } type;

  uint16_t length;
  char content[32];
};
#pragma pack()
```

or, even better:
```cpp
#pragma pack(push, 1)
struct Frame {
  enum class Type : uint8_t {
    REQUEST = 0,
    RESPONSE = 1
  } type;

  uint16_t length;
  char content[32];
};
#pragma pack(pop)
```

### Use `__attribute__((packed))`

It doesn't look as fancy, but at least, this method doesn't bring any side-effects with it.
That's what I've decided to do from now on.

### Use a compiler that will give you warnings / errors

If you ran `make build_clang` instead of `make build_gcc` when running the example, you'd have seen the following output:
```
Protocol.cpp:4:10: error: the current #pragma pack alignment value is modified in the included file [-Werror,-Wpragma-pack]
#include "Frame.hpp"
         ^
./Frame.hpp:4:9: note: previous '#pragma pack' directive that modifies alignment is here
#pragma pack(1)
        ^
```

That's really cool, and I wish gcc had that.
However, it is not always possible to choose which compiler we want to use.
For example, some embedded targets (microcontrollers) are only supported on one compiler.

# Footnotes

**[1]** When possible, prefer using a proper serialization library ! This way, you won't have to manage endianess mismatches, backward compatibility, etc...

**[2]** [Anybody who writes #pragma pack(1) may as well just wear a sign on their forehead that says “I hate RISC”](https://devblogs.microsoft.com/oldnewthing/20200103-00/?p=103290)