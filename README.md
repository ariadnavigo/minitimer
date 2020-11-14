# minitimer - A timer in your terminal

minitimer is a very simple timer that lives in the terminal of your system. It
also provides a named pipe which you can pass commands to to control a running
instance of minitimer.

## Build
minitimer doesn't require any external dependencies.

Build by using:

```
$ make
```

If you'd like to customize the building process, set your desired variables in
config.mk.

minitimer is configured by setting variables in config.h, using config.def.h as
a template.

## Install
You may install minitimer by running the following command as root:

```
# make install
```

This will install the binary under $PREFIX/bin, as defined by your environment,
or /usr/local/bin by default. The Makefile supports the $DESTDIR variable as 
well.

## Usage
minitimer requires you to put how much time you want set the timer to in 
HH:MM:SS format. For example, for 10 minutes 34 seconds:

```
$ minitimer 00:10:34
```

minitimer also provides a stopwatch mode (i.e. counting time up). You may pass
an initial time code from which to start counting up or leave blank to start 
from 00:00:00.

```
$ minitimer -s # minitimer will start counting from 00:00:00 up
```

For further information on the usage of minitimer, please refer to the manual 
page minitimer(1).

## License
minitimer is licensed under the Apache Public License version 2.0. See LICENSE
file for copyright and license details.
