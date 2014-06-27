/*******************************************************************************
***
*** Filename         : util.c
*** Purpose          : Utility functions
*** Author           : Matt J. Gumbley
*** Created          : 20/01/97
*** Last updated     : 20/02/02
***
********************************************************************************
***
*** Modification Record
*** 20/02/02 MJG Build fixed for RH7.x, from a patch by Grant Edwards.
***
*******************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "global.h"
#include "util.h"

/*******************************************************************************
***
*** Function         : diagchar(ch)
*** Preconditions    : ch is an ASCII character
*** Postconditions   : The ASCII name of the character is returned.
***
*******************************************************************************/

char *diagchar(int ch)
{
  static char buf[40];
  char buf1[20];
  char *str;
  switch(ch) {
    case 0 : str="NUL"; break;
    case 1 : str="SOH"; break;
    case 2 : str="STX"; break;
    case 3 : str="ETX"; break;
    case 4 : str="EOT"; break;
    case 5 : str="ENQ"; break;
    case 6 : str="ACK"; break;
    case 0x15 : str="NAK"; break;
    default: sprintf(buf1,"'%c' (%d, 0x%02X)",isprint(ch) ? ch : '?', ch, ch); str=buf1; break;
  }
  strcpy(buf,str);
  return buf;
}

/*******************************************************************************
***
*** Function         : hexdig
*** Preconditions    : 0 <= num <= 15
*** Postconditions   : num is converted into its corresponding hexadecimal digit
***
*******************************************************************************/

char hexdig(int num)
{
static char digs[]="0123456789ABCDEF";
  return num < 16 ? digs[num] : '?';
}

/*******************************************************************************
***
*** Function         : hexdump
*** Preconditions    : buf points to an area of memory and len is the length of
***                    this area
*** Postconditions   : An offset | hexadecimal | printable ASCII dump of the
***                    designated memory area is output
***
*******************************************************************************/

void hexdump(unsigned char *buf, long buflen)
{ 
char LINE[80];
long offset=0L;
long left=buflen;
int i,upto16,x;
unsigned char b;
  while (left>0) {
    for (i=0; i<78; i++)
      LINE[i]=' ';
    LINE[9]=LINE[59]='|';
    LINE[77]='\0';
    sprintf(LINE,"%08lX",offset);
    LINE[8]=' ';
    upto16 = (left>16) ? 16 : (int)left;
    for (x=0; x<upto16; x++) {
      b = buf[offset+x];
      LINE[11+(3*x)]=hexdig((b&0xf0)>>4);
      LINE[12+(3*x)]=hexdig(b&0x0f);
      LINE[61+x]=isprint((char)b) ? ((char)b) : '.';
    }
    puts(LINE);
    offset+=16L;
    left-=16L;
  }
}

/*******************************************************************************
***
*** Function         : read_word16
*** Preconditions    : arr is an array of bytes
*** Postconditions   : read_word16 returns the first two bytes at arr, 
***                    converted to little endian order.
***
*******************************************************************************/

word16 read_word16(byte *arr)
{
  return arr[0] | (arr[1] << 8);
}

/*******************************************************************************
***
*** Function         : read_word32
*** Preconditions    : arr is an array of bytes
*** Postconditions   : read_word32 returns the first four bytes at arr, 
***                    converted to little endian order.
***
*******************************************************************************/

word32 read_word32(byte *arr)
{
  return arr[0] | (arr[1] << 8) | (arr[2] << 16) | (arr[3] << 24);
}

/*******************************************************************************
***
*** Function         : write_word16
*** Preconditions    : arr is an array of bytes, w16 is a 16-bit word
*** Postconditions   : write_word16 stores w16 as the first two bytes at arr, 
***                    in little endian order.
***
*******************************************************************************/

void write_word16(byte *arr, word16 w16)
{
  *arr++ = (w16 & 0x00ff);
  *arr++ = (w16 & 0xff00) >> 8;
}

/*******************************************************************************
***
*** Function         : write_word32
*** Preconditions    : arr is an array of bytes, w32 is a 32-bit word
*** Postconditions   : write_word32 stores w32 as the first four bytes at arr, 
***                    in little endian order.
***
*******************************************************************************/

void write_word32(byte *arr, word32 w32)
{
  *arr++ = (w32 & 0x000000ff);
  *arr++ = (w32 & 0x0000ff00) >> 8;
  *arr++ = (w32 & 0x00ff0000) >> 16;
  *arr++ = (w32 & 0xff000000) >> 24;
}


/*******************************************************************************
***
*** Function         : crc(buf, len)
*** Purpose          : Calculates the 16-bit CCITT checksum with polynomial 
***                    x^16+x^12+x^5+1 (i.e. 0x1021) for the indicated buffer
***
*******************************************************************************/

word16 crc(byte *buf, int len)
{
register word16 CRC = 0;
word16 databyte;
int i;
  while (len--) {
    databyte = *buf++ << 8;
    for (i=8; i>0; i--) {
      if ((databyte ^ CRC) & 0x8000)
        CRC = (CRC << 1) ^ CCITT_CRC_GEN;
      else
        CRC <<= 1;
      databyte <<= 1;
    }
  }
  return (CRC);
}

