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

#include <ncurses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void time_dec(int *hrs, int *mins, int *secs);
static int parse_time(char *time_str, int *hrs, int *mins, int *secs);
static void printw_center(const char *fmt, ...);
static void start_loop(int hrs, int mins, int secs);

static void time_dec(int *hrs, int *mins, int *secs)
{
    --*secs;
    if(*secs < 0)
    {
        *secs = 59;
        --*mins;
    }

    if(*mins < 0)
    {
        *mins = 59;
        --*hrs;
    }
}

static int parse_time(char *time_str, int *hrs, int *mins, int *secs)
{
    char *strptr = strtok(time_str, ":");

    char *errptr = NULL;
    
    /* Hours */
    *hrs = strtoul(strptr, &errptr, 10);
    if(*errptr)
    {
        return -1;
    }

    /* Minutes */
    strptr = strtok(NULL, ":");
    *mins = strtoul(strptr, &errptr, 10);
    if(*errptr)
    {
        return -1;
    }

    /* Secs */
    strptr = strtok(NULL, "\0");
    *secs = strtoul(strptr, &errptr, 10);
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
    char text[1024];
    memset(text, 0, sizeof(text));
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);

    int y = 0;
    int x = 0;
    getmaxyx(stdscr, y, x);
    
    int len = strlen(text);
    int xindent = (x - len) / 2;
    int vindent = (y - 1) / 2;

    mvaddstr(vindent, xindent, text);
}
    
static void start_loop(int hrs, int mins, int secs)
{
    initscr();
    curs_set(0);

    while((hrs > 0) || (mins > 0) || (secs > 0))
    {
        clear();
        printw_center("%02d:%02d:%02d", hrs, mins, secs);
        time_dec(&hrs, &mins, &secs);
        refresh();
        sleep(1);
    }

    clear();
    printw_center("Time's up!");
    getch();
    
    endwin();
}

int main(int argc, char **argv)
{               
    if(argc < 2)
    {
        printf("Time required\n");
        return -1;
    }

    int hrs = 0;
    int mins = 0;
    int secs = 0;
    int status = parse_time(argv[1], &hrs, &mins, &secs);
    if(status)
    {
        printf("Invalid or ill-formed time\n");
        return -1;
    }

    start_loop(hrs, mins, secs);

    return 0;
}
