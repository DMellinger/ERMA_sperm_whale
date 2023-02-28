#ifndef _ERMAFILT_H_
#define _ERMAFILT_H_

extern IIRFILTER numerFilter, denomFilter;

void ermaFiltPrep();
void ermaDownsample(float *X,  int32_t nX,		/* in */
		    long decim,				/* in */
		    float *Y, int32_t *nY,		/* out */
		    float inSRate, float *outSRate);	/* in, out */
void ermaNumerDenomFilt(float *X, long nX,		/* in */
			float *numer, float *denom);	/* out */
void ermaFiltGetBandwidths(float *pNumerBW, float *pDenomBW);

#endif    /* _ERMAFILT_H_ */
