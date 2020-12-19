FROM isnob46/boost_gcc_ssl_builder:latest AS build

COPY ./shared ./shared
WORKDIR /src
COPY ./client ./

#CMD [ "ls -l" ]
RUN cmake . && make 

FROM ubuntu:20.04

WORKDIR /remote_backup_client
COPY --from=build /src/client ./src/client
