# CISA: Continuous Incremental Static Analyzer

## Introduction

CISA is an LLVM-based IR static analysis framework supporting an incremental analysis over
the `git` commit history.

The basic philosophy is to do costly static analyses (e.g., indirect call graph
analysis) incrementally while scanning through the commit history. Every analysis is
done at the parts that are modified by commits (hence incremental) and, as LLVM IR passes,
can refer to the result of other analyses.

It is still in its infancy and only supports limited stuff (e.g., analyses can only refer
to the call graph analysis, not other custom ones). If anybody reads this, 
I welcome **any** contribution.

## Concept

As the introduction mentions, CISA aims to _only analyze changed parts_ from commits.
To do so, CISA scans the commit history within a given range in chronological order and, given the changed entity `X` by the current commit (e.g., changed function or module), CISA calls _two callbacks_ that custom analyzers are expected to implement: `Update(X)` and `Aggregate(X)`.

 * `Update(X)`: _update_ the analysis for the changed entity `X`. This only updates the analysis _inside_ the entity `X`.
 * `Aggregate(X)`: _aggregate_ the analysis result for the changed entity `X`. This assembles the analysis done by `Update` and produces the final analysis result. `Aggregate` is always called after every possible `Update` has been called first, so it's safe to assume _all_ entities in source code are up-to-date.

## Workflow

The following is what developing and using a custom analyzer would look like.

 1. Write a custom analyzer (in `src/analyzer`) that implements `Update` and `Aggregate`.
 2. Build again (`$ make`).
 3. Run the CISA front-end (`$ ./cisa <repo_path> -o <out_path>`).
    * For each commit from the beginning to the end, CISA calls `Update` with all changed entities first and calls `Aggregate` next.
 4. Inspect the printed analysis result in `<out_path>`.

## Features (so far)

 * Integrated call graph analysis [MLTA, CCS'19]
 * Nice C++ interface for custom function-level analyses

## Requirements

 * LLVM 15+ (prebuilt binaries - totally okay)
 * Python 3.8.0+
 * CMake 3.16.3+

## Build

 1. Decompress the [prebuilt LLVM 15 binary](https://releases.llvm.org/download.html) to `llvm` at the root.
    - Or you can create a symlink `llvm` to the LLVM install directory (if you built LLVM on your own).

 2. Make (i.e., `$ make` at the root).

## Example 

 * TODO

## Writing Custom Analyses

 * TODO

## Repository Structure

 * `script`: CISA front-end scripts (Python)
 * `src`: CISA back-end code (C++)
   - `analyzer`: where custom analyzers reside 
   - `callgraph`: incremental call graph analysis (MLTA)
 * `extern`: external dependencies

## TODO

 * Supporting reference saving to LLVM objects (e.g., `Function`) in custom analyses
 * Supporting custom module-level analyses
 * Supporting custom analysis inter-operability

## Reference

 * Call graph analysis: code based on [MLTA](https://github.com/umnsec/mlta)
   (0cfc662b51b4, 01/02/2023)
 * Front-end/back-end binding: [pybind11](https://github.com/pybind/pybind11)
