FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
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
#WORKDIR /remote_backup_client
#COPY /src ./
#RUN cmake . && make

#COPY /src/remote_backup_client ./
#CMD ["./remote_backup_client"]
