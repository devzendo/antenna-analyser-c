/*******************************************************************************
***
*** Filename         : util.h
*** Purpose          : Definitions of utility functions
*** Author           : Matt J. Gumbley
*** Created          : 20/01/97
*** Last updated     : 08/02/99
***
********************************************************************************
***
*** Modification Record
***
*******************************************************************************/

#ifndef UTIL_H
#define UTIL_H

#define CCITT_CRC_GEN 0x1021

#include <sys/types.h>

extern char *diagchar(int);
extern char hexdig(int);
extern void hexdump(unsigned char *, long);
extern u_int16_t read_word16(u_int8_t *);
extern u_int32_t read_word32(u_int8_t *);
extern void write_word16(u_int8_t *, u_int16_t);
extern void write_word32(u_int8_t *, u_int32_t);
extern u_int16_t crc(u_int8_t *, int);

#endif /* UTIL_H */
