#include "time.h"

double get_current_time() {
	static LARGE_INTEGER freq;
	static int init;
	LARGE_INTEGER counter;
	
	if (!init) {
		QueryPerformanceFrequency(&freq);
		init = 1;
	}

	QueryPerformanceCounter(&counter);

	return (double)counter.QuadPart / (double)freq.QuadPart;
}