/* Copyright 2020 Eugenio M. Vigo

   Licensed under the Apache License, Version 2.0 (the "License"); you may not
   use this file except in compliance with the License.  You may obtain a copy
   of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
   License for the specific language governing permissions and limitations under
   the License.
*/

#define PRINTW_BUF_SIZE 1024

#include <ctype.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct time
{
    int hrs;
    int mins;
    int secs;
};

static int time_zero(struct time the_time);
static void time_dec(struct time *the_time);
static int parse_time(char *time_str, struct time *the_time);
static void printw_center(const char *fmt, ...);
static void printw_bottom(const char *fmt, ...);
static void ui_update(struct time the_time);
static void *timer_loop(void *ptr);
static void *cmd_loop(void *ptr);

static pthread_mutex_t timer_runs_mutex = PTHREAD_MUTEX_INITIALIZER;
static int timer_runs = 0;

static int time_zero(struct time the_time)
{
    if((the_time.hrs == 0) && (the_time.mins == 0) && (the_time.secs == 0))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
    
static void time_dec(struct time *the_time)
{
    --the_time->secs;
    if(the_time->secs < 0)
    {
        the_time->secs = 59;
        --the_time->mins;
    }

    if(the_time->mins < 0)
    {
        the_time->mins = 59;
        --the_time->hrs;
    }
}

static int parse_time(char *time_str, struct time *the_time)
{
    char *strptr = strtok(time_str, ":");

    char *errptr = NULL;
    
    /* Hours */
    the_time->hrs = strtoul(strptr, &errptr, 10);
    if(*errptr)
    {
        return -1;
    }

    /* Minutes */
    strptr = strtok(NULL, ":");
    the_time->mins = strtoul(strptr, &errptr, 10);
    if(*errptr)
    {
        return -1;
    }

    /* Secs */
    strptr = strtok(NULL, "\0");
    the_time->secs = strtoul(strptr, &errptr, 10);
    if(*errptr)
    {
        return -1;
    }

    return 0;
}

static void printw_center(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char text[PRINTW_BUF_SIZE];
    memset(text, 0, sizeof(text));
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);

    int y = 0;
    int x = 0;
    getmaxyx(stdscr, y, x);
    
    int len = strlen(text);
    int xindent = (x - len) / 2;
    int yindent = (y - 1) / 2;

    mvaddstr(yindent, xindent, text);
}

static void printw_bottom(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char text[PRINTW_BUF_SIZE];
    memset(text, 0, sizeof(text));
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);

    int y = 0;
    int x = 0;
    getmaxyx(stdscr, y, x);
    
    int len = strlen(text);
    int xindent = (x - len) / 2;
    int yindent = y - 1;

    mvaddstr(yindent, xindent, text);
}
    
static void ui_update(struct time the_time)
{   
    clear();

    printw_center("%02d:%02d:%02d", the_time.hrs, the_time.mins, the_time.secs);

    if(!time_zero(the_time))
    {
        printw_bottom("[Space] = pause/resume\t[q] = Stop");
    }
    else
    {
        printw_bottom("Press any key to exit.");
    }
    
    refresh();
}

static void *timer_loop(void *ptr)
{
    struct time *the_time = ptr;

    pthread_mutex_lock(&timer_runs_mutex);
    timer_runs = 1;
    pthread_mutex_unlock(&timer_runs_mutex);

    while(!time_zero(*the_time) && timer_runs != -1)
    {
        if(timer_runs == 1)
        {
            time_dec(the_time);
            ui_update(*the_time);
            sleep(1);
        }
    }

    return the_time;
}

static void *cmd_loop(void *ptr)
{
    struct time *the_time = ptr;
    
    while(!time_zero(*the_time))
    {
        int cmd = getch();
        if(cmd == ' ')
        {
            pthread_mutex_lock(&timer_runs_mutex);
            timer_runs = timer_runs ^ 1; // XOR'ing
            pthread_mutex_unlock(&timer_runs_mutex);
        }
        else if(tolower(cmd) == 'q')
        {
            pthread_mutex_lock(&timer_runs_mutex);
            timer_runs = -1;
            pthread_mutex_unlock(&timer_runs_mutex);
            return the_time;
        }
        else
        {
            continue;
        }
    }

    return the_time;
}

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Time required\n");
        return -1;
    }

    struct time the_time;
    memset(&the_time, 0, sizeof(struct time));
    int status = parse_time(argv[1], &the_time);
    if(status)
    {
        printf("Invalid or ill-formed time\n");
        return -1;
    }

    initscr();
    noecho();
    curs_set(0);

    pthread_t cmd_thr, timer_thr;
    int cmd_stat = pthread_create(&cmd_thr, NULL, cmd_loop, &the_time);
    int timer_stat = pthread_create(&timer_thr, NULL, timer_loop, &the_time);

    if(!cmd_stat || !timer_stat)
    {
        endwin();
        printf("Internal error while starting up timer.\n");
        return -1;
    }
   
    pthread_join(cmd_thr, NULL);
    pthread_join(timer_thr, NULL);
    
    endwin();

    return 0;
}
