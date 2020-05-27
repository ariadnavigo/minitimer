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

#define FIFONAME_SIZE 64

enum {
	PAUSRES_EV,
	QUIT_EV
};

struct time {
	int hrs;
	int mins;
	int secs;
};

static void die(const char *fmt, ...);
static void cleanup(int fifofd, const char *fifoname, 
                    struct termios *origterm);
static void usage(void);

static int time_lt_zero(struct time the_time);
static void time_dec(struct time *the_time);
static int parse_time(char *time_str, struct time *the_time);

static void ui_update(struct time the_time);
static int poll_event(int fifofd);

static const char fifobase[] = "/tmp/minitimer.";

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
cleanup(int fifofd, const char *fifoname, struct termios *origterm)
{
	close(fifofd);
	unlink(fifoname);
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, origterm) != 0)
		die("Terminal could not be reset.");
}

static void
usage(void)
{
	die("usage: minitimer HH:MM:SS");
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
time_dec(struct time *the_time)
{
	--the_time->secs;
	if (the_time->secs < 0) {
		the_time->secs = 59;
		--the_time->mins;
	}

	if (the_time->mins < 0) {
		the_time->mins = 59;
		--the_time->hrs;
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

static void
ui_update(struct time the_time)
{
	printf("\r%02d:%02d:%02d", the_time.hrs, the_time.mins, the_time.secs);
	fflush(stdout);
}

static int
poll_event(int fifofd)
{
	fd_set fds;
	struct timeval tv = { .tv_sec = 0L, .tv_usec = 0L };
	int n;
	char cmd_buf = 0;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	FD_SET(fifofd, &fds);
	
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
	case 'q':
		return QUIT_EV;
	default:
		return -1;
	}
}

int
main(int argc, char **argv)
{
	struct time the_time;
	int parse_status, fifofd = -1, timer_runs = 1;
	char fifoname[FIFONAME_SIZE];
	struct termios oldterm, newterm;
	
	if (argc < 2)
		usage();

	memset(&the_time, 0, sizeof(struct time));
	parse_status = parse_time(argv[1], &the_time);
	if (parse_status < 0)
		die("Invalid or ill-formed time (must be HH:MM:SS)");

	memset(&oldterm, 0, sizeof(struct termios));
	memset(&newterm, 0, sizeof(struct termios));
	
	if (tcgetattr(STDIN_FILENO, &oldterm) != 0) {
		cleanup(fifofd, fifoname, &oldterm);
		die("Terminal attributes could not be read.");
	}
	
	newterm = oldterm;
	newterm.c_lflag &= ~ICANON & ~ECHO;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newterm) != 0) {
		cleanup(fifofd, fifoname, &oldterm);
		die("Terminal attributes could not be set.");
	}
	
	/* 
	 * Based on ideas from the Býblos project: https://sr.ht/~ribal/byblos
	 */
	snprintf(fifoname, FIFONAME_SIZE, "%s%d", fifobase, getpid());
	if (mkfifo(fifoname, (S_IRUSR | S_IWUSR)) != 0) {
		cleanup(fifofd, fifoname, &oldterm);
		die("File %s not able to be created: %s.", fifoname,
		    strerror(errno));
	}

	if ((fifofd = open(fifoname, (O_RDONLY | O_NONBLOCK))) < 0) {
		cleanup(fifofd, fifoname, &oldterm);
		die("File %s not able to be read: %s.", fifoname,
		    strerror(errno));
	}

	while ((time_lt_zero(the_time) == 0)) {
		switch (poll_event(fifofd)) {
		case PAUSRES_EV:
			timer_runs ^= 1;
			break;
		case QUIT_EV:
			/* C is just syntactic sugar for ASM, isn't it? ;) */
			goto exit;
		}

		if (timer_runs > 0) {
			ui_update(the_time);
			time_dec(&the_time);
			sleep(1);
		}
	}

exit:
	fputc('\n', stdout);
	cleanup(fifofd, fifoname, &oldterm);
	
	return 0;
}
