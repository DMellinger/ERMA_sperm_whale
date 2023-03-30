#ifndef _ERMAFILT_H_
#define _ERMAFILT_H_

extern IIRFILTER numerFilter, denomFilter;

void ermaFiltPrep();
void ermaDownsample(float *X,  int32 nX,		/* in */
		    int32 decim,			/* in */
		    float *Y, int32 *nY,		/* out */
		    float inSRate, float *outSRate);	/* in, out */
void ermaNumerDenomFilt(float *X, int32 nX,		/* in */
			float *numer, float *denom);	/* out */
void ermaFiltGetBandwidths(float *pNumerBW, float *pDenomBW);

#endif    /* _ERMAFILT_H_ */
