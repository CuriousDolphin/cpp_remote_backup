FROM isnob46/boost_gcc_ssl_builder:latest AS build

## COPY PROJECT SOURCES
COPY ./shared ./shared
WORKDIR /src
COPY ./server ./

## BUILD PROJECT 
RUN cmake . && make 

# copy bin to a new image
FROM ubuntu:20.04
WORKDIR /remote_backup_server
EXPOSE 5555
COPY --from=build /src/server ./src/server
