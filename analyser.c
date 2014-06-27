/*******************************************************************************
***
*** Filename         : scope.c
*** Purpose          : Antenna analyser scan to gnuplot output.
*** Author           : Matt J. Gumbley
*** Last updated     : 24/06/14
***
********************************************************************************
***
*** Modification Record
***
*******************************************************************************/

#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <stdlib.h>

#include "config.h"

#include "global.h"
#include "util.h"
#include "asy.h"

static int quit = FALSE;
static char *progname;
static char *defport = "/dev/tty.usbmodemmfd111";
static int defbps=B57600;
static int defsettle=10;
static int defsteps=100;
static char portfd = -1;
static int verbose = FALSE;

void sighandler(int signal)
{
  quit = TRUE;
}

static void banner()
{
  printf("analyser v%s - K6BEZ antenna analyser gnuplot scanner\n", VERSION);
  printf("(C) 2014 Matt J. Gumbley, M0CUV, matt@gumbley.me.uk\n");
  printf("http://github.com/devzendo/antenna-analyser-c\n\n");
}

static void usage()
{
  banner();
  printf("Syntax:\n");
  printf("  %s [options]\n", progname);
  printf("Options:\n");
  printf("  -p<port>  Set analyser port. <port> is something like /dev/tty.usbmodemmfd111.\n");
  printf("            Default is %s.\n", defport);
  printf("  -v        Enable verbose operation\n");
  printf("  -a<hz>    Set start frequency in Hertz.\n");
  printf("  -b<hz>    Set stop frequency in Hertz.\n");
  printf("  -d<ms>    Set settle delay in Milliseconds. Default %d.\n", defsettle);
  printf("  -n<num>   Set number of steps between start and stop frequency. Default %d.\n", defsteps);
  printf("  -f<file>  Set name of analyser output capture file. Default is a\n");
  printf("            temp file that's deleted. Use this to keep the output.\n");
  exit(1);
}

static void spinner()
{
static char chars[]="\\-/|";
static int c=0;
  fprintf(stderr,"%c  \r",chars[c++]);
  c&=3;
}

static void finish(int code)
{
  if (portfd != -1) {
    asy_close(portfd);
  }
  exit(code);
}

static bool write_line(char *line)
{
int len = strlen(line);
int written = asy_write(portfd, (byte *) line, len);
  return len == written;
}

static bool read_line(char *line, int maxlen)
{
int i = 0;
int ch = -1;
  bzero(line, maxlen);
  while (i < maxlen) {
    ch = asy_getc(portfd);
    if (ch < 0) {
      printf("Timeout!\n");
      return FALSE;
    }
    line[i++] = ch;
    if (ch == '\n') {
      return TRUE;
    }
  }

  if (i == maxlen) {
    printf("Buffer overflow detected\n");
    finish(99);
  }
  return FALSE;
}

static void write_line_successfully(char *line, char* error, int code) {
  if (!write_line(line)) {
    puts(error);
    finish(code);
  }
}

static void read_line_successfully(char *line, int linemax, char *error, int code) {
  if (!read_line(line, linemax)) {
    puts(error);
    finish(code);
  }
}
 
int main(int argc, char *argv[])
{
int i;
char *p;
const int portmax = 64;
char port[portmax];
long startFreq = 0L;
long stopFreq = 0L;
int numSteps = defsteps;
int settleDelay = defsettle;
const int linemax = 256;
char line[linemax];
bool scan_end = FALSE;
long scan_freq, scan_vswr, scan_fwdv, scan_revv;

  /* Initialise sensible defaults, etc. */
  progname = argv[0];
  strncpy(port, defport, portmax);

  /* Process command line options */
  for (i=1; i<argc; i++) {
    if (argv[i][0]=='-') {
      p=&argv[i][2];
      switch (argv[i][1]) {
        case 'v':
          verbose = TRUE;
          break;
        case 'p':
          strncpy(port, p, portmax);
          /* XXX: check existence, deviceness? */
          break;
        case 'a':
          sscanf(p, "%ld", &startFreq);
          break;
        case 'b':
          sscanf(p, "%ld", &stopFreq);
          break;
        case 'n':
          sscanf(p, "%d", &numSteps);
          break;
        case 's':
          sscanf(p, "%d", &settleDelay);
          break;
        default:
          usage();
      }
    }
    else
      usage();
  }

  if (verbose) {
    printf("port: %s at %d baud\n", port, defbps);
    printf("start freq: %ld Hz, end freq: %ld Hz, steps: %d, settle: %d ms\n",
      startFreq, stopFreq, numSteps, settleDelay);
  }

  /* Trap CTRL-C */
  signal(SIGINT, &sighandler);

  /* Open port */
  if ((portfd=asy_open(port, defbps)) == -1) {
    printf("port %s open failed\n", port);
    exit(-1);
  }

  write_line_successfully("q", "Could not send a q query to the analyser\n", 1);

  read_line_successfully(line, linemax, 
    "Did not read the first line of query data from the analyser\n", 2);
  if (verbose) {
    printf("Query from analyser: %s", line);
  }

  read_line_successfully(line, linemax, 
    "Did not read the second line of query data from the analyser\n", 2);
  if (verbose) {
    printf("Query from analyser: %s", line);
  }

  sprintf(line, "%ldA", startFreq);
  write_line_successfully(line, "Could not set start frequency\n", 3);

  sprintf(line, "%ldB", stopFreq);
  write_line_successfully(line, "Could not set stop frequency\n", 4);

  sprintf(line, "%dN", numSteps);
  write_line_successfully(line, "Could not set number of steps\n", 5);

  sprintf(line, "%dD", settleDelay);
  write_line_successfully(line, "Could not set settle delay\n", 6);


  write_line_successfully("s", "Could not start scan\n", 7);

  if (verbose) {
    puts("Starting scan\n");
  }
  scan_end = FALSE;
  while (!quit && !scan_end) {
    read_line_successfully(line, linemax, "Did not read the scan response\n", 8);

    if (strncmp("End", line, 3) == 0) {
      scan_end = TRUE;
    }
    else {

      if (verbose) {
        printf("Scan Line: %s", line);
      } else {
        spinner();
      }
      sscanf(line, "%ld.00,0,%ld,%ld.00,%ld.00\n",
             &scan_freq, &scan_vswr, &scan_fwdv, &scan_revv);
      // TODO Print this to the gnuplot input file
      if (verbose) {
        printf("Freq: %ld VSWR: %ld Fwd: %ld Rev: %ld\n",
               scan_freq, scan_vswr, scan_fwdv, scan_revv);
      }
    }
  }

  if (quit) {
    puts("Terminating scan...\n");
    write_line("z");
    asy_flush(portfd);
  }

  if (verbose) {
    puts("Finished\n");
  }

  asy_close(portfd);

  return 0;
}

