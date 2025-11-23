FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3-pip \
    git \
    doxygen \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install conan

RUN conan profile detect --force

WORKDIR /app
COPY . .

RUN conan install . --build=missing
RUN cmake -S . -B build/Release -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build/Release

# Run tests
RUN ./build/Release/bin/unit_tests

# Default command
CMD ["./build/Release/bin/movie_cli"]
