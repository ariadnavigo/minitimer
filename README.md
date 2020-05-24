# Mini Timer - Just a small timer

Mini Timer is a very simple timer that lives in the terminal of your Linux-based system.

## Build
Mini Timer doesn't require any external dependencies.

Build by using:

```
$ make
```

If you'd like to customize the building process, set your desired variables at config.mk.

## Usage
Mini Timer requires you to put how much time you want set the timer to in a HH:MM:SS format. For example, for 10 minutes 34 seconds:

```
$ minitimer 00:10:34
```

### Commands
Press the following keys during execution for performing action:

* p = pause/resume
* q = quit

### Using the named pipe to control the timer
Mini Timer sets up a named pipe in /tmp which you can send the aforementioned commands to. Just pipe the command into it! The pipe is named /tmp/minitimer.$PID, where $PID is the PID of the Mini Timer process you want to send commands to.

You may pause or resume the timer like this, for example:

```
$ ps aux | grep minitimer
user        54738  0.0  0.0   2316   516 pts/0    S+   17:16   0:00 minitimer 0 10 00
$ echo 'p' > /tmp/minitimer.54738 
```

## Install
You may install Mini Timer by running the following command as root:

```
# make install
```

This will install the binary under $PREFIX/bin, as defined by your environment, or /usr/local/bin by default. The Makefile supports the $DESTDIR variable as well.

## License
Mini Timer is licensed under the Apache Public License version 2.0. See LICENSE file for copyright and license details.
