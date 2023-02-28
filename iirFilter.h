#ifndef _IIRFILTER_H_
#define _IIRFILTER_H_

typedef struct {
    float passband[2];	/* corner frequencies of passband, Hz */
    long n; 		/* length of B and A */
    float *B, *A;	/* filter coefficients */
    float *B1, *A1;	/* filter coefficients prepared by initIirFilter */
} IIRFILTER;

/* These are the three filters used by ERMA */
IIRFILTER downsampleFilter, numerFilter, denomFilter;


int initIirFilter(IIRFILTER *ef,	/* in (ef->A,B) and out (ef->A1,B1) */
		  float **warmup);	/* out */
void iirFilter(IIRFILTER *ef,		/* in */
	       float *X,  long nX,	/* in */
	       float *warmup,		/* in & out; length 2n */
	       float *Y);		/* out; length n (same as X) */

#endif    /* _IIRFILTER_H_ */
