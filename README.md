# Granite Compiled Inference via llama.cpp
 - Marwan Yassini Chairi El Kamel
 Forked - HTTP server for llama.cpp by:
 - Allen Bridi

## Overview
This repository contains instructions and a simple script allowing for compiled inference of GGUF files utilising the API provided by llama.cpp and http server provided by cpp-httplib.

## Prerequisites
- CMake
- C++ Compiler (g++, clang)
- GGUF File for Granite (any other model works)
    - Download from UCL SharePoint: [granite-3.1-8b-instruct-Q6_K.gguf](https://liveuclac-my.sharepoint.com/:u:/g/personal/zcababr_ucl_ac_uk/EXqhxvYjBXNGiYGeJYirz9UBIpo1_3Lmpembm1HkUz3kQQ?e=I0Vhn3)
    - Place the downloaded GGUF file in the project root directory
    - Alternative source: https://huggingface.co/lmstudio-community/granite-3.2-8b-instruct-GGUF

## Installation
1. Clone this repository
   
2. Clone llama.cpp into the same folder
    ```bash
    git clone https://github.com/ggml-org/llama.cpp
    ```

2. Clone cpp-httplib into the same folder
    ```bash
    git clone https://github.com/yhirose/cpp-httplib.git
    ```

4. Build the llama.cpp library
    ```bash
    cd llama.cpp
    cmake -B build
    cmake --build build --config Release
    ```

5. Compile the program
    ```bash
    clang++ -std=c++11 -I./llama.cpp/include -I./llama.cpp/ggml/include -I./httplib main.cpp ./llama.cpp/build/bin/libllama.dylib -o llm_server -pthread -Wl,-rpath,./llama.cpp/build/bin;
    ```

5. Run the server with the following parameters
    ```bash
    ./llm_server <model-path.gguf> <server-port>
    ```

