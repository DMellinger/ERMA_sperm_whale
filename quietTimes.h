#ifndef _QUIETTIMES_H_
#define _QUIETTIMES_H_

/*#include <stdlib.h>*/
/*#include "erma.h.h"		*//* for TIMESPAN */
/*#include "ermaConfig.h"	*//* for ERMAPARAMS */


/* This holds start- and stop-times in a file, in both samples and seconds from
 * the start of the file.
 */
typedef struct {
    union {
	int64_t sam[2];			//can refer to sam[0] and sam[1], or...
	struct {int64_t sam0, sam1;};	//...sam0 and sam1 */
    };
    TIMESPAN tS;			//same information, but in seconds
} TIMESPAN_S;


/* This holds the set of quiet times in a file.
 */
typedef struct {   
    TIMESPAN_S *tSpan;	//start- and stop-times & -sams of quiet periods in file
    size_t tSpanSize;	//for bufgrow()
    int32 n;		//number of valid elements in tSpan
} QUIETTIMES;


void initQUIETTIMES(QUIETTIMES *qt);
void resetQuietTimes(QUIETTIMES *qt);
QUIETTIMES *findQuietTimes(float *snd, size_t nSamp, float sRate,
			   ERMAPARAMS *ep, QUIETTIMES *qt, char *baseDir);
void printQuietTimes(QUIETTIMES *qt);

#endif	/* _QUIETTIMES_H_ */
