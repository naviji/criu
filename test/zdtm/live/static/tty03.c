#define _XOPEN_SOURCE
#include <stdlib.h>
#include "zdtmtst.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/ioctl.h>

const char *test_doc	= "Check a controlling terminal, if a proper fd belongs to another session leader";
const char *test_author	= "Andrey Vagin <avagin@openvz.org>";

int main(int argc, char ** argv)
{
	int fdm, fds, exit_code = 1, status;
	char *slavename;
	pid_t sid_b, sid_a, pid;
	int pfd[2];

	test_init(argc, argv);

	if (pipe(pfd) == -1) {
		err("pipe");
		return 1;
	}

	fdm = open("/dev/ptmx", O_RDWR);
	if (fdm == -1) {
		err("Can't open a master pseudoterminal");
		return 1;
	}

	grantpt(fdm);
	unlockpt(fdm);
	slavename = ptsname(fdm);

	pid = test_fork();
	if (pid == 0) {
		if (setsid() == -1) {
			err("setsid");
			return 1;
		}

		close(pfd[0]);

		/* set up a controlling terminal */
		fds = open(slavename, O_RDWR | O_NOCTTY);
		if (fds == -1) {
			err("Can't open a slave pseudoterminal %s", slavename);
			return 1;
		}
		ioctl(fds, TIOCSCTTY, 1);

		pid = test_fork();
		if (pid == 0) {
			if (setsid() == -1) {
				err("setsid");
				return 1;
			}

			close(pfd[1]);

			test_waitsig();
			exit(0);
		}

		close(fds);
		close(pfd[1]);

		test_waitsig();

		kill(pid, SIGTERM);
		wait(&status);

		exit(status);
	}

	close(pfd[1]);
	if (read(pfd[0], &sid_a, 1) != 0) {
		err("read");
		goto out;
	}

	if (ioctl(fdm, TIOCGSID, &sid_b) == -1) {
		err("The tty is not a controlling");
		goto out;
	}

	test_daemon();
	test_waitsig();

	if (ioctl(fdm, TIOCGSID, &sid_a) == -1) {
		fail("The tty is not a controlling");
		goto out;
	}

	if (sid_b != sid_a) {
		fail("The tty is controlling for someon else");
		goto out;
	}

	exit_code = 0;

out:
	kill(pid, SIGTERM);
	wait(&status);

	if (status == 0 && exit_code == 0)
		pass();

	return exit_code;
}
