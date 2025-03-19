@echo off
echo Building llama.cpp server for Windows...

REM Check for MSYS2 CLANG64 environment
where clang++ >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: clang++ not found. Please run this from MSYS2 CLANG64 terminal.
    exit /b 1
)

REM Build llama.cpp if not already built
if not exist "llama.cpp\build\bin\libllama.dll" (
    echo Building llama.cpp...
    cd llama.cpp
    cmake -B build -DLLAMA_BUILD_SERVER=ON -DBUILD_SHARED_LIBS=ON
    cmake --build build --config Release
    cd ..
)

REM Compile the server
echo Compiling server...
clang++ -std=c++11 -I./llama.cpp/include -I./llama.cpp/ggml/include -I./httplib main.cpp -L./llama.cpp/build/bin -lllama -lws2_32 -o llm_server.exe -lpthread

REM Copy required DLLs
echo Copying DLLs...
copy llama.cpp\build\bin\libllama.dll .
copy llama.cpp\build\bin\ggml*.dll .
copy C:\msys64\clang64\bin\libc++.dll .
copy C:\msys64\clang64\bin\libunwind.dll .

echo Build complete! Run with: ./llm_server.exe path/to/model.gguf 8080