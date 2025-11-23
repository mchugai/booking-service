#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."

BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_DIR=build/${BUILD_TYPE}

case "$BUILD_TYPE" in
    Release)   CONAN_BUILD_TYPE="release" ;;
    Debug)     CONAN_BUILD_TYPE="debug" ;;
    *)
        echo "Unsupported BUILD_TYPE: $BUILD_TYPE"
        exit 1
        ;;
esac

conan install . --build=missing
cmake --preset conan-${CONAN_BUILD_TYPE}
cmake --build ${BUILD_DIR}

echo "Local build completed: ${BUILD_DIR}"
