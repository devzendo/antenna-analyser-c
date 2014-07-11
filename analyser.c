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
#include <unistd.h>
#include <errno.h>

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
static const int linemax = 256;
static const int fileNameMax = 128; // get this from limits.h?
static char *tempScanFileName = NULL;
static char scanFileName[fileNameMax];
static char plotFileName[fileNameMax];
static bool scanFileTemporary = TRUE;
static FILE *scanOutput = NULL;
static FILE *gnuplotCommandsOutput = NULL;

static const int PLOT_TYPE_VSWR = 0;
static const int PLOT_TYPE_FWD = 1;
static const int PLOT_TYPE_REV = 2;

void sighandler(int signal)
{
  quit = TRUE;
}


static void banner()
{
  printf("analyser v%s - K6BEZ antenna analyser driver & gnuplot plotter\n", VERSION);
  printf("(C) 2014 Matt J. Gumbley, M0CUV, matt@gumbley.me.uk\n");
  printf("http://github.com/devzendo/antenna-analyser-c\n\n");
  printf("With thanks to:\n");
  printf("  Beric Dunn K6BEZ for the analyser design\n");
  printf("  Simon Kennedy G0FCU for assistance with gnuplot\n");
  printf("\n");
}


static void usage()
{
  banner();
  printf("This program can be run in one of two modes:\n");
  printf("* Use a connected analyser to scan a frequency range, writing the\n");
  printf("  output to a file.\n");
  printf("* Generate/display a plot of the current scan, or a previously\n");
  printf("  saved file.\n");
  printf("- You can run both modes at the same time - scan and plot.\n");
  printf("- You can measure detector voltages instead of scanning.\n");
  printf("\n");
  printf("Syntax:\n");
  printf("  %s [options]\n", progname);
  printf("Options:\n");
  printf("  -v        Enable verbose operation\n");
  printf("Scan options:\n");
  printf("  -p<port>  Set analyser port. <port> is something like /dev/tty.usbmodemmfd111.\n");
  printf("            Default is %s.\n", defport);
  printf("  -a<hz>    Set start frequency in Hertz.\n");
  printf("  -b<hz>    Set stop frequency in Hertz.\n");
  printf("  -s<ms>    Set settle delay in Milliseconds. Default %d.\n", defsettle);
  printf("  -n<num>   Set number of steps between start and stop frequency. Default %d.\n", defsteps);
  printf("  -f<file>  Set name of analyser output capture file. Default is a\n");
  printf("            temp file that's deleted. Use this to keep the output.\n");
  printf("(You must give -a/-b to run a scan.)\n");
  printf("\n");
  printf("Detector voltage scan:\n");
  printf("  -df       Read forward detector voltages\n");
  printf("  -dr       Read reverse detector voltages\n");
  printf("(You may give -a<hz> to set the frequency before measuring voltage)\n");
  printf("\n");
  printf("Plot options:\n");
  printf("  -t<title> Set the title shown in the plot output\n");
  printf("  -o<file>  Set name of plot output file e.g. dipole.png\n");
  printf("  -m<term>  Use this terminal type with gnuplot, e.g.\n");
  printf("            qt, aqua, x11, png, canvas, eps. Default is canvas.\n");
  printf("  -w        Display the plot in a window, using an appropriate\n");
  printf("            gnuplot terminal for your system: aqua on Mac OS X...\n");
  printf("(You must give either -o and -m<term> to plot to a file\n");
  printf(" or -w to display interactively without saving the plot.\n");
  printf(" If you have used -f to scan to a file and want to plot that file,\n");
  printf(" just give -f and the plot options.\n");
  exit(1);
}


static void spinner()
{
static char chars[]="\\-/|";
static int c=0;
  fprintf(stderr,"%c  \r",chars[c++]);
  c&=3;
}


static void close_serial() {
  if (portfd != -1) {
    asy_close(portfd);
    portfd = -1;
  }
}

static void finish(int code)
{
  close_serial();

  if (scanOutput != NULL) {
    fclose(scanOutput);
  }

  if (scanFileTemporary) {
    unlink(scanFileName);
  }
  if (tempScanFileName != NULL) {
    free(tempScanFileName);
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
 

static char *tmpenv = NULL;

char *allocateTempFileName() {
char tempPath[fileNameMax];
char *tempFileName;

  if (tmpenv == NULL) {
    tmpenv = getenv("TMPDIR");
  }
  if (tmpenv == NULL) {
    tmpenv = "/tmp/";
  }

  if (tmpenv[strlen(tmpenv) - 1] == '/') {
    sprintf(tempPath, "%stemp.XXXX", tmpenv);
  } else {
    sprintf(tempPath, "%s/temp.XXXX", tmpenv);
  }

  if (mktemp(tempPath) == NULL) {
    printf("Cannot create temporary file name\n");
    exit(-1);
  }
  tempFileName = strdup(tempPath);
  if (tempFileName == NULL) {
    printf("Cannot allocate memory for temporary file name\n");
    exit(-1);
  }
  return tempFileName;
}


void openSerialAndScanOutput(bool verbose, char *port, char *scanFileName) {
char line[linemax];

  if (verbose) {
    printf("port: %s at %d baud\n", port, defbps);
  }

  scanOutput = fopen(scanFileName, "w+");
  if (scanOutput == NULL) {
    printf("Cannot open scan file '%s' for write: %s\n", scanFileName, strerror(errno));
    exit(-1);
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
}


void closeSerialAndScanOutput() {
  fclose(scanOutput);
  scanOutput = NULL;

  close_serial();
}


void scan(bool verbose, char* port, long startFreq, long stopFreq,
  int numSteps, int settleDelay, char *scanFileName) {
bool scan_end = FALSE;
char line[linemax];
long scan_freq, scan_vswr, scan_fwdv, scan_revv;
char scanLineOutput[linemax];

  openSerialAndScanOutput(verbose, port, scanFileName);

  if (verbose) {
    printf("start freq: %ld Hz, end freq: %ld Hz, steps: %d, settle: %d ms\n",
      startFreq, stopFreq, numSteps, settleDelay);
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
      // Ignore the .00 parts of the fields for now...
      sscanf(line, "%ld.00,0,%ld,%ld.00,%ld.00\n",
             &scan_freq, &scan_vswr, &scan_fwdv, &scan_revv);
      sprintf(scanLineOutput, "%f %f\n", scan_freq / 1000000.0, scan_vswr / 1000.0);
      fputs(scanLineOutput, scanOutput);
      if (verbose) {
        printf("Freq: %ld VSWR: %ld Fwd: %ld Rev: %ld\n",
               scan_freq, scan_vswr, scan_fwdv, scan_revv);
        printf("Output to gnuplot: %s", scanLineOutput);
      }
    }
  }

  if (quit) {
    puts("Terminating scan...\n");
    write_line("z");
    asy_flush(portfd);
  }

  closeSerialAndScanOutput();
}

void oscilloscope(bool verbose, char* port, long startFreq, int settleDelay, char *scanFileName, int plotType) {
bool scan_end = FALSE;
char line[linemax];
long sample_num, voltage;
char scanLineOutput[linemax];

  openSerialAndScanOutput(verbose, port, scanFileName);

  if (verbose) {
    printf("start freq: %ld Hz, settle: %d ms\n", startFreq, settleDelay);
  }

  sprintf(line, "%ldA", startFreq);
  write_line_successfully(line, "Could not set start frequency\n", 3);

  sprintf(line, "%dD", settleDelay);
  write_line_successfully(line, "Could not set settle delay\n", 6);


  write_line_successfully("o", "Could not start oscilloscope\n", 7);

  if (verbose) {
    puts("Starting oscilloscope\n");
  }
  scan_end = FALSE;
  while (!quit && !scan_end) {
    read_line_successfully(line, linemax, "Did not read the oscilloscope response\n", 8);

    if (strncmp("End", line, 3) == 0) {
      scan_end = TRUE;
    }
    else {

      if (verbose) {
        printf("Oscilloscope Line: %s", line);
      } else {
        spinner();
      }
      // Ignore the .00 parts of the fields for now...
      sscanf(line, "%ld %ld\n", &sample_num, &voltage);
      sprintf(scanLineOutput, "%ld %ld\n", sample_num, voltage);
      fputs(scanLineOutput, scanOutput);
      if (verbose) {
        printf("Sample: %ld Voltage: %ld\n", sample_num, voltage);
        printf("Output to gnuplot: %s", scanLineOutput);
      }
    }
  }

  closeSerialAndScanOutput();
}


void plot(bool window, char *title, char *term, 
  char *plotFileName, char *scanFileName, int plotType) {
char gnuplotCommand[linemax];
char *gnuplotCommandsFileName = allocateTempFileName();
char termTitleCommand[linemax];

  // Include the title in the plot if the terminal type supports it in the 
  // term command. gif, jpeg, png don't. 
  termTitleCommand[0] = '\0';
  if (strcmp(term, "aqua") == 0 ||
      strcmp(term, "x11") == 0) {
    sprintf(termTitleCommand, " title \"%s\"", title);
  }

  // Generate the plot
  gnuplotCommandsOutput = fopen(gnuplotCommandsFileName, "w+");
  if (gnuplotCommandsOutput == NULL) {
    printf("Cannot open gnuplot commands file '%s' for write: %s\n", gnuplotCommandsFileName, strerror(errno));
    finish(-1);
  }

  fprintf(gnuplotCommandsOutput, "set term %s size 600,400%s\n", 
    term, termTitleCommand);
  if (plotFileName[0] != '\0') {
    fprintf(gnuplotCommandsOutput, "set output \"%s\"\n", plotFileName);
  }
  fprintf(gnuplotCommandsOutput, "set xtics scale 2,1\n");
  fprintf(gnuplotCommandsOutput, "set mxtics 5\n");
  fprintf(gnuplotCommandsOutput, "set linetype 1 lw 1 lc rgb \"blue\" pointtype 0\n");
  switch (plotType) {
    case PLOT_TYPE_VSWR:
      fprintf(gnuplotCommandsOutput, "set xlabel 'Frequency (MHz)'\n");
      fprintf(gnuplotCommandsOutput, "set ylabel 'SWR'\n");
      fprintf(gnuplotCommandsOutput, "plot '%s' smooth bezier title '%s'\n", 
        scanFileName, title);
      break;
    case PLOT_TYPE_FWD:
      fprintf(gnuplotCommandsOutput, "set xlabel 'Samples'\n");
      fprintf(gnuplotCommandsOutput, "set ylabel 'Forward Detector'\n");
      fprintf(gnuplotCommandsOutput, "plot '%s' smooth bezier, '%s' with points\n",
        scanFileName, scanFileName);
      break;
    case PLOT_TYPE_REV:
      fprintf(gnuplotCommandsOutput, "set xlabel 'Samples'\n");
      fprintf(gnuplotCommandsOutput, "set ylabel 'Reverse Detector'\n");
      fprintf(gnuplotCommandsOutput, "plot '%s' smooth bezier, '%s' with points\n",
        scanFileName, scanFileName);
      break;
  }
  fclose(gnuplotCommandsOutput);
    
  sprintf(gnuplotCommand, "gnuplot %s", gnuplotCommandsFileName);
  system(gnuplotCommand);

  unlink(gnuplotCommandsFileName);
  free(gnuplotCommandsFileName);
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
char title[linemax];
char term[linemax];
bool window = FALSE;
int plotType = PLOT_TYPE_VSWR;

  /* Initialise sensible defaults, etc. */
  progname = argv[0];
  strncpy(port, defport, portmax);

  tempScanFileName = allocateTempFileName();
  strcpy(scanFileName, tempScanFileName);


  plotFileName[0] = '\0';

  strcpy(title, "Unknown Antenna");
  strcpy(term, "canvas");
  
  /* Process command line options */
  for (i=1; i<argc; i++) {
    if (argv[i][0]=='-') {
      p=&argv[i][2]; // could be a buffer overflow ... if argv[i] is '-'
      switch (argv[i][1]) {
        case 'a':
          sscanf(p, "%ld", &startFreq);
          break;
        case 'b':
          sscanf(p, "%ld", &stopFreq);
          break;
        case 'd':
          switch (*p) {
            case 'f':
              plotType = PLOT_TYPE_FWD;
              break;
            case 'r':
              plotType = PLOT_TYPE_REV;
              break;
            default:
              usage();
          }
          break;
        case 'f':
          strncpy(scanFileName, p, fileNameMax);
          scanFileTemporary = FALSE;
          break;
        case 'm':
          strncpy(term, p, linemax);
          break;
        case 'n':
          sscanf(p, "%d", &numSteps);
          break;
        case 'o':
          strncpy(plotFileName, p, fileNameMax);
          break;
        case 'p':
          strncpy(port, p, portmax);
          /* XXX: check existence, deviceness? */
          break;
        case 's':
          sscanf(p, "%d", &settleDelay);
          break;
        case 't':
          strncpy(title, p, linemax);
          break;
        case 'v':
          verbose = TRUE;
          break;
        case 'w':
          window = TRUE;
          plotFileName[0] = '\0';
#if defined(MACOSX)
          strcpy(term, "aqua");
#endif
#if defined(REDHAT) || defined(DEBIAN) || defined(RASPBIAN)
          strcpy(term, "x11");
#endif
          break;
        default:
          usage();
      }
    }
    else
      usage();
  }

  // Are we plotting VSWR?
  if (plotType == PLOT_TYPE_VSWR && startFreq != 0L && stopFreq != 0L) {
    scan(verbose, port, startFreq, stopFreq, numSteps, settleDelay, scanFileName);

  // Are we measuring detector voltages?
  } else if ((plotType == PLOT_TYPE_FWD || plotType == PLOT_TYPE_REV) &&
              startFreq != 0L) {
    oscilloscope(verbose, port, startFreq, settleDelay, scanFileName, plotType);

  }

  // Are we plotting?
  // TODO Should allow interactive mode if term is qt, without having to specify a
  // plot file name.
  if ( plotFileName[0] != '\0' || // to a file
       window                     // to a window
     ) {
    plot(window, title, term, plotFileName, scanFileName, plotType);
  }

  if (verbose) {
    puts("Finished\n");
  }

  finish(0);
}

