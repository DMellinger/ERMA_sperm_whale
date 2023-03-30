#ifndef _WISPRFILE_H_
#define _WISPRFILE_H_

/*#include <stdio.h>*/		/* for FILE */
/*#include <time.h>*/		/* for time_t */

#define WISPR_HEADER_SIZE	512
#define WISPR_BLOCK_SIZE	512

typedef struct wisprinfo {
    char wisprVersion[WISPR_HEADER_SIZE + 1];	/* from first line of file */
    float sRate;			/* samples/s */
    size_t nSamp;  			/* # samples in file */
    int32 sampleSize;			/* bytes per sample (usu. 2) */
    double timeE;			/* seconds since 1/1/1970 */
    int32 isWave;			/* 1 = WAVE file, 0 = WISPR */
    FILE *fp;				/* file descriptor for open file */
} WISPRINFO;

void wisprInitWISPRINFO(WISPRINFO *w);
WISPRINFO *wisprReadHeader(WISPRINFO *w, char *filename);
void wisprReadSamples(WISPRINFO *wi, float **pSnd, size_t *pSndSize);
size_t wisprReadFloat(float *sams, void *sndBuf, long offsetSam, size_t nSam,
		      WISPRINFO *wi);
void wisprPrintInfo(WISPRINFO *wi);
void wisprCleanup(WISPRINFO *wi);

#endif    /* _WISPRFILE_H_ */
