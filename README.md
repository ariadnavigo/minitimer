# Mini Timer - A timer in your terminal

Mini Timer is a very simple countdown timer that lives in the terminal of your 
Linux system. It supports pausing and resuming the countdown. It also provides
a named pipe which you can pass commands to to control a running instance of 
Mini Timer.

## Build
Mini Timer doesn't require any external dependencies.

Build by using:

```
$ make
```

If you'd like to customize the building process, set your desired variables at 
config.mk.

Mini Timer is configured by setting variables in config.h, using config.def.h 
as a template.

## Install
You may install Mini Timer by running the following command as root:

```
# make install
```

This will install the binary under $PREFIX/bin, as defined by your environment,
 or /usr/local/bin by default. The Makefile supports the $DESTDIR variable as 
well.

## Usage
Mini Timer requires you to put how much time you want set the timer to in a 
HH:MM:SS format. For example, for 10 minutes 34 seconds:

```
$ minitimer 00:10:34
```

For further information on the usage of Mini Timer, please refer to the manual 
page minitimer(1).

## License
Mini Timer is licensed under the Apache Public License version 2.0. See LICENSE
 file for copyright and license details.
