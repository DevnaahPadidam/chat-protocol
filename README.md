
# Chat Protocol Project

## Overview
This project implements a chat protocol using QUIC. It includes a server and client application that can exchange text messages and perform file transfers.

## Directory Structure
- `client/`: Contains the client code.
- `server/`: Contains the server code.
- `common/`: Contains shared code between client and server.
- `config/`: Configuration files for client and server.
- `Makefile`: For building the project.

## Requirements
- Linux-based OS
- gcc
- json-c library
- quiche library

## Setup Instructions
1. Clone the repository:
   ```bash
   git clone <repository_url>
   cd <repository_directory>
   ```

2. Install dependencies:
   - If using a package manager:
     ```bash
     sudo apt-get install libjson-c-dev
     ```
   - If not, build and install `json-c` manually:
     ```bash
     wget https://s3.amazonaws.com/json-c_releases/releases/json-c-0.15.tar.gz
     tar -xzf json-c-0.15.tar.gz
     cd json-c-0.15
     mkdir build
     cd build
     cmake ..
     make
     make install
     ```

3. Build the quiche library:
   ```bash
   git clone --recursive https://github.com/cloudflare/quiche.git ~/chat-protocol/common/quiche
   cd ~/chat-protocol/common/quiche
   cargo build --release
   ```

4. Build the project:
   ```bash
   make
   ```

5. Run the server:
   ```bash
   ./server/server
   ```

6. Run the client:
   ```bash
   ./client/client
   ```

## Configuration
- `config/server_config.json`:
  ```json
  {
    "port": 4433
  }
  ```
- `config/client_config.json`:
  ```json
  {
    "server_ip": "127.0.0.1",
    "server_port": 4433
  }
  ```

## Usage
- To send a text message, simply type the message and press Enter.
- To send a file, type `file:<filename>`.

## Error Handling
Basic error handling is implemented to ensure robustness. If a connection drops or an invalid message is received, appropriate error messages are logged, and the server or client attempts to recover gracefully.

## Commenting and Documentation
All functions and methods are appropriately documented to describe their functionality. Complex algorithms and data structures are also documented for clarity.

## Extra Credit
- **Using a Cloud-Based Git System**: This project is submitted via a cloud-based git system. 

## Author
- Devnaah Padidam
