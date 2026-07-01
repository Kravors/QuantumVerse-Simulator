# QuantumVerse Simulator - Docker Container
# Usage: docker build -t quantumverse .
#        docker run --rm -it --device=/dev/dri quantumverse

FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libglfw3-dev \
    libglew-dev \
    libopenal-dev \
    libalut-dev \
    libxi6 \
    libxrender1 \
    libxrandr2 \
    libxfixes3 \
    libxcb1 \
    libx11-6 \
    libxext6 \
    python3 \
    python3-pip \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /opt/quantumverse

# Copy source code
COPY . .

# Build the project
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_USE_IMGUI=ON \
    && cmake --build build --config Release --parallel $(nproc)

# Set the entrypoint
ENTRYPOINT ["build/quantumverse_imgui"]
CMD ["--help"]