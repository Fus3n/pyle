# Benchmarks

Recorded against Pyle 0.2.0 (`94d417f`). Host machine: Windows 10/11 (WSL2, Ubuntu 26.04.1 for the Linux rows). Python is CPython; Windows rows use the official MSVC-built CPython, Linux rows are Ubuntu's gcc build.

Scripts live in this folder or in the repo; every row lists the exact script and toolchain. Nothing gets merged into this file without a runnable command and a Pyle version to attribute it to.

## Factorial

`examples/factorial.pyl` computes `factorial(15)` in a `while` loop, 100,000 times, timing the loop in seconds. `benchmarks/factorial.py` is the Python twin: identical algorithm and control flow, same 100,000 iterations, reporting the minimum of N runs (default 5).

Commands:

```sh
xmake run pyle examples/factorial.pyl          # Pyle
python benchmarks/factorial.py 5               # Python, best of 5
```

| Build | Compiler | Best (seconds) |
|-------|----------|----------------|
| Windows Pyle mingw | clang 23.1.5 | 0.0451 |
| Windows Pyle MSVC (VS 2022) | MSVC | 0.0668 |
| Windows CPython 3.14.6 | MSVC | 0.0613 |
| Linux Pyle (WSL2) | gcc 15.2.0 | 0.0425 |
| Linux CPython 3.14.4 (WSL2) | gcc 15.2.0 | 0.0397 |

Read: this benchmark sits at parity-to-an-edge, not a Pyle win. Python is faster on Linux, clang mingw flips the result on Windows, and the same clang advantage is absent on the MSVC build (0.067 vs Python 0.061). Compiler choice dominates the delta at this scale.

### Raw runs

- Windows Pyle mingw: 0.0452, 0.0451, 0.0458, 0.0463, 0.0455
- Windows Pyle MSVC: 0.0706, 0.0668, 0.0704, 0.0832, 0.0826
- Windows CPython (best of 5): 0.0613
- Linux Pyle: 0.0429, 0.0430, 0.0425, 0.0432, 0.0431
- Linux CPython (best of 5): 0.0397; individual runs 0.0397, 0.0404, 0.0407, 0.0489

## Planned

Benchmarks where Pyle genuinely beats the nearest Python alternative become the headline; this factorial row stays here until then. New rows require a runnable script, the Pyle version, and the toolchain on every entry.
