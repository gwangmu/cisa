# Introduction

CISA is a static analysis framework that supports an incremental analysis over
the `git` commit history.

The basic philosophy: do some costly static analyses (e.g., indirect call graph
analysis) incrementally while scaning through the commit history. Write some
_more_ custom analyses that utilize those incrementally-done ananlyses.

It is still in its infancy and only supports limited stuff. If anybody read
this, I welcome **any** contribution.

# Features (so far)

 * Integrated incremental call graph analysis [MLTA, CCS'19]
 * Nice C++ interface for custom function-level analyses

# Requirements

 * LLVM 15+ (prebuilt binaries - totally okay)
 * Python 3.8.0+
 * CMake 3.16.3+

# Build

_Every step is done at the repository root directory. (i.e., the directory
having this README.md file)_

 1. Create a symlink `llvm` to the LLVM install directory.
  - The "install directory": the directory containing `bin`, `lib`, and
    stuff. Or rather, you can just directly decompress the (prebuilt LLVM
    binaries)[https://releases.llvm.org/download.html] to `llvm`.

 2. Make.

```
$ make
```

# Example 

 * TODO

# Writing Custom Analyses

 * TODO

# Reference

 * Call graph analysis: code based on [mlta](https://github.com/umnsec/mlta)
   (0cfc662b51b4, 01/02/2023)
