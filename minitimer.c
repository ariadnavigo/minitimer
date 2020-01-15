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

#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int errcond = 0;

static size_t parse_time_into_secs(char *time_str);

static size_t parse_time_into_secs(char *time_str)
{
    char *strptr = strtok(time_str, ":");

    char *errptr = NULL;
    
    /* Hours */
    size_t hours = strtoul(strptr, &errptr, 10);
    if(*errptr)
    {
        errcond = -1;
        return 0;
    }

    /* Minutes */
    strptr = strtok(NULL, ":");
    size_t mins = strtoul(strptr, &errptr, 10);
    if(*errptr)
    {
        errcond = -1;
        return 0;
    }

    /* Secs */
    strptr = strtok(NULL, "\0");
    size_t secs = strtoul(strptr, &errptr, 10);
    if(*errptr)
    {
        errcond = -1;
        return 0;
    }

    return hours * 3600 + mins * 60 + secs;
}

int main(int argc, char **argv)
{               
    if(argc < 2)
    {
        printf("Time required\n");
        return -1;
    }

    size_t time_in_secs = parse_time_into_secs(argv[1]);
    if((time_in_secs == 0) && (errcond != 0))
    {
        printf("Invalid or ill-formed time\n");
        return -1;
    }

    sleep(time_in_secs);

    printf("Time's up!\n");

    return 0;
}
