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


/* This flag says whether we're compiling this on a Raspberry Pi system.
 * (Initially the code is being developed in Windows.)
 */
#define ON_RPI	__arm__	  //say "#if ON_RPI" to use (NOT #ifdef!!)


/**********************************************************************/
/* These are various debugging flags that can be turned on and off independently
 * by commenting/uncommenting them out. Use them with "#ifdef <FLAGNAME>".
 * They're always off when compiled on the RPi.
 */
#if !ON_RPI && 0
//Save temp-numerPow.flt, temp-ratio.flt, temp-normPowNumer.flt, etc.
#define DEBUG_SAVE_ARRAYS

//Save the click neighborhoods in temp-ratioNbds.csv.
#define DEBUG_SAVE_NBDS


#endif	//if ON_RPI
/**********************************************************************/



/* A TIMESPAN holds start- and end-times of something. The values can be any of
 * the types of times mentioned above: seconds since start of a file (names
 * ending in S), seconds since the Epoch (names ending in E), or days since the
 * Epoch (names ending in D).
 */
typedef union {
    double t[2];		//start- and stop-times, in seconds or days
    struct {
	double t0, t1;		//the same thing, as separate named variables
    };
} TIMESPAN;
#define TIMESPAN_DUR(timespan)	((timespan).t1 - (timespan).t0)

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
#include <unistd.h>
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
#define  int64	 int64_t
#define uint64	uint64_t

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
