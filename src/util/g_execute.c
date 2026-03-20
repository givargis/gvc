// Copyright (c) Tony Givargis, 2024-2026

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "g_execute.h"

int
g_execute(const char *argv[])
{
	pid_t pid, pid_;
	int status;

	if (0 > (pid = fork())) {
		G_TRACE("unable to fork child process");
		return -1;
	}
	else if (!pid) {
		execv(argv[0], (char * const *)argv);
		G_TRACE("unable to execute child process (abort)");
		abort();
		return -1;
	}
	else {
		for (;;) {
			pid_ = waitpid(pid, &status, 0);
			if ((-1 == pid_) && (EINTR == errno)) {
				continue;
			}
			break;
		}
		if ((-1 == pid_) || (pid_ != pid) || !WIFEXITED(status)) {
			G_TRACE("system failure detected (abort)");
			abort();
			return -1;
		}
		if (WEXITSTATUS(status)) {
			G_TRACE("system failure detected");
			return -1;
		}
	}
	return 0;
}
