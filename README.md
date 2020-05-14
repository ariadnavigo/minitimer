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

## Install
You may install Mini Timer by running the following command as root:

```
# make install
```

This will install the binary under $PREFIX/bin, as defined by your environment, or /usr/local/bin by default.

## License
Mini Timer is licensed under the Apache Public License version 2.0. See LICENSE file for copyright and license details.
