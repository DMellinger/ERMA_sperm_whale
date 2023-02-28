#ifndef _ERMA_H_
#define _ERMA_H_


/* About times in this ERMA project:
 * Time values are stored in three ways:
 *   (1) Seconds since the start of a file; variables that use this end in S,
 *       like timeS.
 *   (2) Seconds since the Epoch (1/1/1970), like Unix does; variables that
 *       use this end in E, like clickE.
 *   (3) Days since the Epoch (1/1/1970), like MATLAB does except with a
 *       different epoch date; variables that use this end in D, like minTimeD.
 */


/* This file is included by every module in the ERMA project. It has ALL the
 * #includes needed by any module, which (a) makes it easy to set up the
 * makefile, but (b) means that whenever you touch any .h file, everything gets
 * recompiled. But there are few enough modules that recompiling everything
 * doesn't take inordinately long.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>		/* for int32_t etc. */
#include <string.h>
#include <math.h>
#include <time.h>		/* for time_t */
#include <sys/stat.h>		/* for stat() */
#include <sys/param.h>		/* for MIN and MAX */
#include <dirent.h>
#include <glob.h>
#include <float.h>

/* Make some less-unwieldy synonyms. */
#define  int16   int16_t
#define uint16	uint16_t
#define  int32   int32_t
#define uint32	uint32_t

/* The #includes for the ERMA project: */
#include "gpio.h"
#include "ermaGoodies.h"
#include "wisprFile.h"
#include "wavFile.h"
#include "ermaErrors.h"
#include "ermaConfig.h"
#include "iirFilter.h"
#include "ermaFilt.h"
#include "quietTimes.h"
#include "ermaNew.h"
#include "processFile.h"
#include "encounters.h"
#include "expDecay.h"

#endif	/* _ERMA_H_ */
