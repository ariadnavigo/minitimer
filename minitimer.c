/* See LICENSE file for copyright and license details. */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

static char *argv0; /* Required here by arg.h */
#include "arg.h"
#include "config.h"

#define FIFONAME_SIZE 64

enum {
	PAUSRES_EV,
	INCR_EV,
	LAP_EV,
	QUIT_EV
};

struct time {
	int hrs;
	int mins;
	int secs;
};

static void die(const char *fmt, ...);
static void usage(void);

static int time_lt_zero(struct time the_time);
static void time_inc(struct time *the_time, int secs);
static int parse_time(char *time_str, struct time *the_time);

static struct termios ui_setup(struct termios *old);
static void ui_update(const struct time *the_time, int status);
static int poll_event(int fifofd);

static void
die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);

	va_end(ap);

	exit(1);
}

static void
usage(void)
{
	die("usage: minitimer [-sv] [HH:MM:SS]");
}

static int
time_lt_zero(struct time the_time)
{
	if (the_time.hrs < 0)
		return 1;
	else
		return 0;
}

static void
time_inc(struct time *the_time, int secs)
{
	int car;

	the_time->secs += secs;

	/* Basic sexagesimal arithmetics follow */

	if (the_time->secs < 0) {
		car = the_time->secs / 60 + 1;
		the_time->secs += 60 * car;
		the_time->mins -= car;
	}

	if (the_time->secs > 59) {
		the_time->mins += the_time->secs / 60;
		the_time->secs %= 60;
	}

	if (the_time->mins < 0) {
		car = the_time->mins / 60 + 1;
		the_time->mins += 60 * car;
		the_time->hrs -= car;
	}

	if (the_time->mins > 59) {
		the_time->hrs += the_time->mins / 60;
		the_time->mins %= 60;
	}
}

static int
parse_time(char *time_str, struct time *the_time)
{
	char *strptr, *errptr;

	strptr = strtok(time_str, ":");
	if (strptr == NULL)
		return -1;

	/* Hours */
	the_time->hrs = strtoul(strptr, &errptr, 10);
	if (*errptr != 0)
		return -1;

	/* Minutes */
	if ((strptr = strtok(NULL, ":")) != NULL) {
		the_time->mins = strtoul(strptr, &errptr, 10);
		if (*errptr != 0)
			return -1;
	} else {
		return -1;
	}

	/* Secs */
	if ((strptr = strtok(NULL, "\0")) != NULL) {
		the_time->secs = strtoul(strptr, &errptr, 10);
		if (*errptr != 0)
			return -1;
	} else {
		return -1;
	}

	/* Disallow input of negative values */
	if ((the_time->hrs < 0) || (the_time->mins < 0) ||
	    (the_time->secs < 0))
		return -1;
	else
		return 0;
}

static struct termios
ui_setup(struct termios *old)
{
	struct termios new;

	if (tcgetattr(STDIN_FILENO, old) < 0)
		die("Terminal attributes could not be read.");

	new = *old;
	new.c_lflag &= ~ICANON & ~ECHO;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new) < 0)
		die("Terminal attributes could not be set.");

	return new;
}

static void
ui_update(const struct time *the_time, int status)
{
	static struct time output;

	if (the_time != NULL)
		output = *the_time;

	printf("\r");
	putchar((status > 0) ? status_ind : ' ');
	printf(outputfmt, output.hrs, output.mins, output.secs);

	fflush(stdout);
}

static int
poll_event(int fifofd)
{
	fd_set fds;
	struct timeval tv;
	int n;
	char cmd_buf;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	FD_SET(fifofd, &fds);

	cmd_buf = 0;
	tv.tv_sec = 0L;
	tv.tv_usec = 0L;
	if ((n = select(fifofd + 1, &fds, NULL, NULL, &tv)) > 0) {
		if (FD_ISSET(STDIN_FILENO, &fds))
			read(STDIN_FILENO, &cmd_buf, 1);
		if (FD_ISSET(fifofd, &fds)) {
			read(fifofd, &cmd_buf, 1);
		}
	}

	switch (tolower(cmd_buf)) {
	case 'p':
		return PAUSRES_EV;
	case 'l':
		return LAP_EV;
	case '+':
		return INCR_EV;
	case 'q':
		return QUIT_EV;
	default:
		return -1;
	}
}

int
main(int argc, char *argv[])
{
	struct time the_time;
	int delta, parse_status, fifofd, timer_runs, ui_active;
	char fifoname[FIFONAME_SIZE];
	struct termios oldterm;

	delta = -1; /* Default is counting time down. */

	ARGBEGIN {
	case 's':
		delta = 1;
		break;
	case 'v':
		die("minitimer %s. See LICENSE file for copyright and license"
		    " details.", VERSION);
		break;
	default:
		usage();
		break;
	} ARGEND;

	/* the_time is set by default to 00:00:00 */
	memset(&the_time, 0, sizeof(struct time));
	if (argc > 0) {
		parse_status = parse_time(argv[0], &the_time);
		if (parse_status < 0)
			die("Invalid or ill-formed time (must be HH:MM:SS)");
	}

	/*
	 * Setting the named pipe up. This is based on ideas from the Býblos 
	 * text editor project (https://sr.ht/~ribal/byblos/)
	 */
	snprintf(fifoname, FIFONAME_SIZE, "%s%d", fifobase, getpid());
	if (mkfifo(fifoname, (S_IRUSR | S_IWUSR)) < 0)
		die("File %s not able to be created: %s.", fifoname,
		    strerror(errno));

	if ((fifofd = open(fifoname, (O_RDONLY | O_NONBLOCK))) < 0) {
		unlink(fifoname);
		die("File %s not able to be read: %s.", fifoname,
		    strerror(errno));
	}

	ui_setup(&oldterm);

	timer_runs = 1;
	ui_active = 1;
	while ((time_lt_zero(the_time) == 0)) {
		switch (poll_event(fifofd)) {
		case PAUSRES_EV:
			timer_runs ^= 1;
			break;
		case LAP_EV:
			ui_active ^= 1;
			break;
		case INCR_EV:
			time_inc(&the_time, time_incr_secs);
			break;
		case QUIT_EV:
			/* C is just syntactic sugar for ASM, isn't it? ;) */
			goto exit;
		}

		ui_update((ui_active > 0) ? &the_time : NULL, timer_runs);
		if (timer_runs > 0) {
			time_inc(&the_time, delta);
			sleep(1);
		}
	}

exit:
	fputc('\n', stdout);
	close(fifofd);
	unlink(fifoname);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldterm);

	return 0;
}

