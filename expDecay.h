#ifndef _EXPDECAY_H_
#define _EXPDECAY_H_

void expDecay(float *x, int32 nX, float sRate, float *pPrev, float decayTime,
	      float warmTime, float ignoreThresh, float ignoreLimT, int doDiv);

#endif	/* _EXPDECAY_H_ */
