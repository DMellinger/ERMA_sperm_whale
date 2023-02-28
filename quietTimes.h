#ifndef _QUIETTIMES_H_
#define _QUIETTIMES_H_

/*#include <stdlib.h>*/
/*#include "ermaConfig.h"		*//* for ERMAPARAMS */


/* This holds start- and stop-times in a file, in both samples and seconds from
 * the start of the file.
 */
typedef struct {
    union {
	int32 sam[2];		    /* can refer to sam[0] and sam[1], or... */
	struct {int32 sam0, sam1;}; /* ...sam0 and sam1 */
    };	
    union {
	float t[2];		    /* can refer to t[0] and t[1], or... */
	struct {float t0, t1;};	    /* ...t0 and t1 */
    };
} TIMESPAN;


typedef struct
{   
    TIMESPAN *tSpan;/* start- and stop-times/samples of quiet periods in file */
    long n;	    /* number of valid elements in t */
    size_t tSpanSize;/* for bufgrow() */
} QUIETTIMES;

void initQUIETTIMES(QUIETTIMES *qt);
void resetQuietTimes(QUIETTIMES *qt);
QUIETTIMES *findQuietTimes(float *snd, long nSamp, float sRate, ERMAPARAMS *ep,
		    QUIETTIMES *qt);
void printQuietTimes(QUIETTIMES *qt);

#endif	/* _QUIETTIMES_H_ */
