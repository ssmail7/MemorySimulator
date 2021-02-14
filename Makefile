.PHONY: all memsim clean

all: memsim

memsim: memsim.c
	gcc -o memsim memsim.c

clean:
	rm memsim