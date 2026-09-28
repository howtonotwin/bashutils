# bashutils
A collection of loadable builtins for GNU Bash, intended to help write robust
shell scripts (on Linux).

## Contents
Many of the builtins are small wrappers that "just call" a given Linux system
call, with as little "interpretation" as is reasonable. The idea is to let
scripts directly access the functionality exposed by the Linux kernel, which is
sometimes nicer and more conducive to robust programs than what is exposed via
plain Bash and standard utilities. Members of this family include:
* `open`: "just call `openat(2)`"

   `open` provides access to a variety of useful flags supported by the Linux
   kernel, such as `O_PATH` (save a place in the filesystem), `O_DIRECTORY`
   (check for a directory), `O_NONBLOCK` (various effects, including "don't hang
   on a named pipe", and "don't wait for a CD to be inserted"), and `O_TMPFILE`
   (create a file with no name).
* `lnat`: "just call `linkat(2)`" (but see below)

  `lnat` is like `ln(1)`, but its operands can be relative to file descriptors.
* `seek`: "just call `lseek(2)`"
* `fsync`: "just call `fsync(2)`/`fdatasync(2)`/`syncfs(2)`"
* and several others

Additional functionalities, not covered by the above, include:
* `lnat -f`: overwrite an existing filename *atomically*. Common GNU `ln -f`
  leaves a window where the target file appears to have been deleted. Together
  with `open -T` and `fsync`, `lnat -f` can also be used to implement an atomic
  overwriting copy or an atomic cross-filesystem move (common GNU `cp` and `mv`
  delete the target file before beginning to copy the new file).
* `scriptlocal`: a (rather hacky) utility for stopping a script's global
  variables from overwriting environment variables/being exported unexpectedly.
* `capset`: modify the shell's capability sets with a syntax like `capsh(1)`'s,
  but without requiring a new program be exec-ed.

## Building
This project expects to be built with GCC 16 (or later), GNU Make. The build
expects GNU Bash to be the shell in use and expects pkg-config to be available.
The build uses pkg-config to find and link with Bash 5 and libcap. (Note that
the Bash installation that runs during the build and the Bash installation
against which the loadable builtins are linked need not be the same.) Assuming
all dependencies are in place, just issuing `make` should be enough to build.
Assuming appropriate privileges, `make install` will then install the new
builtins into `$(PREFIX)` (i.e. copy them into the Bash installation under that
prefix).

Note that some of the new builtins may share a name with some of the builtins
that ship with Bash, and thus overwrite them. It is probably best to avoid
installing this project directly into a system installation of Bash.

## License
This project is released under the GNU General Public License version 3 (GPLv3)
or, at your option, any later version. This is the same license as the latest
versions of Bash itself, against which all of the builtin binaries dynamically
link. A copy of the GPLv3 is available in the file LICENSE (which itself is
*not* released under the GPL).

The `capset` builtin specifically also links against `libcap`, which needs to be
used under its BSD-3-Clause licensing option in order for its license to be
compatible with the GPLv3.

Except for those files or sections marked otherwise, this file and project are\
Copyright 2026 Rasheeq Azad <rasheeqazad@howtonotwin.net>
