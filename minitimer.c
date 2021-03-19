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

static int time_lt_zero(struct time tm);
static void time_inc(struct time *tm, int secs);
static int parse_time(char *time_str, struct time *tm);

static void print_time(const struct time *tm, int run_stat, int lap_stat);
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
time_lt_zero(struct time tm)
{
	return tm.hrs < 0;
}

static void
time_inc(struct time *tm, int secs)
{
	int car;

	tm->secs += secs;

	/* Basic sexagesimal arithmetics follow */

	if (tm->secs < 0) {
		car = tm->secs / 60 + 1;
		tm->secs += 60 * car;
		tm->mins -= car;
	}

	if (tm->secs > 59) {
		tm->mins += tm->secs / 60;
		tm->secs %= 60;
	}

	if (tm->mins < 0) {
		car = tm->mins / 60 + 1;
		tm->mins += 60 * car;
		tm->hrs -= car;
	}

	if (tm->mins > 59) {
		tm->hrs += tm->mins / 60;
		tm->mins %= 60;
	}
}

static int
parse_time(char *time_str, struct time *tm)
{
	char *strptr, *errptr;

	strptr = strtok(time_str, ":");
	if (strptr == NULL)
		return -1;

	/* Hours */
	tm->hrs = strtoul(strptr, &errptr, 10);
	if (*errptr != 0)
		return -1;

	/* Minutes */
	if ((strptr = strtok(NULL, ":")) != NULL) {
		tm->mins = strtoul(strptr, &errptr, 10);
		if (*errptr != 0)
			return -1;
	} else {
		return -1;
	}

	/* Secs */
	if ((strptr = strtok(NULL, "\0")) != NULL) {
		tm->secs = strtoul(strptr, &errptr, 10);
		if (*errptr != 0)
			return -1;
	} else {
		return -1;
	}

	/* Disallow input of negative values */
	if ((tm->hrs < 0) || (tm->mins < 0) || (tm->secs < 0))
		return -1;
	else
		return 0;
}

static void
print_time(const struct time *tm, int run_stat, int lap_stat)
{
	static struct time output;

	if (tm != NULL)
		output = *tm;

	printf("\r");
	putchar((run_stat > 0) ? run_ind : ' ');
	putchar((lap_stat > 0) ? lap_ind : ' ');
	printf(outputfmt, output.hrs, output.mins, output.secs);

	fflush(stdout);
}

static int
poll_event(int fifofd)
{
	fd_set fds;
	struct timeval tv;
	char cmd_buf;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	FD_SET(fifofd, &fds);

	cmd_buf = 0;
	tv.tv_sec = 0L;
	tv.tv_usec = 0L;
	if (select(fifofd + 1, &fds, NULL, NULL, &tv) > 0) {
		if (FD_ISSET(STDIN_FILENO, &fds))
			read(STDIN_FILENO, &cmd_buf, 1);
		if (FD_ISSET(fifofd, &fds))
			read(fifofd, &cmd_buf, 1);
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
	struct time tm;
	int delta, parse_stat, fifofd, run_stat, lap_stat;
	char fifoname[FIFONAME_SIZE];
	struct termios oldterm, newterm;

	delta = -1; /* Default is counting time down. */

	ARGBEGIN {
	case 's':
		delta = 1;
		break;
	case 'v':
		die("minitimer %s", VERSION);
		break;
	default:
		usage();
		break;
	} ARGEND;

	/* tm is set by default to 00:00:00 */

	memset(&tm, 0, sizeof(struct time));
	if (argc > 0) {
		parse_stat = parse_time(argv[0], &tm);
		if (parse_stat < 0)
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

	/* termios shenaningans to get raw input */

	if (tcgetattr(STDIN_FILENO, &oldterm) < 0) {
		close(fifofd);
		unlink(fifoname);
		die("Terminal attributes could not be read.");
	}

	newterm = oldterm;
	newterm.c_lflag &= ~ICANON & ~ECHO;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newterm) < 0) {
		close(fifofd);
		unlink(fifoname);
		die("Terminal attributes could not be set.");
	}

	/* Finally! Done setting up stuff, let's get some action! */

	run_stat = 1;
	lap_stat = 0;
	while ((time_lt_zero(tm) == 0)) {
		switch (poll_event(fifofd)) {
		case PAUSRES_EV:
			run_stat ^= 1;
			break;
		case LAP_EV:
			lap_stat ^= 1;
			break;
		case INCR_EV:
			time_inc(&tm, time_incr_secs);
			break;
		case QUIT_EV:
			/* C is just syntactic sugar for ASM, isn't it? ;) */
			goto exit;
		}

		if (run_stat > 0) {
			print_time((lap_stat > 0) ? NULL : &tm, run_stat, 
			           lap_stat);
			time_inc(&tm, delta);
		} else {
			print_time(NULL, run_stat, lap_stat);
		}
		sleep(1);
	}

exit:
	putchar('\n');
	close(fifofd);
	unlink(fifoname);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldterm);

	return 0;
}

