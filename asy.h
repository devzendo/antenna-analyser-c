/*******************************************************************************
***
*** Filename         : asy.h
*** Purpose          : Definitions for the ASY physical layer in NCP
*** Author           : Matt J. Gumbley
*** Created          : 16/01/97
*** Last updated     : 29/12/98
***
********************************************************************************
***
*** Modification Record
***
*******************************************************************************/

#ifndef ASY_H
#define ASY_H

void asy_flush(int);
int  asy_test(int);
int  asy_getc(int);
int  asy_uputc(int,byte);
int  asy_write(int,byte*, int);
int  asy_open(char*,int);
void asy_close(int);

#endif /* ASY_H */

