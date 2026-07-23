# QuantumVerse Simulator - Docker Container
# Build: docker build -t quantumverse:3.10.2 .
# Run:   docker run --rm -it --device=/dev/dri quantumverse:3.10.2

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
    libx11-xcb-dev \
    libxkbcommon-x11-dev \
    libpcre2-dev \
    libfreetype6-dev \
    libpng-dev \
    libjpeg-dev \
    libsqlite3-dev \
    libharfbuzz-dev \
    libb2-dev \
    libdouble-conversion-dev \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-quickcontrols2-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/quantumverse

COPY . .

RUN cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DQUANTUMVERSE_BUILD_TESTS=ON \
        -DQUANTUMVERSE_USE_QT=ON \
    && cmake --build build --parallel

ENTRYPOINT ["build/quantumverse_qml"]
CMD ["--help"]
