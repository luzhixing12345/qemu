
CC = gcc
CFLAGS = -Wall -Wsign-compare -Wdeclaration-after-statement -Wstrict-aliasing -D_GNU_SOURCE -std=gnu11 -Wno-declaration-after-statement
CFLAGS += `pkg-config --cflags glib-2.0`
LDFLAGS = -pthread `pkg-config --libs glib-2.0`

# target is all the c file
TARGET = $(patsubst %.c, %, $(wildcard *.c))

all: $(TARGET)

%:%.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)