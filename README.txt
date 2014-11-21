This is a C command line driver for the K6BEZ antenna analyser (with
modifications by M0CUV and G0FCU).
Thanks to Lars Anderson and Krzysztof Piecuch.

To build
========

For Mac OSX
-----------
Tested on Lion, should build on more recent versions.
You will need to install the Xcode command line tools.

I developed it and tested using gnuplot installed from MacPorts, using
AquaTerm to display graph windows.

The Makefile is set to build on OSX, so just run make.


For Ubuntu 14.04 / Linux Mint 17
--------------------------------
Should build on other versions, these are the most recent LTS
releases. I've tested this on Lubuntu on an old PC :)

You will need to:
sudo apt-get install build-essential git gnuplot gnuplot-x11
You may prefer KDE, and want to install the gnuplot-qt package.

The Makefile needs editing to build on Linux, ensure the
right PLATFORM definition (DEBIAN) is set, then run make.


Porting
-------
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

To run, plug in your analyser, and determine its port. On Mac OS X:
ls -l /dev/tty.usbmodem*

On Lubuntu 14.04, the default port used by the Arduino Micro is /dev/ttyACM0,
which is what will be used, unless you override this on the command line.

./analyser -?
Gives the online help.

Scanning...
./analyser -p/dev/tty.usbmodem1d11 -a3500000 -b3800000 -n20 -v
Will scan the UK amateur 80m band 3.50 - 3.80 MHz, with 20 steps. Mac OSX port.

./analyser -a3500000 -b3800000 -n20 -oswr.png -mpng
Will scan the UK amateur 80m band 3.50 - 3.80 MHz, with 20 steps, creating a .png file
called swr.png in the current directory, showing the SWR across the band. Default Linux
/dev/ttyACM0 port.

"Oscilloscope" detector voltage plotting...
./analyser -c -df -w -mqt
Plots the forward detector voltage in a window using the gnuplot 'qt' terminal type.
(On Linux, currently, does not keep this window open - not sure why)

./analyser -c -df -oplot.png -mpng
Plots the forward detector voltage to a .png file called plot.png in the current directory.



