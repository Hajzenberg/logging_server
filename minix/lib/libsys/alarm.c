#include <lib.h>
#include <string.h>
#include <minix/safecopies.h>

int start_alarm(char *time) {
	message m;
	memset(&m, 0, sizeof(m));

	return _syscall(PS_PROC_NR, PS_START, &m);
}

int stop_alarm() {
	message m;
	memset(&m, 0, sizeof(m));

	return _syscall(PS_PROC_NR, PS_SNOOZE, &m);
}

int snooze() {
	message m;
	memset(&m, 0, sizeof(m));

	return _syscall(PS_PROC_NR, PS_STOP, &m);
}
