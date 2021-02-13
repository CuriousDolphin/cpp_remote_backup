# REMOTE BACKUP
Client-server application that perform an incremental backup of the content of a folder.

## Architecture
- high level client architecture https://whimsical.com/client-v2-LA4fGCYtwK7kxcj2D5j48U

## Technologies
- cpp 17
- Boost asio
- Redis 
- Docker (multi-stage build and compose)
- OpenSSL
  
## Properties
- multi-user
- multi-thread
- portable
- reilient,i hope :) 
- customizable delays
- easily expandable



# Try it
## Launch server 
in root dir type: ```docker-compose up``` , 
this will download the server image (already built), and run it. 
*(if you want to build manually, just edit docker-compose.yml)*
- data volume is mounted at /server/DATA
- redis volume is mounted at /server/redis
## Launch client
You need to copy ```client.docker-compose.yml``` inside the folder you want to dynamic-backup.
reneame it in ```docker-compose.yml``` and edit last record with desired arguments:
- arg1 **username** (available: ivan,francesco,alberto) 
- arg2 **password** (default: "mimmo")
- arg3 **server-host** (if you run server in same machine of the client put ```host.docker.internal``` otherwise the ip of the server )
- arg4 **server-port** 
- arg5 **file_watcher delay** (milliseconds) determines the interval between filesystem scans 
- arg5 **snapshot_delay** (seconds) determines how often the client requests a fresh copy of the remote filesystem structure

After the edit just run:  ```docker-compose up```.
- The volume shared with container is the directory itself.


# Setting-up environment for developing on windows over linux subsystem
## Requirements
- docker
## 1 - setup wsl2 for work on win clion
https://www.jetbrains.com/help/clion/how-to-use-wsl-development-environment-in-clion.html#wsl-tooclhain

Needed package (linux,wsl2) : 

```
 apt-get update \
  && apt-get install -y ssh \
  build-essential \
  gcc \
  g++ \
  gdb \
  clang \
  cmake \
  rsync \
  tar \
  python \
  gdbserver \
  openssh-server\
  libssl-dev\
  libboost-all-dev\
  netcat\
  && apt-get clean
```


## 2 - setup cpp_redis setup (linux,wsl2) 
``` 
# Clone the project
git clone https://github.com/cpp-redis/cpp_redis.git
# Go inside the project directory
cd cpp_redis
# Get tacopie submodule
git submodule init && git submodule update
# Create a build directory and move into it
mkdir build && cd build
# Generate the Makefile using CMake
cmake .. -DCMAKE_BUILD_TYPE=Release
# Build the library
make
# Install the library
make install 
```

## 3 - start redis
go to redis folder and simply launch ```docker-compose up```

## 4 - You're ready to working with Clion (windows) over linux-subsystem

