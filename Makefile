all: compile mapreduce

compile: # produce object file
	gcc -Wall -c dragonshell.c

mapreduce: # produce executable
	gcc -Wall -o dragonshell dragonshell.o

clean: # remove objects and executable
	rm *.o dragonshell

compress: 
	zip dragonshell.zip *.c *.md Makefile