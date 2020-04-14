Prior Art
=========

Service management is not a particularly new subject. While `sysvinit` and
`systemd` have garnered much of the attention span of the GNU/Linux world, with
`upstart` having previously seen some interest, other competitors exist. These
are described below:

IBM System Resource Controller
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A simple system for running subsystems which are comprised of subservers (i.e.
daemons.) Restarts subsystems automatically and provides notification of
failure.

The Daemontools Family
~~~~~~~~~~~~~~~~~~~~~~

`daemontools` is a service supervisor implemented by D.J. Bernstein in the
late 90s, which is designed in line with Bernstein's principles of good
software. These principles have led to a simple and elegant result.
Since then, derivatives and reimplementations along similar lines have
appeared. Some of these took on the question of broader-scale service/system
management (as opposed to just supervision of daemons), e.g. dependency
management. A few among the daemontools family are listed below:

- `runit` by Gerrit Pape: Designed from scratch to be useable as a replacement
  for `init` altogether. Used by the Void distro of GNU/Linux.
- `s6` by Laurent Bercot: Built with particular principles in mind,
  notably the ability to act as low-level components of a larger-scale service
  management suite. Accordingly `s6-rc` has been developed atop, which provides
  dependency management and the like. Includes a simple scripting language
  called `execline` for writing service start scripts in.
  A solution well-worth considering.
- `nosh` by Jonathan de Boyne Pollard. Like `s6-rc`, `nosh` provides
  higher-level service management features and is already equipped with a
  `systemd` unit-file converter and service descriptions for BSD and Linux.
  Named for its simple no-shell script interpreter.
  `nosh` is probably the best-designed cross-platform system/service manager.

OpenRC
~~~~~~

Noted for its use on the Gentoo and Funtoo GNU/Linux distros. Lightweight,
written in C and POSIX `sh`. Provides the traditional feel of init scripts
while offering modern functionality.

Launchd
~~~~~~~

Apple's take on system and service management. Combines `init`, Mach's
`mach_init`, `crond`, `atd`, `inetd`, and other low-level daemons into one.
Previously open-source under the Apache License, now appears to have been
closed (something to do with iPhone jailbreaking?), with the most recent code
drop being some years old. This is likely to be the nail in the coffin for
attempts to port launchd to FreeBSD.

SMF
~~~

The *Service Management Facility* of Illumos. System XVI is very similar in
design to SMF.