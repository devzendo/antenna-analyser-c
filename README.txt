This is a C command line driver for the K6BEZ antenna analyser (with
modifications by M0CUV and G0FCU).

To build, just make (on Mac OSX).
To build on other platforms, edit the Makefile, and ensure the
right PLATFORM definition is set, then run make.

To run, plug in your analyser, and determine its port:
ls -l /dev/tty.usbmodem*

./analyser -p/dev/tty.usbmodem1d11 -a3500000 -b3800000 -n20 -v
Will scan the UK amateur 80m band 3.50 - 3.80 MHz, with 20 steps.

./analyser -?
Gives the online help.


