/* See LICENSE file for copyright and license details. */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

struct time {
	int hrs;
	int mins;
	int secs;
};

static void die(const char *str);
static void usage(void);

static int time_lt_zero(struct time the_time);
static void time_dec(struct time *the_time);
static int parse_time(char *time_str, struct time *the_time);

static void ui_update(struct time the_time);
static int kbhit(void);
static void poll_event(int *timer_runs);

static void
die(const char *str)
{
	fprintf(stderr, "%s\n", str);
	exit(1);
}

static void
usage(void)
{
    die("Usage: minitimer HH:MM:SS");
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
	char *strptr = NULL;
	char *errptr = NULL;
	
	strptr = strtok(time_str, ":");
	if (!strptr)
		return -1;

	/* Hours */
	the_time->hrs = strtoul(strptr, &errptr, 10);
	if (*errptr)
		return -1;

	/* Minutes */
	if ((strptr = strtok(NULL, ":"))) {
		the_time->mins = strtoul(strptr, &errptr, 10);
		if (*errptr)
			return -1;
	} else {
		return -1;
	}

	/* Secs */
	if ((strptr = strtok(NULL, "\0"))) {
		the_time->secs = strtoul(strptr, &errptr, 10);
		if (*errptr)
			return -1;
	} else {
		return -1;
	}

	/* Disallow input of negative values */
	if ((the_time->hrs < 0) || (the_time->mins < 0) || (the_time->secs < 0))
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
kbhit(void)
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	
	FD_ZERO(&fds);
	FD_SET(0, &fds);

	return select(1, &fds, NULL, NULL, &tv);
}

static void
poll_event(int *timer_runs)
{
	char buf = 0;
	
	if (kbhit()) {
		read(STDIN_FILENO, &buf, 1); 
		switch (tolower(buf)) {
		case ' ':
			*timer_runs ^= 1;
			break;
		case 'q':
			*timer_runs = -1;
			break;
		}
	}
}

int
main(int argc, char **argv)
{
	struct time the_time;
	int timer_runs = 1;
	int parse_status = 0;
	
	struct termios old, new;
	
	if (argc < 2) {
	    usage();
		return -1;
	}

	memset(&the_time, 0, sizeof(struct time));
	parse_status = parse_time(argv[1], &the_time);
	if (parse_status < 0)
		die("Error: Invalid or ill-formed time (must be HH:MM:SS)");

	memset(&old, 0, sizeof(struct termios));
	memset(&new, 0, sizeof(struct termios));

	if (tcgetattr(STDIN_FILENO, &old) != 0)
	    die("Error: Could not get terminal attributes.");
	
	new = old;
	new.c_lflag &= ~ECHO;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new) != 0)
	    die("Error: Could not set terminal attributes.");
		
	timer_runs = 1;
	while (!time_lt_zero(the_time) && timer_runs != -1) {
		poll_event(&timer_runs);
		if (timer_runs == 1) {
			ui_update(the_time);
			time_dec(&the_time);
			sleep(1);
		}
	}
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &old) != 0)
		die("Error: Could not reset terminal. Use `reset' to do it manually.");
	
	printf("\n");
	
	return 0;
}
