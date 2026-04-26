# Changes from upstream gfsk8-modem-clean

This fork adds the following on top of upstream:

## New files

- `python/CMakeLists.txt` — CMake rules to build the Python extension module
- `python/gfsk8_python.cpp` — pybind11 wrapper exposing the library's public API to Python, including DecodedText for Varicode unpacking
- `CHANGES_FROM_UPSTREAM.md` — this file

## Modifications to existing files

- `CMakeLists.txt`:
  - Added `set(CMAKE_POSITION_INDEPENDENT_CODE ON)` so the static library can be linked into a shared object
  - Added `BUILD_PYTHON_MODULE` option (default ON) and conditional `add_subdirectory(python)`
- `README.md` — added fork notice block at top

## Notes

- The Python wrapper includes the private header `src/DecodedText.h` to expose Varicode-decoded text. This creates a build-time dependency on internal API; future upstream refactors may require corresponding wrapper changes.
- Built and tested on Raspberry Pi 4 (aarch64, Raspberry Pi OS 64-bit Bookworm) with Python 3.11 and pybind11 2.10.3.
