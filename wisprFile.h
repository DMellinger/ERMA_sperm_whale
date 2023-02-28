#ifndef _WISPRFILE_H_
#define _WISPRFILE_H_

/*#include <stdio.h>*/		/* for FILE */
/*#include <time.h>*/		/* for time_t */

#define WISPR_HEADER_SIZE	512
#define WISPR_BLOCK_SIZE	512
#define WISPR_SAMPLE_SIZE	2	/* bytes per sample */

typedef struct wisprinfo {
    char wisprVersion[WISPR_HEADER_SIZE + 1];	/* from first line of file */
    float sRate;			/* samples/s */
    long nSamp;  			/* # samples in file */
    long sampleSize;			/* bytes per sample (usu. 2) */
    double timeE;			/* seconds since 1/1/1970 */
    long isWave;			/* 1 = WAVE file, 0 = WISPR */
    FILE *fp;				/* file descriptor for open file */
} WISPRINFO;

void wisprInitWISPRINFO(WISPRINFO *w);
WISPRINFO *wisprReadHeader(WISPRINFO *w, char *filename);
void wisprReadSamples(WISPRINFO *wi, float **pSnd, size_t *pSndSize);
long wisprReadData(int16_t **bufp, WISPRINFO *w, long offsetSam, long nSamToRead);
void wisprPrintInfo(WISPRINFO *wi);
void wisprCleanup(WISPRINFO *wi);

#endif    /* _WISPRFILE_H_ */
