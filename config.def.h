/* See LICENSE file for copyright and license details. */

/* 
 * fifobase: Base pathname of the named pipe. The actual name will also have 
 * the PID attached to it as a suffix (i.e. "$fifobase$PID").
 */
static const char fifobase[] = "/tmp/minitimer.";

/*
 * outputfmt: Controls the output format of the timer. The format string must 
 * follow the printf format specification and always print all three: hours, 
 * minutes, and seconds as integers (%d). The default format is: HH:MM:SS.
 */
static const char outputfmt[] = "%02d:%02d:%02d";

/* 
 * run_ind: This character is printed before the timer when running (also
 * when running in the background when showing a lap).
 */
static const char run_ind = '*';

/* 
 * lap_ind: This character is printed before the timer to show that the time
 * currently shown is a lap.
 */
static const char lap_ind = '+';

/*
 * time_incr_secs: Increment the timer by how many seconds when pressing '+'.
 */
static const int time_incr_secs = 10;
