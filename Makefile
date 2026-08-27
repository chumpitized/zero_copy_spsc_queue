SRC 	= 	src/parallel_queue.c		\
			src/cached_parallel_queue.c	\
			src/thread.c				\
			src/time.c

MAIN 	= src/main.c
OUT 	= bin/main
CFLAGS 	= -std=c99

run:
	gcc $(CFLAGS) $(MAIN) $(SRC) -o $(OUT)