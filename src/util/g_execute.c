// Copyright (c) Tony Givargis, 2024-2026

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "g_file.h"
#include "g_execute.h"

static void
log_error(const char *s)
{
	const char *e;

	g_log("error: *** Captured Error Output from Child Process (BEG)");
	while ((e = strchr(s, '\n'))) {
		g_log("error: *** %.*s", (int)(e - s), s);
		s = e + 1;
	}
	if (*s) {
		g_log("error: *** %s", s);
	}
	g_log("error: *** Captured Error Output from Child Process (END)");
}

int
g_execute(const char *argv[])
{
	const char *pathname, *output;
	pid_t pid, pid_;
	int fd, status;

	if (!(pathname = g_file_pathname(".log"))) {
		G_TRACE("^");
		return -1;
	}
	if (0 > (pid = fork())) {
		g_free((void *)pathname);
		G_TRACE("unable to fork child process");
		return -1;
	}
	if (!pid) { // child
		if (0 > (fd = open(pathname, O_RDWR|O_CREAT|O_TRUNC, 0644))) {
			g_free((void *)pathname);
			G_TRACE("unable to open log file (abort)");
			abort();
		}
		if ((0 > dup2(fd, STDOUT_FILENO)) ||
		    (0 > dup2(fd, STDERR_FILENO))) {
			close(fd);
			g_file_unlink(pathname);
			g_free((void *)pathname);
			G_TRACE("system failure detected (abort)");
			abort();
		}
		close(fd);
		execv(argv[0], (char * const *)argv);
		g_file_unlink(pathname);
		g_free((void *)pathname);
		G_TRACE("unable to execute child process (abort)");
		abort();
		return -1;
	}
	else { // parent
		for (;;) {
			pid_ = waitpid(pid, &status, 0);
			if ((-1 == pid_) && (EINTR == errno)) {
				continue;
			}
			break;
		}
		if ((-1 == pid_) || (pid_ != pid) || !WIFEXITED(status)) {
			g_file_unlink(pathname);
			g_free((void *)pathname);
			G_TRACE("unable to execute child process");
			return -1;
		}
		if (WEXITSTATUS(status)) {
			if ((output = g_file_string_read(pathname))) {
				log_error(output);
				g_free((void *)output);
			}
			g_file_unlink(pathname);
			g_free((void *)pathname);
			G_TRACE("child process returned a non-zero status");
			return -1;
		}
	}
	g_file_unlink(pathname);
	g_free((void *)pathname);
	return 0;
}
