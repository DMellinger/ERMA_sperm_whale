
#include "erma.h"

/* Given a vector x of length nX, compute an exponentially decaying average of
 * x. Note that the average itself is returned, not x minus the average. This is
 * the running average
 *		y(i) = alpha * y(i-1) + (1-alpha) * x(i)
 * where alpha is a decay constant. alpha is set such that a unit impulse will
 * decay to approximately 1/e in time decayTime. Before the averaging process is
 * started, the filter is "warmed up" for warmTime seconds; a good value to use
 * is decayTime*3, but if you don't want warmup, use warmTime=0.
 *
 * pPrev can be a pointer to the previous value returned, so as to continue
 * processing an ongoing signal on successive calls. If you don't need pPrev,
 * use NULL.
 *
 * The result (the running average) is returned in the input array x. (So make a
 * copy of x before calling this if you'll still need it.) If doDiv is non-zero,
 * the running average itself is not returned in x, but rather the input x
 * divided by the running average.
 *
 * If a sample is above ignoreThresh, it is ignored and the running average is
 * left as is for that sample. This allows abruptly loud sounds (like cetacean
 * clicks or glider motors) to not change the running average. However, if the
 * loud (above ignoreThresh) samples persist for more than ignoreLimT seconds,
 * they ARE used to update the threshold.
 */
void expDecay(float *x, int32 nX, float sRate, float *pPrev, float decayTime,
	      float warmTime, float ignoreThresh, float ignoreLimT, int doDiv)
{
    float prev;

    /* Calculate average over warmTime s to initialize pPrev. */
    int32 nWarm = round(warmTime * sRate);		//# of warmup samples
    if (pPrev == NULL) {
	prev = meanF(x, MIN(nX, MAX(1, nWarm)));
	pPrev = &prev;
    }
    float alpha = 1 - exp(-1 / (decayTime * sRate));	//decay per sample
    float runMean = *pPrev;
    int32 ignoreCount = 0;
    int32 ignoreLimSam = round(ignoreLimT * sRate);
    for (int32 i = 0; i < nX; i++) {
	if (x[i] <= runMean * ignoreThresh) {
	    runMean = (1.0 - alpha) * runMean + alpha * x[i];
	    ignoreCount = 0;
	} else {
	    if (ignoreCount < ignoreLimSam) {
		ignoreCount += 1;
	    } else {
		ignoreCount = 0;
		runMean = x[i];			//reset to current x-value
	    }
	}
	x[i] = doDiv ? x[i]/runMean : runMean;
    }
    *pPrev = runMean;
}
