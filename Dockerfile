# Use Ubuntu as base image
FROM ubuntu:22.04

# Install required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    clang \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Clone required repositories
RUN git clone https://github.com/ggml-org/llama.cpp.git && \
    git clone https://github.com/yhirose/cpp-httplib.git

# Build llama.cpp
RUN cd llama.cpp && \
    cmake -B build && \
    cmake --build build --config Release

# Copy source files
COPY main.cpp .
COPY models/granite-3.1-8b-instruct-Q6_K.gguf ./models/

# Compile the server
RUN clang++ -std=c++11 \
    -I./llama.cpp/include \
    -I./llama.cpp/ggml/include \
    -I./cpp-httplib \
    main.cpp \
    ./llama.cpp/build/bin/libllama.dylib \
    -o llm_server \
    -pthread \
    -Wl,-rpath,./llama.cpp/build/bin

# Expose the default port
EXPOSE 8080

# Run the server
CMD ["./llm_server", "./models/granite-3.1-8b-instruct-Q6_K.gguf", "8080"] 