# QuantumVerse Simulator - Docker Container
# Build: docker build -t quantumverse:3.7.2 .
# Run:   docker run --rm -it --device=/dev/dri quantumverse:3.7.2

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install build and runtime dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    python3 \
    python3-pip \
    pkg-config \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libglfw3-dev \
    libglew-dev \
    libxi6 \
    libxrender1 \
    libxrandr2 \
    libxfixes3 \
    libxcb1 \
    libx11-6 \
    libxext6 \
    libopenal-dev \
    libalut-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/quantumverse

COPY . .

RUN cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DQUANTUMVERSE_BUILD_TESTS=ON \
        -DQUANTUMVERSE_USE_QT=OFF \
    && cmake --build build --parallel

ENTRYPOINT ["build/quantumverse_imgui"]
CMD ["--help"]
