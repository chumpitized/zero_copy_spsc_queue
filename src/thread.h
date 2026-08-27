#ifndef THREAD_H
#define THREAD_H

typedef void * Thread;

typedef struct ThreadArg ThreadArg;
struct ThreadArg {
	void (*func)(void *data);
	void *data;
};

int 			get_cpu_thread_count	(void);
Thread 			create_thread			(void (*func)(void *data), void *data);
unsigned long 	get_current_thread_id	();
void 			join_thread				(Thread thread);
void 			detach_thread			(Thread thread);

#endif