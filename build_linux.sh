#!/bin/sh

#export CLANG_CL_WINE="$HOME/.wine/drive_c/Program Files/LLVM/bin/clang-cl.exe"

sed -i "s/FMT_HAS_INCLUDE(<cxxabi.h>) || defined(__GLIBCXX__)/defined(DSFKJFKSNOTDEFINED)/" build_wine/_deps/fmt-src/include/fmt/std.h

sed -i "s/add_subdirectory(\"proxy_generator\")//" ./RE-UE4SS/UE4SS/CMakeLists.txt

cmake -B build_wine \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 \
  -DCMAKE_TOOLCHAIN_FILE=./RE-UE4SS/cmake/toolchains/wine-msvc-toolchain.cmake

cmake --build build_wine

