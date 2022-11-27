compile:
make

run:
make run

# consumer-producer
This is the first assignment of OS class of 2021-2022. The main process creates N (specified in Makefile) child processes. The idea is to make main process 
serve requests from child processes that act like clients. Child processes can require certain lines from the file "lines.txt". The sychronization is done 
using 4 POSIX named semaphores and 2 POSIX named shared memory segments.

