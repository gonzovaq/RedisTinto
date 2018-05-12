TARGET = Coordinador
LIBS = -g -lcrypto -lcommons -lreadline -pthread -lpthread
CC = gcc
CFLAGS = -g -Wall -g
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c)) 
HEADERS = $(wildcard *.h)

.PHONY: default all clean

default: $(TARGET)
all: default 

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ 

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) 
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@ 

clean: 
	-rm -f *.o
	-rm -f $(TARGET)