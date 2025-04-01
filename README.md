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

### Unix-based Systems (Linux/macOS)

1. Clone this repository
   
2. Clone llama.cpp into the same folder
    ```bash
    git clone https://github.com/ggml-org/llama.cpp
    ```

3. Clone cpp-httplib into the same folder
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

### Windows

1. Install MSYS2 and Required Packages
    - Download and install MSYS2 from https://www.msys2.org/
    - Open MSYS2 CLANG64 terminal from the Start Menu
    - Update and install required packages:
    ```bash
    pacman -Syu
    pacman -S mingw-w64-clang-x86_64-toolchain mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-openblas git
    ```

2. Clone Repositories
    ```bash
    git clone --recurse-submodules https://github.com/ggerganov/llama.cpp
    git clone https://github.com/yhirose/cpp-httplib httplib
    ```

3. Build llama.cpp
    ```bash
    cd llama.cpp
    cmake -B build -DLLAMA_BUILD_SERVER=ON -DBUILD_SHARED_LIBS=ON
    cmake --build build --config Release -j4
    cd ..
    ```

4. Compile the Server
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

5. Copy Required DLLs
    ```bash
    cp ./llama.cpp/build/bin/libllama.dll .
    cp ./llama.cpp/build/bin/ggml*.dll .
    cp /c/msys64/clang64/bin/libc++.dll .
    cp /c/msys64/clang64/bin/libunwind.dll .
    ```

## Running the Server

### Unix-based Systems
```bash
./llm_server <model-path.gguf> <server-port>
```

### Windows
```bash
./llm_server.exe "C:/Path/to/your/model.gguf" 8080
```

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

### Missing DLLs (Windows)
If you get a "DLL not found" error on Windows, make sure all required DLLs are in the same directory as your exe or in your system PATH.

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

## Deployment Strategies

### Docker Deployment

1. Create a Dockerfile:
```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Clone repositories
RUN git clone https://github.com/ggerganov/llama.cpp && \
    git clone https://github.com/yhirose/cpp-httplib httplib

# Build llama.cpp
WORKDIR /app/llama.cpp
RUN cmake -B build && \
    cmake --build build --config Release

# Copy application files
WORKDIR /app
COPY main.cpp .
COPY *.gguf .

# Compile the server
RUN g++ -std=c++11 \
    -I./llama.cpp/include \
    -I./llama.cpp/ggml/include \
    -I./httplib \
    main.cpp \
    ./llama.cpp/build/bin/libllama.so \
    -o llm_server \
    -pthread

# Expose port
EXPOSE 8080

# Run the server
CMD ["./llm_server", "model.gguf", "8080"]
```

2. Build and run the container:
```bash
docker build -t granite-inference .
docker run -p 8080:8080 granite-inference
```

### Kubernetes Deployment

1. Create a Kubernetes deployment manifest:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: granite-inference
spec:
  replicas: 1
  selector:
    matchLabels:
      app: granite-inference
  template:
    metadata:
      labels:
        app: granite-inference
    spec:
      containers:
      - name: granite-inference
        image: granite-inference:latest
        ports:
        - containerPort: 8080
        resources:
          requests:
            memory: "8Gi"
            cpu: "4"
          limits:
            memory: "16Gi"
            cpu: "8"
        volumeMounts:
        - name: model-storage
          mountPath: /app
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-pvc
---
apiVersion: v1
kind: Service
metadata:
  name: granite-inference
spec:
  selector:
    app: granite-inference
  ports:
  - port: 80
    targetPort: 8080
  type: LoadBalancer
```

2. Deploy to Kubernetes:
```bash
kubectl apply -f k8s-deployment.yaml
```

### IBM Cloud Deployment

1. IBM Cloud Kubernetes Service (IKS) Setup:
```bash
# Install IBM Cloud CLI
curl -fsSL https://clis.cloud.ibm.com/install | sh

# Login to IBM Cloud
ibmcloud login

# Create a Kubernetes cluster (if not exists)
ibmcloud ks cluster create classic --name granite-cluster --zone us-south

# Get cluster credentials
ibmcloud ks cluster config --cluster granite-cluster
```

2. Container Registry Setup:
```bash
# Create a namespace in IBM Container Registry
ibmcloud cr namespace-add granite-namespace

# Tag and push the Docker image
docker tag granite-inference:latest us.icr.io/granite-namespace/granite-inference:latest
docker push us.icr.io/granite-namespace/granite-inference:latest
```

3. Deploy to IBM Cloud:
```bash
# Apply Kubernetes manifests
kubectl apply -f k8s-deployment.yaml

# Monitor deployment
kubectl get pods
kubectl get services
```

### Deployment Considerations

1. **Resource Management**:
   - Monitor CPU and memory usage
   - Adjust resource limits based on model size and load
   - Consider using IBM Cloud Monitoring for metrics

2. **Scaling Strategy**:
   - Horizontal scaling: Deploy multiple replicas behind a load balancer
   - Vertical scaling: Adjust pod resources based on load
   - Auto-scaling: Configure HPA (Horizontal Pod Autoscaling)

3. **Security**:
   - Use IBM Cloud IAM for authentication
   - Enable TLS/SSL for API endpoints
   - Implement network policies
   - Use secrets for sensitive data

4. **Monitoring and Logging**:
   - Set up IBM Cloud Logging
   - Configure health checks
   - Implement metrics collection

5. **Cost Optimization**:
   - Use IBM Cloud cost estimator
   - Implement auto-scaling based on demand
   - Consider reserved instances for predictable workloads

