#include "config.h"
#include "mouse.h"
#include <pthread.h>
#include <unistd.h>

struct timeout_wrapper_args {
	void (*timeout_func)(void *arg);
	void *timeout_args;
	int ms;
};

void* timeout_wrapper(void* arg) {
	struct timeout_wrapper_args *args = (struct timeout_wrapper_args *)arg;
	usleep(args->ms * 1000);
	args->timeout_func(args->timeout_args);
	delete args;
	return NULL;
}

void create_timeout(void (*timeout_func)(void* arg), void *timeout_args, int ms) {
	struct timeout_wrapper_args *args = new struct timeout_wrapper_args();
	args->timeout_func = timeout_func;
	args->timeout_args = timeout_args;
	args->ms = ms;
	pthread_t thread;
	pthread_create(&thread, NULL, timeout_wrapper, args);
}

int main() {
	Mouse m;
	m.open_dev();
	if (!m.detect())
		errexit("not Elan Touchpad");

	m.init();
	m.parse_pkts();
	m.dump_loop();
}
