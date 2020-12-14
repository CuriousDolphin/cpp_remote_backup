FROM isnob46/boost_gcc_ssl_builder:latest AS build
ENV DEBIAN_FRONTEND=noninteractive

COPY ./shared ./shared
WORKDIR /src
COPY ./client ./

#CMD [ "ls -l" ]
RUN cmake . && make 

FROM ubuntu:latest
WORKDIR /remote_backup_client
COPY --from=build /src ./

