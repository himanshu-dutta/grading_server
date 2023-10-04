BUILD_DIR=./build
SRC_DIR=./src
INCLUDE_DIR=./include

CC=g++
CFLAGS=-std=c++17 -I$(INCLUDE_DIR)

LIB_OBJS=protocol.o tcpsocket.o utils.o

all: build server client loadtestclient

server: server.o grader.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $(patsubst %,$(BUILD_DIR)/%,$?)

client: client.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $(patsubst %,$(BUILD_DIR)/%,$?)

loadtestclient: loadtestclient.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $(patsubst %,$(BUILD_DIR)/%,$?)

%.o: $(SRC_DIR)/%.cc
	$(CC) -c $(CFLAGS) -o $(BUILD_DIR)/$@ $<

build:
	mkdir -p $(BUILD_DIR) 

clean:
	@rm -rf build

.PHONY: clean all