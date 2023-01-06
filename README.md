# minitimer - A timer in your terminal

minitimer is a very simple timer that lives in the terminal of your system. It
also provides a named pipe which you can pass commands to to control a running
instance of minitimer.

## Basic usage

minitimer takes the time to count down in YY:MM:SS format. The operation allows
for commands to ``(p)ause``, save a ``(l)ap``, and to ``(q)uit``. Commands
can be sent to a running instance of minitimer via a named pipe.

```
$ ./minitimer 00:10:30
* 00:10:25
$ echo 'q' > /tmp/minitimer.148371 # Sends 'quit' to minitimer (PID 148371) 
```

You may check the ``minitimer(1)`` manpage for further usage information.

## Build

minitimer is supported on Linux and OpenBSD, and requires:

1. A C99 compiler

Build by using:

```
$ make
```

If you'd like to customize the building process, set your desired variables in
``config.mk``.

minitimer is configured by setting variables in ``config.h``, using
``config.def.h`` as a template.

## Install

You may install minitimer by running the following command as root:

```
# make install
```

This will install the binary under ``$PREFIX/bin``, as defined by your
environment, or ``/usr/local/bin`` by default. The Makefile supports the
``$DESTDIR`` variable as well.

## License

minitimer is published under an MIT/X11/Expat-type License. See ``LICENSE``
file for copyright and license details.
