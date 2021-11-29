FROM ubuntu:latest

RUN groupadd -r -g 999 dict && useradd -r -g dict -u 999 dict

WORKDIR /app

RUN apt-get update
RUN apt-get install make gcc libcurl4-openssl-dev libsqlite3-dev -y

COPY ./*.c .
COPY ./*.h .
COPY ./Makefile .

RUN mkdir build
RUN make

EXPOSE 5000
CMD [ "/app/dict-server" ]
