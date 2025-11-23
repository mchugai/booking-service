# Booking Service

A thread-safe C++ reservation service with fine-grained locking, atomic booking operations, JSON-based data initialization, and a clean API suitable for various booking domains (movies, events, rooms, seats, etc.).

## Automation Scripts
The /scripts folder contains convenience scripts:

### build_local.sh - Builds the project locally (Conan + CMake)
```bash
./scripts/build_local.sh
```

### run_local_all.sh - Full local pipeline: build, generate docs, run tests, run CLI
```bash
./scripts/run_local_all.sh
```

### run_docker_all.sh - Full Docker pipeline: build image, generate docs in container, run tests, run CLI
```bash
./scripts/run_docker_all.sh
```

## Local Build

```bash
conan install . --build=missing
cmake --preset conan-release
cmake --build build/Release

# Generate docs
doxygen Doxyfile

# Run Tests
./build/Release/bin/unit_tests

# Run CLI
./build/Release/bin/movie_cli
```

## Using Docker

```bash
# Build the image
docker build -t booking-service .

# Generate docs
docker run --rm -u $(id -u):$(id -g) -v $(pwd):/app booking-service doxygen /app/Doxyfile

# Run tests
docker run --rm booking-service ./build/Release/bin/unit_tests

# Run the CLI
docker run -it booking-service
```
