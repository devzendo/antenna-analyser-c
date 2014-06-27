/*******************************************************************************
***
*** Filename         : global.h
*** Purpose          : Some global definitions
*** Author           : Matt J. Gumbley
*** Created          : 19/01/97
*** Last updated     : 26/01/97
***
********************************************************************************
***
*** Modification Record
***
*******************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

#include "config.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if defined(HAVE_SIZED_INT_TYPES)
#include <sys/types.h>
typedef u_int8_t byte;
typedef u_int16_t word16;
typedef u_int32_t word32;
#else
/* Shouldn't happen */
#error Cannot compile program without sized int types!
#endif

typedef int bool;

#endif /* GLOBAL_H */

