FROM ubuntu:latest

WORKDIR ./app

RUN apt-get update
RUN apt-get install make gcc libcurl4-openssl-dev libsqlite3-dev -y
COPY ./*.c .
COPY ./*.h .
COPY ./Makefile .
RUN mkdir build
RUN make
# We only need the server program
RUN rm ./define

EXPOSE 5000
CMD ["./dict-server"]
