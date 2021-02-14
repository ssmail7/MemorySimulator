# MemorySimulator

For a detailed description of this project, please refer to Project 2 - Memsim.pdf

HOW TO COMPILE

A. To compile program using Makefile (from Terminal):
	From the directory containing Makefile & memsim.c:
		Type "make"
	(** Makefile will compile memsim.c **)

B. To compile program manually (from Terminal):

	Type "gcc -o memsim memsim.c"


HOW TO RUN

Once the file has been compiled,

	Use the following format to run the program:
		"./memsim <tracefile> <nframes> <rdm|lru|fifo|vms> <debug|quiet>"
	(** Note: The desired tracefile to be run must be located in the same
	location as the memsim.exec file.) 
Arguments:
1. The name of the memory trace file to use.
2. The number of page frames in the simulated memory.
3. The page replacement algorithm to use.
4. The execution mode.
- If "quiet", then the simulator will run silently with no output until the
	very end, at which point it will print out a few simple statistics.
- If the "debug", then the simulator will print out messages displaying the
	details of each event in the trace.

HOW TO CLEAN (Remove memsim)

	From the directory containing Makefile & memsim.c:
		Type "make clean"
	(** Makefile will remove memsim **)
