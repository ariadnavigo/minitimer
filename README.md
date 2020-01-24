# Mini Timer - Just a small timer

Mini Timer is a very simple timer alarm that lives in the terminal of your Linux-based system. This is in a very early stage of development, but the primary goal is that it actually plays some sound when the timer is fired. Other development goals can be found on the project's GitHub Issues page.

## Build
Mini Timer requires these dependencies:

1. pkg-config (only at compile-time)
2. ncurses with development headers

Build by using:

```
$ make
```

If you'd like to build Mini Timer with debugging symbols:

```
$ make dbg
```

## Usage
Mini Timer requires you to put how much time you want set the timer to in a HH:MM:SS format. For example, for 10 minutes 34 seconds:

```
$ minitimer 00:10:34
```

## License
Mini Timer is licensed under the Apache Public License version 2.0. See LICENSE.txt for the full text of the license.
