SERVER := dict-server
CLIENT := define
CC     := cc
CFLAGS := -Wall -Werror -Wextra -Wpedantic -O2
OUT    := build
LIBS   := -lssl -lcrypto -lcurl -lsqlite3

PREFIX?=/usr/local

# Mac os openssl
ifeq ($(uname), Darwin)
	OPEN_SSL_DIR=/usr/local/opt/openssl
	LINK_FLAGS+=-I$(OPEN_SSL_DIR)/include -L$(OPEN_SSL_DIR)/lib
endif

$(OUT)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(SERVER) $(CLIENT)

SERVER_OBJS = $(OUT)/server.o \
              $(OUT)/hmap.o \
              $(OUT)/inet.o \
              $(OUT)/panic.o \
              $(OUT)/http.o \
			  $(OUT)/htmlgrep.o \
			  $(OUT)/dbclient.o \
			  $(OUT)/eloop.o \
			  $(OUT)/cstr.o

$(SERVER): $(SERVER_OBJS)
	$(CC) -o $(SERVER) $(SERVER_OBJS) $(LINK_FLAGS) $(LIBS)

CLIENT_OBJS = $(OUT)/client.o \
              $(OUT)/inet.o \
              $(OUT)/panic.o

$(CLIENT): $(CLIENT_OBJS)
	$(CC) -o $(CLIENT) $(CLIENT_OBJS) $(LINK_FLAGS) $(LIBS)

install:
	mkdir -p $(PREFIX)/bin $(PREFIX)/share/man/main1
	install -c m 555 $(CLIENT) $(PREFIX)/bin
	install -c m 444 $(CLIENT).1 $(PREFIX)/share/man/man1/$(TARGET).1

clean:
	rm $(SERVER)
	rm $(CLIENT)
	rm $(OUT)/*.o

$(OUT)/client.o: \
	./client.c \
	./inet.h \
	./panic.h

$(OUT)/server.o: \
	./server.c \
	./hmap.h \
	./http.h \
	./inet.h \
	./panic.h \
	./eloop.h

$(OUT)/hmap.o: \
	./hmap.c \
	./hmap.h

$(OUT)/http.c: \
	./http.c \
	./http.h

$(OUT)/inet.o: \
	./inet.c \
	./inet.h

$(OUT)/panic.o: \
	./panic.c \
	./panic.h

$(OUT)/htmlgrep.o: \
	./htmlgrep.c \
	./htmlgrep.h

$(OUT)/dbclient.o: \
	./dbclient.c \
	./dbclient.h

$(OUT)/eloop.o: \
	./eloop.c \
	./eloop.h

$(OUT)/cstr.o: \
	./cstr.c \
	./cstr.h
