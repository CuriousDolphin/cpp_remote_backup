# Setup wsl2 for work on win clion
https://www.jetbrains.com/help/clion/how-to-use-wsl-development-environment-in-clion.html#wsl-tooclhain

Needed package : 

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


## cpp_redis (library) install (linux-wsl2) 
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

# redis container
go to redis folder and simply launch ```docker-compose up```

