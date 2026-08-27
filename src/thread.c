#include "thread.h"
#include "assert.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>

int get_cpu_thread_count(void) {
	SYSTEM_INFO info;
    GetSystemInfo(&info);

    return (int)info.dwNumberOfProcessors;
}

DWORD WINAPI win32_thread(LPVOID arg) {
	ThreadArg *t_arg = (ThreadArg *)arg;
	
	void (*func)(void *) 	= t_arg->func;
	void *data 				= t_arg->data;

	free(t_arg);

	func(data);
	return 0;
}

Thread create_thread(void (*func)(void *data), void *data) {
	assert(func);

	ThreadArg *arg 	= malloc(sizeof *arg);
	arg->func 		= func;
	arg->data 		= data;

	Thread thread = CreateThread(NULL, 0, win32_thread, arg, 0, NULL);
	if (!thread) {
		free(arg);
		return NULL;
	}

	return thread;
}

unsigned long get_current_thread_id() {
	return GetCurrentThreadId();
}

void join_thread(Thread thread) {
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
}

void detach_thread(Thread thread) {
	CloseHandle(thread);
}