# Copyright 2020 Eugenio M. Vigo
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.

APPNAME := minitimer

CC ?= gcc

SRCFILES := $(wildcard *.c)
OBJFILES := $(SRCFILES:%.c=%.o)

CFLAGS := -Wall -Wextra -Wpedantic -std=c99 $(shell pkg-config --cflags ncurses)
LIBS := $(shell pkg-config --libs ncurses) -lpthread

.PHONY: dbg clean purge

$(APPNAME): $(OBJFILES)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) -c $< $(CFLAGS) $(DEBUG)

dbg: DEBUG := -g
dbg: $(APPNAME)

clean:
	rm -f *.o

purge: clean
	rm -f $(APPNAME)
