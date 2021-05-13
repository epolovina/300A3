CC = gcc
CCFLAGS = -g -Wall -Wpedantic -pthread
BUILD_DIR = build
OBJECTS = list.o sender.o receiver.o init.o main.o
TARGET = s-talk

OBJECT := $(addprefix $(BUILD_DIR)/,$(OBJECTS))

.PHONY: all valgrind clean

all: clean $(TARGET)

$(TARGET): $(OBJECT) 
	$(CC) $(CCFLAGS) $(OBJECT) -o $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(CCFLAGS) $< -o $@

valgrind: s-talk
	valgrind -s --leak-check=full \
			 --show-leak-kinds=all \
			 --track-origins=yes \
			 --show-reachable=yes\
			./s-talk 5000 127.0.0.1 5001

clean:
	rm -rf *.o *.out s-talk a3.zip submission/* build
cleansub:
	rm -f *.o *.out s-talk

s:
	./s-talk 5000 127.0.0.1 5001
l:
	./s-talk 5001 127.0.0.1 5000

sub:
	cp -t submission/\
	 list.c list.h\
	 receiver.c receiver.h\
	 sender.c sender.h\
	 init.c init.h\
	 main.c Makefile\
	 README.md

zip: sub
	zip -r a3.zip submission/

$(BUILD_DIR):
	mkdir $@