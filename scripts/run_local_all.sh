#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."

BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_DIR=build/${BUILD_TYPE}

if [ -x "./scripts/build_local.sh" ]; then
    ./scripts/build_local.sh
else
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
fi
echo "--- Build completed ---"

if command -v doxygen >/dev/null 2>&1; then
    echo "--- Generating Doxygen docs ---"
    doxygen Doxyfile
    echo "Documentation generated in ./docs"
else
    echo "Doxygen not installed. Skipping documentation generation."
fi

echo "--- Running unit tests ---"
${BUILD_DIR}/bin/unit_tests

echo "--- Running CLI ---"
${BUILD_DIR}/bin/movie_cli
