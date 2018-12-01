# Thread-Safe-Memory-Allocator
Implementation of a high-performance thread-safe allocator

In this assignment, I have made a high-performance thread-safe allocator.

Test Programs
Provided with the assignment download are two test programs:

ivec_main - Collatz Conjecture simulation with dynamic array
list_main - Collatz Conjecture simulation with linked list
A Makefile is included that links these two programs with each of three different memory allocators: the system allocator and two allocators you will need to provide.

Allocator is made thread safe by adding a single mutex guarding free list. There are no data races when calling hmalloc/hfree functions concurrently from multiple threads.
Realloc is implemented.

In the graph and report, all six versions of the program ({sys, hw7, par} X {ivec, list}). 
Timing is made on the local development machine.

report.txt includes:

ASCII art table of results
Information on test hardware / OS.
Discussion of the strategy for creating a fast allocator.
Discussion of the results
