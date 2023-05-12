
#include "erma.h"


/* Defined below */
static void calcAverageRatio(float *num, float *den, int32 nNum,
			     int32 avgSam, float *ratio, int32 *pNRatio,
			     float *numAvg, float *denAvg);
int32 peakNear(float *x, int32 nX, int32 ix, int32 nbdSam);
void findClicks(float *x, int32 nX, float segT0, float *ratio, int32 nRatio,
		float sRate, ERMAPARAMS *ep, int32 delaySam, float bwNumer,
		FILECLICKS *fc, float *seg, size_t nSeg, float segSRate);
void addSpec(CLICKSPEC cs, float t, float *seg, size_t nSeg,
	     float sRate, size_t nSpecSams, float *win);
void writeFILECLICKS(FILECLICKS *fc, char *filename);
void writeALLCLICKS(ALLCLICKS *ac, char *filename);



/* Prepare a new FILECLICKS for use.
 */
void initFILECLICKS(FILECLICKS *fc)
{
    fc->timeS = NULL;		/* times of clicks in file, s */
    fc->timeSSize = 0;		/* for bufgrow */
    fc->n = 0;			/* number of clicks found this file */
}


/* Prepare a FILECLICKS for re-use with a different file.
 */
void resetFILECLICKS(FILECLICKS *fc)
{
    /* Leave timeS as long as needed */
    fc->n = 0;
}


void initALLCLICKS(ALLCLICKS *ac)
{
    ac->timeD = NULL;
    ac->timeDSize = 0;
    ac->n = 0;
}


/* Iterate through the quiet time segments in qt, running ERMA on each segment.
 * Results are in fc, as times in seconds in snd at which a click occurred.
 */
void ermaSegments(float *snd, WISPRINFO *wi, ERMAPARAMS *ep, QUIETTIMES *qt,
		  FILECLICKS *fc)
{
    int i;
    int32 i0, i1;

    resetFILECLICKS(fc);
    for (i = 0; i < qt->n; i++) {
	i0 = qt->tSpan[i].sam0;
	i1 = qt->tSpan[i].sam1;
	ermaNew(&snd[i0], i1 - i0, qt->tSpan[i].tS.t0, wi->sRate, ep, fc);
    }
}


/* Run the ERMA process on a segment of snd for nSam samples. Results (click
 * detections) are left in fc.
 */
void ermaNew(float *seg, int32 nSeg, float segT0, float sRate, ERMAPARAMS *ep,
	     FILECLICKS *fc)
{
    static float *x = NULL;	/* the decimated signal */
    static size_t xSize = 0;
    int32 nX, nRatio;
    float bwNumer, bwDenom, newSRate;

    /* Decimate the signal. x ends up 1/ep->decim as long as seg, but during
     * filtering it needs to be as long as seg, so nSeg is used here. */
    BUFGROW(x, nSeg, ERMA_NO_MEMORY_DECIMBUF);
    float origSRate = sRate;
    ermaDownsample(seg, nSeg, ep->decim, x, &nX, sRate, &newSRate);
    sRate = newSRate;

    /* Do the ERMA filtering: calculate the numerator signal, the denominator
     * signal, and (eventually) their power ratio. */
    static float *numer = NULL, *denom = NULL, *ratio = NULL;
    static size_t numerSize = 0, denomSize = 0, ratioSize = 0;
    BUFGROW(numer, nX, ERMA_NO_MEMORY_NUMER_DENOM);
    BUFGROW(denom, nX, ERMA_NO_MEMORY_NUMER_DENOM);
    BUFGROW(ratio, nX, ERMA_NO_MEMORY_NUMER_DENOM);
    ermaNumerDenomFilt(x, nX, numer, denom); //numer, denom same length as x

#ifdef DEBUG_SAVE_ARRAYS
    printf("ermaNew: writing temp signal files\n");
    writeFloatArray(seg, nSeg, "temp-x.flt");
    writeFloatArray(x, nX, "tempY-downSampled.flt");
    char fname[256];
    sprintf(fname, "tempY-downSampled.b%d", (long)round(sRate / 100));
    writeShortFromFloat(x, nX, fname);
    writeFloatArray(numer, nX, "temp-numer.flt");
    writeFloatArray(denom, nX, "temp-denom.flt");
#endif

    ermaFiltGetBandwidths(&bwNumer, &bwDenom);
    bwNumer /= 1000.0;		/* make Hz into kHz */
    bwDenom /= 1000.0;		/* make Hz into kHz */
    
    int32 filtDelaySam = (numerFilter.n + 1) / 2;
/*    float filtDelayT = (float)filtDelaySam / sRate;*/

    /* Convert filtered signals to power per kHz of bandwidth, re-using numer
     * and denom for this (called powNumer and powDenom in * MATLAB). */
    float bwNumerInv = 1.0 / bwNumer;
    float bwDenomInv = 1.0 / bwDenom;
    for (int32 i = 0; i < nX; i++) {
	numer[i] = numer[i] * numer[i] * bwNumerInv;
	denom[i] = denom[i] * denom[i] * bwDenomInv;
    }
    /* Compute power ratio while power is in numer and denom. */
    float avgSam = round(ep->avgT * sRate);	// # samples to average over
    #if !ON_RPI
    static float *numAvg = NULL, *denAvg = NULL;
    static size_t numAvgSize = 0, denAvgSize = 0;
    BUFGROW(numAvg, nX, ERMA_NO_MEMORY_NUMER_DENOM_AVG);
    BUFGROW(denAvg, nX, ERMA_NO_MEMORY_NUMER_DENOM_AVG);
    calcAverageRatio(numer, denom, nX, avgSam, ratio, &nRatio, numAvg, denAvg);
    #else
    calcAverageRatio(numer, denom, nX, avgSam, ratio, &nRatio, NULL, NULL);
    #endif
    //Delay from numer to ratio, i.e., numer[i] aligns with ratio[i - delaySam]
    int32 delaySam = avgSam / 2;

#ifdef DEBUG_SAVE_ARRAYS
    printf("ermaNew: writing temp files starting at %.5f s\n", segT0);
    writeFloatArray(numer,  nX, "temp-numerPow.flt");
    writeFloatArray(denom,  nX, "temp-denomPow.flt");
    writeFloatArray(ratio,  nX, "temp-ratio.flt");
    writeFloatArray(numAvg, nX-avgSam+1, "temp-numAvg.flt");
    writeFloatArray(denAvg, nX-avgSam+1, "temp-denAvg.flt");
    writeFloatArray(ratio,  nX, "temp-ratio.flt");
#endif
    /* Exponentially decay numer (which is power per kHz of bandwidth), dividing
     * by running mean and leaving result in numer. Result (i.e., numer) is
     * called normPowNumer in MATLAB. */
    expDecay(numer, nX, sRate, NULL, ep->decayTime, ep->decayTime,
	     ep->ignoreThresh / bwNumer, ep->ignoreLimT, 1);

#ifdef DEBUG_SAVE_ARRAYS
    //Same normPowNumer (here in numer).
    writeFloatArray(numer, nX, "temp-normPowNumer.flt");
#endif

    /* Find clicks - peaks in numer (which is normPowNumer) that also satisfy
     * the ERMA ratio test. Click detection times are put into fc. */
    int32 startClickNo = fc->n;
    findClicks(numer, nX, segT0, ratio, nRatio, sRate, ep, delaySam, bwNumer,
	       fc, seg, nSeg, origSRate);
}

/*
nyquistFreq = sRate / 2;
[numerFilt.B,numerFilt.A] = butter(p.filterOrder, p.f_numer / nyquistFreq);
[denomFilt.B,denomFilt.A] = butter(p.filterOrder, p.f_denom / nyquistFreq);
bwNumer = diff(p.f_numer / 1000);	% bandwidth of  numerator  band, kHz
bwDenom = diff(p.f_denom / 1000);	% bandwidth of denominator band, kHz

% Filter.
sndNumer = filter(numerFilt.B, numerFilt.A, snd);
sndDenom = filter(denomFilt.B, denomFilt.A, snd);
filtDelaySam = (p.filterOrder + 1) / 2;
filtDelayT = filtDelaySam / sRate;
% Variables affected by filtDelayT (see also avgDelayT below): sndNumer sndDenom
%   powNumer powDenom expDPowHi ignoredIx normPowNumer candIx candT

% Find click candidates (candIx) in powNumer using expdecay and firstPeak.
powNumer = sndNumer .^ 2 / bwNumer;		% power per kHz of bandwidth
powDenom = sndDenom .^ 2 / bwDenom;		% power per kHz of bandwidth
[expDPowHi,~,~,ignoredIx] = expdecay(powNumer, sRate, p.decayTime, [], ...
  p.decayTime, false, false, p.ignoreThresh / bwNumer, p.ignoreLimT);
normPowNumer = powNumer ./ expDPowHi.'';  % numer power normalized by long-term avg
nbdSam = round(p.peakNbdT * sRate);
refractorySam = round(p.refractoryT * sRate);
powerThreshPerKHz = p.powerThresh / bwNumer;
candIx = firstPeak(normPowNumer, powerThreshPerKHz, refractorySam);
candIx = peakNear(normPowNumer, candIx, nbdSam);
candT = candIx / sRate;

% Test the candidates to see whether they have the minimum ratio needed.
avgSam = round(p.avgT * sRate);
avgPowNumer = average(powNumer, avgSam);
avgPowDenom = average(powDenom, avgSam);
avgDelayT = (avgSam - 1) / sRate;
ratio = avgPowNumer ./ avgPowDenom;
% Variables affected by avgDelayT (these also have filtDelayT):
%    avgPowNumer avgPowDenom ratio goodRatioT

% Find the peaks where ratio > p.ratioThresh. peakRatioIx has indices into ratio
% that are above threshold, goodRatioIxP has logical indices into peakRatioIx,
% and goodRatioIx has indices into ratio.
peakRatioIx = peakNear(ratio, candIx + filtDelaySam - avgSam, nbdSam);
goodRatioIxP = ratio(peakRatioIx) > p.ratioThresh;
goodRatioIx = peakRatioIx(goodRatioIxP);
goodRatioT = goodRatioIx / sRate;

clickT = goodRatioT + filtDelayT + avgDelayT;

*/


/* Time-average the signals num (numerator) and den (denominator), which each
 * have length nNum, over avgSam samples, then calculate their ratio. The result
 * is stored in array 'ratio' (which the caller should allocate before calling
 * this), whose length is returned in *pNRatio; this is shorter than nNum by
 * avgSam-1 samples.
 *
 * The running average is calculated by (1) summing the first avgSam-1 samples,
 * (2) on each successive sample, adding the next sample (so the sum has avgSam
 * of them), calculating and storing the average in ratio[j], and then
 * subtracting the sample that happened avgSam samples ago from the sum, so it
 * once again has the sum of the most recent avgSam-1 samples. This happens in
 * parallel for both num and den, with the running sums being numSum and denSum
 * respectively.
 *
 * Since we're summing floats, this method can accumulate errors. So every
 * nPerLoop samples, the process is re-started to get rid of any accumulated
 * error.
 *
 * Also, if numAvg[] and denAvg[] are non-NULL, store the num and den averages
 * in them.
 */
static void calcAverageRatio(float *num, float *den, int32 nNum,
			     int32 avgSam, float *ratio, int32 *pNRatio,
			     float *numAvg, float *denAvg)
{
    const int32 nPerLoop = MAX(1000, avgSam*2);

    //iBig is the "big loop i" - it steps forward by nPerLoop each time
    for (int32 iBig = 0; iBig < nNum; iBig += nPerLoop) {
	float numSum = 0, denSum = 0;
	int32 i = iBig;		//i is used after loop is done
	for (int32 iEnd = MIN(iBig + avgSam - 1, nNum) ; i < iEnd; i++) {
	    numSum += num[i];
	    denSum += den[i];
	}
	//At this point i=iBig + avgSam - 1; this i is used in this loop.
	for (int32 iEnd = MIN(i+nPerLoop, nNum), j = iBig; i < iEnd; i++, j++) {
	    /* Currently numSum & denSum have sum of the last avgSam-1 sams */
	    numSum += num[i];
	    denSum += den[i];
	    /* The averages would be numSum and denSum each divided by avgSam,
	     * but avgSam disappears because we're taking the ratio of them. */
	    ratio[j] = numSum / denSum;
	    if (numAvg != NULL) {	//debugging; numAvg is NULL on RPi
		numAvg[j] = numSum / (float)avgSam;
		denAvg[j] = denSum / (float)avgSam;
	    }
	    //Subtract the 'trailing' sample from the running sum.
	    numSum -= num[j];
	    denSum -= den[j];
	}
    }
    *pNRatio = nNum - (avgSam - 1);
}


/* Find clicks - peaks in x (which is normPowNumer) and ratio where (a) the
 * start of the peak in x is above thresh, (b) there aren't past values in x
 * within refractorySam samples that are above peak, (c) the location of the
 * peak is adjusted to the highest value within nbdSam samples, and (d) the
 * ratio at the peak is above another threshold. The peaks found are stored in
 * fc along with their spectra. seg is the original signal, which is used only
 * for calculating spectra.
 */
void findClicks(float *x, int32 nX, float segT0, float *ratio, int32 nRatio,
		float sRate, ERMAPARAMS *ep, int32 delaySam, float bwNumer,
		FILECLICKS *fc, float *seg, size_t nSeg, float segSRate)
{
#ifdef DEBUG_SAVE_NBDS
    printf("findClicks: printing click nbds to file\n");
    FILE *fp = fopen("temp-ratioNbds.csv", "w");
    fprintf(fp, "npnPeak,nbd0,nbd1,ratioPeak,isClick\n");
#endif

    int32 nbdSam = round(ep->peakNbdT * sRate);
    int32 refractorySam = round(ep->refractoryT * sRate);
    float powerThreshPerKHz = ep->powerThresh / bwNumer;
    size_t nSpecSams = (size_t)round(ep->specLenS * sRate / SPECLEN) * SPECLEN;

    //Set up Hanning window if needed.
    static float win[NFFT];
    static int winInitted = 0;
    if (!winInitted) {
	fftMakeWindow(win, WIN_HANNING, NFFT, 0);
	winInitted = 1;
    }
    
    int32 nLow = 0;
    int32 i = 0;
    while (i < nRatio) {	//i can get changed inside the loop
	if (x[i] <= powerThreshPerKHz) {
	    nLow += 1;
	} else {
	    /* Found a value above thresh. Is it a new peak? */
	    if (nLow >= refractorySam) {
		/* Found the start of a peak. Find its high point. */
		int32 ixN = peakNear(x, nX, i, nbdSam);
		/* Find corresponding high point in ratio[]. */
		int32 ixR = peakNear(ratio, nRatio, i - delaySam, nbdSam);
#ifdef DEBUG_SAVE_NBDS
		int32 segIx0 = segT0 * sRate;
		fprintf(fp, "%d,%d,%d,%d,%d\n", ixN + segIx0,
			i - delaySam - nbdSam + segIx0,
			i - delaySam + nbdSam + 1 + segIx0,
			ixR + segIx0, ratio[ixR] > ep->ratioThresh);
#endif
		if (ratio[ixR] > ep->ratioThresh) {
		    /* Found a click. Add it to fc. */
		    BUFGROW(fc->timeS, fc->n + 1, ERMA_NO_MEMORY_PEAK);
/*		    BUFGROW(fc->spec, fc->n + 1, ERMA_NO_MEMORY_PEAK);*/
		    fc->timeS[fc->n] = (ixR + delaySam) / sRate + segT0;
/*		    addSpec(fc->spec[fc->n], (float)ixN/sRate, seg, nSeg,*/
/*			    segSRate, nSpecSams, win);*/
		    fc->n += 1;
		    /* Advance i past this peak. Gets incremented below too. */
		    i = MAX(i, ixR - delaySam);
		}
	    }
	    nLow = 0;
	}
	i += 1;
    }
#ifdef DEBUG_SAVE_NBDS
    fclose(fp);
#endif
}


/* Put a spectrum taken from the original signal seg in cs. The spectrum is
 * calculated for a series of sound segments of length NFFT; their power is
 * summed and then divided by the number of spectra to get the average power
 * spectrum for this click.
 */
void addSpec(CLICKSPEC cs, float t, float *seg, size_t nSeg,
	     float sRate, size_t nSpecSams, float *win)
{
    size_t ix = (size_t)round(t * sRate);
    size_t ix0 = ix - nSpecSams/2;
    size_t ix1 = ix + (nSpecSams + 1)/2;
    float xRe[NFFT], xIm[NFFT];
    float sumSpec[SPECLEN];

    //Take care of endpoints.
    if (ix0 < 0) {
	ix1 -= ix0;
	ix0 = 0;
    }
    if (ix1 > nSeg) {
	ix0 -= ix1 - nSeg;
	ix1 = nSeg;
    }
    if (ix0 < 0)
	ix0 = 0;
    
    //Prepare sumSpec, which has the summed power spectra.
    int32 nSumSpec = 0;	//number of spectra that get summed
    for (int32 j = 0; j < SPECLEN; j++)
	sumSpec[j] = 0.0;

    for (size_t i = ix0; i + NFFT < ix1; i += SPECLEN) {
	//Set up real and imaginary arrays. The latter starts off 0.
	memcpy(xRe, &seg[i], NFFT * sizeof(xRe[0]));
	for (int32 j = 0; j < NFFT; j++)
	    xIm[j] = 0.0;
	//Window the real part.
	for (int32 j = 0; j < NFFT; j++)
	    xRe[j] *= win[j];

	//Calculate spectrum. It ends up in upper half, reversed.
	fft(xRe, xIm, FFTM);

	//Add summed power to sumSpec.
	nSumSpec++;
	for (int32 jSrc = SPECLEN, jDst = 0; jSrc < NFFT; jSrc++, jDst++)
	    sumSpec[jDst] += xRe[jSrc]*xRe[jSrc] + xIm[jSrc]*xIm[jSrc];
    }
    //Put average power in output (cs). Also reverse sumSpec to get low
    //frequencies first.
    for (int32 j = 0, k = SPECLEN-1; j < SPECLEN; j++, k--)
	cs[j] = sumSpec[k] / nSumSpec;

#ifdef DEBUG_SAVE_SPECTRA
    writeFloatArray(cs, SPECLEN, "temp-clickSpectrum.flt");
#endif
}


/* For debugging.
 */
void writeFILECLICKS(FILECLICKS *fc, char *filename)
{
    FILE *fp = fopen(filename, "w");
    for (int32 i = 0; i < fc->n; i++)
	fprintf(fp, "%.3f\n", fc->timeS[i]);
    fclose(fp);
}


/* For debugging.
 */
void writeALLCLICKS(ALLCLICKS *ac, char *filename)
{
    FILE *fp = fopen(filename, "w");
    for (int32 i = 0; i < ac->n; i++) {
	double timeE_dbl = ac->timeD[i] * 24*60*60;
	time_t timeE_int = (time_t)floor(timeE_dbl);
	char *asc = asctime(gmtime(&timeE_int));
	float fracSec = remainder(timeE_dbl, 1.0L);
	fprintf(fp, "%s%4.3f\n", asc, fracSec);
    }
    fclose(fp);
}


/* Append the clicks in the FILECLICKS fc, which has seconds in a file, to the
 * ALLCLICKS ac, which has days since the Epoch.
 */
void appendClicks(ALLCLICKS *ac, FILECLICKS *fc, double fileTimeE)
{
    int32 i, j;
    double secPerDay = 24*60*60;
    
    BUFGROW(ac->timeD, ac->n + fc->n, ERMA_NO_MEMORY_APPENDCLICKS);
    for (i = 0, j = ac->n; i < fc->n; i++, j++)
	ac->timeD[j] = ((double)fc->timeS[i] + fileTimeE) / secPerDay;
    ac->n += fc->n;
}


/* Find the highest value in x within nbd samples around x[ix] and return its
 * index.
 */
int32 peakNear(float *x, int32 nX, int32 ix, int32 nbd)
{
    /* MAX and MIN here are for handling the ends of x */
    int32 i0 = MAX(0, ix - nbd);
    int32 i1 = MIN(nX, ix + nbd + 1);	//+1 matches MATLAB 1-based indexing
    
    int32 maxPos = maxIx(&x[i0], i1 - i0, NULL) + i0;
    return maxPos;
}


/* Save detections to outPath. A base time of the start of the file is written
 * first as seconds since The Epoch, then all detections are written as seconds
 * since that base time (since start of file).
 */
void saveNewClicks(ALLCLICKS *allC, int32 startN, double fileTimeE,
		   char *outPath, char *inPath)
{
    const double secPerDay = 24*60*60;
    FILE *fp = fopen(outPath, "a");

    if (fp != NULL) {
	if (allC->n > startN) {
	    double baseTime_D = fileTimeE / secPerDay;
/*	    double baseTime_D = allC->timeD[0];	//times are measured from this*/
	    fprintf(fp, "$clickDet,%s,%.3lf",
		    pathFile(inPath), baseTime_D * secPerDay);
	    for (int32 i = startN; i < allC->n; i++) 
		fprintf(fp, ",%.3lf", (allC->timeD[i]- baseTime_D) * secPerDay);
	    fprintf(fp, "\n");
	}
	fclose(fp);
    }
}
