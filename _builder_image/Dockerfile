FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
  && apt-get install -y  \
  build-essential \
  gcc \
  g++ \
  gdb \
  clang \
  cmake \
  libssl-dev\
  libboost-all-dev\
  git\
  && apt-get clean 


## INSTALL CPP REDIS
RUN git clone https://github.com/cpp-redis/cpp_redis.git && cd cpp_redis \
  && git submodule init && git submodule update \
  && mkdir build && cd build \
  && cmake .. -DCMAKE_BUILD_TYPE=Release \
  && make \
  && make install \
  && cd ..   

ENTRYPOINT /bin/bash

