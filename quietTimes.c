
#include "erma.h"

/* defined below */
void checkQuiet(int64 i0, int64 i1, float blockDurS, int64 blockLen,
		int32 nBlocks, float sRate, ERMAPARAMS *ep, QUIETTIMES *qt);



void initQUIETTIMES(QUIETTIMES *qt)
{
    qt->tSpan = NULL;
    qt->n = 0;
    qt->tSpanSize = 0;
}


/* Reset a QUIETTIMES so it can be reused. This leaves the allocated storage
 * (qt->tSpan) in place but resets the count of how many items are in qt.
 */
void resetQuietTimes(QUIETTIMES *qt)
{
    qt->n = 0;
}


/* Given a signal snd, find the sections of time when glider motors aren't
 * running and things are reasonably quiet.
 */
QUIETTIMES *findQuietTimes(float *snd, size_t nSamp, float sRate,
			   ERMAPARAMS *ep, QUIETTIMES *qt)
{
    int32 blockLen = round(ep->ns_tBlockS * sRate);  /* block len in samples */
    float blockDurS = blockLen / sRate;		/* block len in s */
    int32 nBlocks = nSamp / blockLen;		/* number of blocks in snd */
    static float *avgPower = NULL;
    static size_t avgPowerSize = 0;		/* for bufgrow */

    BUFGROW(avgPower, nBlocks, ERMA_NO_MEMORY_AVGPOWER);
    for (int32 i = 0, p = 0; i < nBlocks; i++, p += blockLen) {
	/* find DC offset = average of all the samples */
	double sum = 0.0; /* double, as it's sum of potentially many floats */
	for (int32 j = p; j < p+blockLen; j++)
	    sum += (double)snd[j];
	float avgSam = (float)(sum / (double)blockLen);

	/* Find average power of this block = sum(samples^2)/nSamples */
	sum = 0.0;
	for (int32 j = p; j < p+blockLen; j++) {
	    float val = snd[j] - avgSam;
	    sum += (double)(val * val);
	}
	avgPower[i] = (float)(sum / (double)blockLen);
    }
#if !ON_RPI    
    printSignal(avgPower, nBlocks, "temp-avgPower.csv");
#endif

    /* Find noise: runs of minConsec blocks that are all above threshold. */
    int32 minConsec = roundf(ep->ns_tConsecS / blockDurS);
    int32 padBlock = ep->ns_padSec / blockDurS;	/* amount to pad in blocks */
    int inNoise = 0;		/* currently in long-enough noise section? */
    int32 quietStart = 0;	/* start ix of most recent non-noise section */
    int32 n = 0;		/* number of consecutive noise blocks seen */
    for (int32 i = 0; i < nBlocks; i++) {
	if (avgPower[i] >= ep->ns_thresh) {
	    /* Above noise threshold; check if already in noise section. */
	    n += 1;
	    if (n == minConsec && !inNoise) {
		/* This noise section is long enough to count. Try adding the
		 * quiet section that just ended because of this noise. */
		inNoise = 1;
		checkQuiet(quietStart, i - n + 1 - padBlock,
			   blockDurS, blockLen, nBlocks, sRate, ep, qt);
	    }
	} else {
	    /* Below threshold; if in noise, end it. */
	    if (inNoise) {
		/* Was in noise section; now at start of quiet. */
		quietStart = i + padBlock;
	    }
	    n = 0;
	    inNoise = 0;
	}
    }
    if (!inNoise) {
	checkQuiet(quietStart, nBlocks - (n>0 ? n+padBlock : 0) + 1,
		   blockDurS, blockLen, nBlocks, sRate, ep, qt);
    }
}


/* Check a possible quiet section, which (with padding already added) starts at
 * block i0 and ends at block i1, to see if it's long enough. If it is, add it
 * to the QUIETTIMES list.
 */
void checkQuiet(int64 i0, int64 i1, float blockDurS, int64 blockLen,	//in
		int32 nBlocks, float sRate, ERMAPARAMS *ep,		//in
		QUIETTIMES *qt)					//in & out
{
    float dur = (i1 - i0) * blockDurS;

    if (dur >= ep->ns_minQuietS) {
	BUFGROW(qt->tSpan,   qt->n + 1, ERMA_NO_MEMORY_QUIETTIME);
	qt->tSpan[qt->n].sam0  = MAX(i0, 0)       * blockLen;
	qt->tSpan[qt->n].sam1  = MIN(i1, nBlocks) * blockLen;
	qt->tSpan[qt->n].tS.t0 = qt->tSpan[qt->n].sam0 / sRate;
	qt->tSpan[qt->n].tS.t1 = qt->tSpan[qt->n].sam1 / sRate;
	qt->n++;
    }
}


/* For debugging */
void printQuietTimes(QUIETTIMES *qt)
{
    printf("Quiet times: \n");
    for (int32 i = 0; i < qt->n; i++) {
	printf("    %.3f-%.3f (sams %d-%d)\n", qt->tSpan[i].tS.t0,
	       qt->tSpan[i].tS.t1, qt->tSpan[i].sam0, qt->tSpan[i].sam1);
    }
}



#ifdef NEVER
/* The equivalent MATLAB code. This finds noise sections, not quiet sections. */
% Calculate average power in each time block. The DC offset in each block
% (average sample value in that block) is removed first.
sndDur = length(x) / sRate;			% signal duration, s
blockLen = round(p.ns_tBlockS * sRate);		% # samples per block
nBlocks = floor(nRows(x) / blockLen);		% number of blocks
blockDurS = blockLen / sRate;			% might not quite equal tBlockS
x = reshape(x(1 : blockLen*nBlocks), blockLen, nBlocks);
x = x - ones(blockLen,1) * mean(x,1);		% remove DC offset
avgPower = sum(x.^2,1) / blockLen;

% Find runs of minConsec blocks that are all above threshold. The result,
% noiseTimesS, is a 2xN array with start- and stop-times of when noise is
% present. Times are measured from the start of the file.
minConsec = round(p.ns_tConsecS / blockDurS);
cIx = consec(find(avgPower >= p.ns_thresh));	% indices of consecutive blocks
cIxTrim = cIx(:, diff(cIx,1,1) >= minConsec);	% ...that are long enough
noiseTimesS = (cIxTrim - 1) * blockDurS;	% -1 handles 1-based indexing

% Apply padding.
if (~isempty(cIxTrim))
  pd = repmat([-1;1] * p.ns_padSec, 1, nCols(cIxTrim));
  noiseTimesS = max(0, min(sndDur, noiseTimesS + pd));
end

#endif
