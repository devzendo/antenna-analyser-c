This is a C command line driver for the K6BEZ antenna analyser (with
modifications by M0CUV and G0FCU).

To build
========

For Mac OSX
-----------
Tested on Lion, should build on more recent versions.
You will need to install the Xcode command line tools.
The Makefile is set to build on OSX, so just run make.

For Ubuntu 14.04 / Linux Mint 17
--------------------------------
Should build on other versions, these are the most recent LTS
releases.
The Makefile needs editing to build on Linux, ensure the
right PLATFORM definition (DEBIAN) is set, then run make.


The analyser driver should port to other UNIXlike systems,
unfortunately, it's not going to be an easy port to Windows.
Beric Dunn has a .NET driver that should work there.

If you have any problems getting it to build on other
platforms, please get in touch:
matt.gumbley@gmail.com
@mattgumbley on Twitter.
Pull requests are very welcome.


To run
======

To run, plug in your analyser, and determine its port:
ls -l /dev/tty.usbmodem*

./analyser -p/dev/tty.usbmodem1d11 -a3500000 -b3800000 -n20 -v
Will scan the UK amateur 80m band 3.50 - 3.80 MHz, with 20 steps.

./analyser -?
Gives the online help.


