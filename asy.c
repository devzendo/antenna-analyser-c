/*******************************************************************************
***
*** Filename         : asy.c
*** Purpose          : Linux implementation of NCP ASY routines
*** Author           : Matt J. Gumbley
*** Created          : 16/01/97
*** Last updated     : 20/01/97
***
*** Notes            : The following preprocessor symbols are used:
***                    DEBUG - to include dump code.
***
********************************************************************************
***
*** Modification Record
***
*******************************************************************************/

#define DataTimeout 20

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "global.h"
#include "asy.h"
#include "util.h"

static byte PendingDataBuffer;
static int PendingData = FALSE;
static struct termios OriginalSerialParameters;


/*******************************************************************************
***
*** Function         : asy_open opens the specified port with the specified 
***                    characteristics.
*** Preconditions    : Port is a string like "/dev/ttyS0"
***                    Baud is B300 to B38400
***                    Handshake is XONXOFF, RTSCTS
*** Postconditions   : asy_open is positive, and the open worked. (8/N/1 with
***                    hardware handshaking.
***                    The returned value is the file descriptor for access.
***                    asy_open is -1, and the open failed.
***
*******************************************************************************/

int asy_open(char *Port, int Baud)
{
  struct termios SerialParameters;
  int i, fd;

  /* We want to open the port in nodelay mode, so we are informed if the
     port cannot be opened. */
  if ((fd = open (Port, O_RDWR | O_NDELAY)) == -1) {
#ifdef DEBUG
    fprintf (stderr, "asy_open: Cannot open port %s\n", Port);
#endif
    return -1;
  }

  /* Now change back to delayed mode */
  if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) & (~O_NDELAY)) == -1) {
#ifdef DEBUG
    fprintf (stderr, "asy_open: Cannot change O_NDELAY on port %s\n",Port);
#endif
    return -1;
  }

  /* Now change the rest of the parameters - non-canonical input, etc. */
  if (tcgetattr (fd, &SerialParameters) == -1) {
#ifdef DEBUG
    fprintf (stderr, "asy_open: Cannot tcgetattr on port %s. Errno = %d\n", Port, errno);
#endif
    return -1;
  }

  (void) tcgetattr (fd, &OriginalSerialParameters);

  SerialParameters.c_cflag = Baud | CS8 | CLOCAL | CREAD | CRTSCTS;
  SerialParameters.c_lflag = 0;
  SerialParameters.c_oflag = 0;
  SerialParameters.c_iflag = IGNBRK | IGNPAR;
  for (i = 0; i < NCCS; i++)
    SerialParameters.c_cc[i] = (unsigned char) 0;

  SerialParameters.c_cc[VMIN] = (unsigned char) 0;
  SerialParameters.c_cc[VTIME] = (unsigned char) DataTimeout;

  if (tcsetattr (fd, TCSANOW, &SerialParameters) == -1) {
#ifdef DEBUG
    fprintf (stderr, "asy_open: Cannot tcsetattr on port %s. Errno = %d\n", Port, errno);
#endif
    return -1;
  }

  PendingData = FALSE;
  return fd;
}


/*******************************************************************************
***
*** Function         : asy_close(fd)
*** Precondition     : fd is open
*** Postcondition    : The port is reset to its original settings, and is 
***                    closed.
***
*******************************************************************************/

void asy_close(int fd)
{
  if (tcsetattr (fd, TCSANOW, &OriginalSerialParameters) == -1) {
#ifdef DEBUG
    fprintf (stderr, "asy_close: Cannot reset with tcsetattr. Errno = %d\n", errno);
#endif
  }
  else {
#ifdef DEBUG
    fprintf (stderr, "asy_close: Port closed.\n");
#endif
  }
  close(fd);
}


/*******************************************************************************
***
*** Function         : asy_uputc
*** Precondition     : fd is open.
*** Postcondition    : asy_uputc is TRUE and the data has been sent.
***                    asy_uputc is FALSE and the data has not been sent.
***                    Boolean status indicates success.
***
*******************************************************************************/

int asy_uputc(int fd, byte Data)
{
  byte Buffer = Data;
  int Status;
  if (fd == 0) {
#ifdef DEBUG
    fprintf(stderr, "asy_uputc: Port not open\n");
#endif
    return FALSE;
  }
#ifdef DEBUG
  fprintf(stderr, "asy_uputc: Put %s ..", diagchar(Buffer));
#endif
  /* Re-write until we are not affected by a signal. */
  while ((Status = write (fd, &Buffer, 1)) == -1 && errno == EINTR) ;
  if (Status != 1) {
#ifdef DEBUG
    fprintf (stderr, "asy_uputc: Write returned %d. Errno=%d\n", Status, errno);
#endif
    return FALSE;
  }
  return TRUE;
}


/*******************************************************************************
***
*** Function         : asy_write
*** Precondition     : fd is open.
*** Postcondition    : asy_write is positive and the data has been sent. The
***                    return value indicates how many bytes have been sent.
***                    asy_uputc is FALSE and the data has not been sent.
***
*******************************************************************************/

int asy_write(int fd, byte *Data, int len)
{
  int Status;
  if (fd == 0) {
#ifdef DEBUG
    fprintf(stderr, "asy_write: Port not open\n");
#endif
    return FALSE;
  }
#ifdef DUMP
  fprintf(stderr, "asy_write: dump:\n");
  hexdump(Data, len);
#endif
  /* Re-write until we are not affected by a signal. */
  while ((Status = write (fd, Data, len)) == -1 && errno == EINTR) ;
  if (Status != len) {
#ifdef DEBUG
    fprintf (stderr, "asy_write: Write returned %d. Errno=%d\n", Status, errno);
#endif
    return FALSE;
  }
  return Status;
}


/*******************************************************************************
***
*** Function         : asy_flush
*** Precondition     : fd is open.
*** Postcondition    : The port is flushed.
***
*******************************************************************************/

void asy_flush(int fd)
{
  char Trashcan;
  if (fd == 0) {
#ifdef DEBUG
    fprintf(stderr, "asy_flush: Port not open\n");
#endif
    return;
  }
  /* Switch to nodelay mode for a sec */
  (void) fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NDELAY);
  /* Exhaust all input data */
  while (read (fd, &Trashcan, 1) == 1) {
#ifdef DEBUG
    fprintf (stderr, "asy_flush: Flushed character %d\n", Trashcan);
#endif
  }
  /* Now switch back to delayed action */
  (void) fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) & (~O_NDELAY));
  PendingData = FALSE;
}


/*******************************************************************************
***
*** Function         : asy_getc()
*** Precondition     : fd is open.
*** Postcondition    : asy_getc returns a positive value, the character read.
***                    asy_getc returns a negative value, indicating a timeout.
***
*******************************************************************************/

int asy_getc(int fd)
{
  byte Buffer;
  int Status;
  struct tms Timest;
  clock_t Interval;
  if (fd == 0) {
#ifdef DEBUG
    fprintf(stderr, "asy_getc: Port not open\n");
#endif
    return -1;
  }
  /* Is any data pending from an asy_test? */
  if (PendingData) {
    PendingData = FALSE;
#ifdef DEBUG
    fprintf(stderr,"asy_getc: Pending char : %s\n", diagchar(PendingDataBuffer));
#endif
    return (int) PendingDataBuffer;
  }

  /* Re-read until we are not affected by a signal. */
  do {
    Interval = times (&Timest);
    Status = read (fd, &Buffer, 1);
    Interval = times (&Timest) - Interval;
  }
  while (Status == -1 && errno == EINTR);
  if (Status != 1) {
#ifdef DEBUG
    fprintf (stderr, "asy_getc: Read returned %d. Errno=%d Interval=%ld\n", 
             Status, errno, (long) Interval);
#endif
    return -1;
  }
#ifdef DEBUG
  fprintf(stderr,"asy_getc: Read char : %s\n", diagchar(Buffer));
#endif
  return (int) Buffer;
}


/*******************************************************************************
***
*** Function         : asy_test
*** Preconditions    : fd is open.
*** Postconditions   : asy_test is TRUE, and there is data to be read.
***                    asy_test is FALSE, and there is no data to be read.
***
*******************************************************************************/

int asy_test(int fd)
{
#ifdef DEBUG
  static int LastPendingData = 999;
#endif
  if (fd == 0) {
#ifdef DEBUG
    fprintf(stderr, "asy_test: Port not open\n");
#endif
    return -1;
  }
  if (PendingData) {
#ifdef DEBUG
    printf("asy_test: Previous data pending\n");
#endif
    return TRUE; /* There's something from last time, still... */
  }
  /* Switch to nodelay mode for a sec */
  (void) fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NDELAY);
  /* Is there anything to read? */
  PendingData = (read (fd, &PendingDataBuffer, 1) == 1);
#ifdef DEBUG
  printf("asy_test: PendingData is %d\n",PendingData);
  if (PendingData) {
    printf("asy_test: data available %s\n",diagchar(PendingDataBuffer));
  }
  else {
    if (LastPendingData != PendingData) {
      printf("asy_test: no data\n");
    }
  }
  LastPendingData = PendingData;
#endif
  /* Now switch back to delayed action */
  (void) fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) & (~O_NDELAY));
  return PendingData;
}

