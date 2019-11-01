CC:=gcc
WARN:=-Wall -Werror
LIB:=-lm -pthread
CCOPTS:=-std=c99 -ggdb -D_GNU_SOURCE

all: compile wc

compile:
	$(CC) $(WARN) $(CCOPTS) -c threadpool.c threadpool.h $(LIB)
	$(CC) $(WARN) $(CCOPTS) -c mapreduce.c mapreduce.h threadpool.h $(LIB)
	$(CC) $(WARN) $(CCOPTS) -c distwc.c mapreduce.h $(LIB)

wc: threadpool.o mapreduce.o distwc.o
	$(CC) $(WARN) $(CCOPTS) $(LIB) -o wordcount $^

clean:
	rm -rf *.o *gch wordcount

clean-results:
	rm *.txt

clean-all: clean clean-results

compress:
	zip mapreduce.zip *.c *.h *.md Makefile
