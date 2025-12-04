#!/bin/bash
set -ex
BUILD_TYPE=${1:-"Debug"}
PROJECT_ROOT="$(dirname "$(readlink -f "$0")")/.."
echo "--- Building project in ${BUILD_TYPE} mode ---"
mkdir -p "$PROJECT_ROOT/build"
cd "$PROJECT_ROOT/build"
#这个..是一体的，是这个cmake的源目录参数
cmake \
  -D CMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -D CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
  "$PROJECT_ROOT" # 比..更健壮
make -j$(nproc)
echo "--- Build finished successfully ---"