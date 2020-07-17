CC = gcc
# CC = clang
CCFLAGS = -g -Wall -Wpedantic -pthread
# TARGET = s-talk.o list.o #instructorList.o
TARGET = list.o sender.o receiver.o init.o main.o

all: clean s-talk

s-talk: $(TARGET)
	$(CC) $(CCFLAGS) $(TARGET) -o s-talk

$(TARGET): %.o: %.c
	$(CC) -c $(CCFLAGS) $< -o $@

valgrind: s-talk
	valgrind -s --leak-check=full \
			 --show-leak-kinds=all \
			 --track-origins=yes \
			 --show-reachable=yes\
			./s-talk 5000 127.0.0.1 5001

# clean:
# 	rm -f s-talk.o *.out s-talk

clean:
	rm -f *.o *.out s-talk a3.zip submission/*

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
	 short.txt longText.txt lines.txt

zip: sub
	zip -r a3.zip submission/