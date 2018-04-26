TARGET = Planificador
LIBS = -lcrypto -lcommons	-lreadline
CC = gcc
CFLAGS = -g -Wall
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