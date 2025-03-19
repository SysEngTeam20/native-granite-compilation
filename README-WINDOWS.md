# Granite Compiled Inference via llama.cpp for Windows

## Overview
This repository contains instructions for compiling and running a simple HTTP server for local inference with Granite models (or any GGUF model) on Windows using llama.cpp and cpp-httplib.

## Prerequisites
- Windows 10 or 11
- MSYS2 with CLANG64 environment
- GGUF File for Granite (or any other GGUF model)
  - https://huggingface.co/lmstudio-community/granite-3.2-8b-instruct-GGUF

## Installation

### Step 1: Install MSYS2 and Required Packages
1. Download and install MSYS2 from https://www.msys2.org/
2. Open MSYS2 CLANG64 terminal from the Start Menu
3. Update and install required packages:
```bash
pacman -Syu
pacman -S mingw-w64-clang-x86_64-toolchain mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-openblas git
```

### Step 2: Create Project Directory and Clone Repositories
```bash
# Navigate to your desired location (using Unix-style paths in MSYS2)
mkdir -p /c/Users/YourUsername/Desktop/granite-inference
cd /c/Users/YourUsername/Desktop/granite-inference

# Clone the required repositories
git clone --recurse-submodules https://github.com/ggerganov/llama.cpp
git clone https://github.com/yhirose/cpp-httplib httplib
```

### Step 3: Build llama.cpp
```bash
cd llama.cpp
cmake -B build -DLLAMA_BUILD_SERVER=ON -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release -j4
cd ..
```

### Step 4: Download model (if you haven't already)
Download your preferred GGUF model and place it in your project directory.

### Step 5: Compile the Server
```bash
clang++ -std=c++11 \
  -I./llama.cpp/include \
  -I./llama.cpp/ggml/include \
  -I./httplib \
  main.cpp \
  -L./llama.cpp/build/bin \
  -lllama \
  -lws2_32 \
  -o llm_server.exe \
  -lpthread
```

### Step 6: Copy Required DLLs
You need to copy several DLLs to the same directory as your executable:
```bash
cp ./llama.cpp/build/bin/libllama.dll .
cp ./llama.cpp/build/bin/ggml*.dll .
cp /c/msys64/clang64/bin/libc++.dll .
cp /c/msys64/clang64/bin/libunwind.dll .
```

### Step 7: Run the Server
```bash
./llm_server.exe "C:/Path/to/your/model.gguf" 8080
```
The server will start and listen on port 8080.

## Using the API

The server provides two endpoints:

### 1. Form-based endpoint:
```bash
curl -X POST http://localhost:8080/generate -d "prompt=Hello, how are you?"
```

### 2. JSON-based endpoint:
```bash
curl -X POST http://localhost:8080/api/generate -H "Content-Type: application/json" -d '{"prompt":"Hello, how are you?"}'
```

### Health Check:
```bash
curl http://localhost:8080/health
```

## Troubleshooting

### Missing DLLs
If you get a "DLL not found" error, make sure all required DLLs are in the same directory as your exe or in your system PATH. Additional DLLs may be required depending on your exact environment configuration.

### Server Crashes
If the server crashes during inference, try:
- Using a smaller model
- Reducing the context size
- Checking the model file integrity

### No Response from Server
If the server doesn't respond to requests:
- Check that it's still running
- Verify you're using the correct endpoint format
- Try both the `/generate` and `/api/generate` endpoints with their respective formats

## Notes
- This server is designed for local use and development purposes
- Performance will depend on your hardware, particularly CPU capabilities
- For production use, consider security hardening and load optimization