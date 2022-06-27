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

#include "config.h"

#define FILENAME_SIZE 64

enum {
	INCR_EV,
	LAP_EV,
	PAUSRES_EV,
	QUIT_EV,
	RESET_EV
};

typedef struct {
	int hrs;
	int mins;
	int secs;
} Time;

static void die(const char *fmt, ...);
static void file_cleanup(void);
static void usage(void);

static void time_inc(Time *tm, int secs);
static int parse_time(char *time_str, Time *tm);

static void out(Time *tm, int run, int lap, const char *label, int nl);
static int poll_event(int fifofd);

static int fifofd;
static char fifoname[FILENAME_SIZE];

static void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);

	va_end(ap);

	file_cleanup();
	exit(1);
}

static void
file_cleanup(void)
{
	close(fifofd);
	unlink(fifoname);
}

static void
usage(void)
{
	die("usage: minitimer [-lsv] [-L label] [-T command] [HH:MM:SS]");
}

static void
time_inc(Time *tm, int secs)
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
parse_time(char *time_str, Time *tm)
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
	if (tm->hrs < 0 || tm->mins < 0 || tm->secs < 0)
		return -1;
	else
		return 0;
}

static void
out(Time *tm, int run, int lap, const char *label, int nl)
{
	static Time output;

	if (lap == 0)
		output = *tm;

	if (nl == 0)
		putchar('\r');
	putchar(run > 0 ? run_ind : ' ');
	putchar(lap > 0 ? lap_ind : ' ');
	printf(outputfmt, output.hrs, output.mins, output.secs);
	if (label != NULL)
		printf("%s%s", lblsep, label);
	if (nl > 0)
		putchar('\n');

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
	case 'l':
		return LAP_EV;
	case 'p':
		return PAUSRES_EV;
	case 'q':
		return QUIT_EV;
	case 'r':
		return RESET_EV;
	case '+':
		return INCR_EV;
	default:
		return -1;
	}
}

int
main(int argc, char *argv[])
{
	Time tm, init_tm;
	char *mtlabel, *timeout_cmd;
	int opt, nl, delta, run_stat, lap_stat;
	struct termios oldterm, newterm;

	/* Setting defaults */
	nl = 0;
	delta = -1;
	mtlabel = NULL;
	timeout_cmd = NULL;

	while ((opt = getopt(argc, argv, ":lsvL:T:")) != -1) {
		switch (opt) {
		case 'l':
			nl = 1;
			break;
		case 's':
			delta = 1;
			break;
		case 'v':
			printf("minitimer %s\n", VERSION);
			return 0;
		case 'L':
			mtlabel = optarg;
			break;
		case 'T':
			timeout_cmd = optarg;
			break;
		default:
			usage();
			break;
		}
	}

	/* tm is set by default to 00:00:00 */

	memset(&tm, 0, sizeof(Time));
	if (optind < argc && parse_time(argv[optind], &tm) < 0)
		usage();

	/* Saving the initial tm value in case we hit reset */
	init_tm = tm;

	/*
	 * Setting the named pipe up. This is based on ideas from the Býblos
	 * text editor project (https://sr.ht/~ribal/byblos/)
	 */

	snprintf(fifoname, FILENAME_SIZE, "%s%d", fifobase, getpid());
	if (mkfifo(fifoname, S_IRUSR | S_IWUSR) < 0
	    || (fifofd = open(fifoname, O_RDONLY | O_NONBLOCK)) < 0)
		die("%s: %s.", fifoname, strerror(errno));

	/* termios shenaningans to get raw input */

	if (tcgetattr(STDIN_FILENO, &oldterm) < 0)
		die("Terminal attributes could not be read.");

	newterm = oldterm;
	newterm.c_lflag &= ~ICANON & ~ECHO;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newterm) < 0)
		die("Terminal attributes could not be set.");

	/* Finally! Done setting up stuff, let's get some action! */

	run_stat = 1;
	lap_stat = 0;
	while (tm.hrs >= 0) {
		switch (poll_event(fifofd)) {
		case INCR_EV:
			time_inc(&tm, time_incr_secs);
			break;
		case LAP_EV:
			lap_stat ^= 1;
			break;
		case PAUSRES_EV:
			run_stat ^= 1;
			break;
		case QUIT_EV:
			run_stat = 0;
			goto exit;
		case RESET_EV:
			tm = init_tm;
			break;
		}

		out(&tm, run_stat, lap_stat, mtlabel, nl);
		if (run_stat > 0)
			time_inc(&tm, delta);

		sleep(1);
	}

exit:
	if (nl == 0)
		putchar('\n');
	file_cleanup();
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldterm);

	if (run_stat == 1 && timeout_cmd != NULL) {
		if (system(timeout_cmd) != 0)
			return 1;
	}

	return 0;
}

