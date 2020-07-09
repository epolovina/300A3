CXX = gcc
CXXFLAGS = -g -Wall -Wpedantic
TARGET = s-talk.o list.o #instructorList.o

all: clean s-talk

s-talk: s-talk.o list.o
	$(CXX) $(CXXFLAGS) $(TARGET) -pthread -o s-talk

s-talk.o: 
	$(CXX) $(CXXFLAGS) -c s-talk.c

list.o:
	$(CXX) $(CXXFLAGS) -c list.c

# clean:
# 	rm -f s-talk.o *.out s-talk
clean:
	rm -f *.o *.out s-talk

s:
	./s-talk 5000 127.0.0.1 5001
l:
	./s-talk 5001 127.0.0.1 5000



