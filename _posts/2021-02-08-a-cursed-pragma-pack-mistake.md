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

## Avoiding the problem

### Use pragma pack properly

### Use __attribute__((packed))

### Use a compiler that will give you warnings / errors

# Footnotes

**[1]** When possible, prefer using a proper serialization library ! This way, you won't have to manage endianess mismatches, backward compatibility, etc...