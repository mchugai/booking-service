#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."

echo -e "--- Building Docker image ---"
docker build -t booking-service .

echo -e "--- Generating Doxygen documentation ---"
if [ -f "Doxyfile" ]; then
    rm -rf docs/html
    docker run --rm \
      -u $(id -u):$(id -g) \
      -v "$(pwd)":/app \
      booking-service \
      doxygen /app/Doxyfile
fi

echo -e "--- Running unit tests ---"
docker run --rm booking-service ./build/Release/bin/unit_tests

echo -e "--- Running CLI ---"
docker run --rm -it booking-service ./build/Release/bin/movie_cli
