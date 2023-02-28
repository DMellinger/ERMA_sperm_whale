
#include "erma.h"


/* Defined below */
static void calcAverageRatio(float *num, float *den, int32 nNum,
			     int32 avgSam, float *ratio, int32 *pNRatio);
int32 peakNear(float *x, int32 nX, int32 ix, int32 nbdSam);
void findClicks(float *x, int32 nX, float *ratio, int32 nRatio, float sRate,
		ERMAPARAMS *ep, int32 avgSam, int32 filtDelaySam, float bwNumer,
		FILECLICKS *fc);
void writeFileClicks(FILECLICKS *fc, char *filename);
void writeAllClicks(ALLCLICKS *ac, char *filename);



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
	ermaNew(&snd[i0], i1 - i0, wi->sRate, ep, fc);
    }
}


/* Run the ERMA process on a segment of snd for nSam samples. Results are left
 * in fc.
 */
void ermaNew(float *seg, int32 nSam, float sRate, ERMAPARAMS *ep,
	     FILECLICKS *fc)
{
    static float *x = NULL;	/* the decimated signal */
    static size_t xSize = 0;
    int32 nX, nRatio;
    float bwNumer, bwDenom, newSRate;

    /* Decimate the signal. */
    BUFGROW(x, (nSam / ep->decim) + 1, ERMA_NO_MEMORY_DECIMBUF);
    ermaDownsample(seg, nSam, ep->decim, x, &nX, sRate, &newSRate);
    sRate = newSRate;

    /* Do the ERMA filtering. */
    static float *numer = NULL, *denom = NULL, *ratio = NULL;
    static size_t numerSize = 0, denomSize = 0, ratioSize = 0;
    BUFGROW(numer, nX, ERMA_NO_MEMORY_NUMER_DENOM);
    BUFGROW(denom, nX, ERMA_NO_MEMORY_NUMER_DENOM);
    BUFGROW(ratio, nX, ERMA_NO_MEMORY_NUMER_DENOM);
    ermaNumerDenomFilt(x, nX, numer, denom); //numer, denom same length as x

/*    printf("ermaNew: writing temp signal files\n");*/
/*    writeSignal(x, nX, "tempY-downSampled.b600");*/
/*    writeSignal(numer, nX, "temp-numer.b600");*/
/*    writeSignal(denom, nX, "temp-denom.b600");*/

    ermaFiltGetBandwidths(&bwNumer, &bwDenom);
    bwNumer /= 1000.0;		/* make Hz into kHz */
    bwDenom /= 1000.0;		/* make Hz into kHz */
    
    int32 filtDelaySam = (numerFilter.n + 1) / 2;
    float filtDelayT = (float)filtDelaySam / sRate;

    /* Convert filtered signals to power per kHz of bandwidth. Results are
     * called powNumer and powDenom in MATLAB. */
    for (int32 i = 0; i < nX; i++) {
	numer[i] = numer[i] * numer[i] / bwNumer;
	denom[i] = denom[i] * denom[i] / bwDenom;
    }
    /* Compute power ratio while power is in numer & denom. */
    float avgSam = round(ep->avgT * sRate);	/* # samples to average over */
    calcAverageRatio(numer, denom, nX, avgSam, ratio, &nRatio);


/*    printf("ermaNew: printing temp ratio file\n");*/
/*    writeSignal(x, nX, "tempY-downSampled.b600");*/
/*    printSignal(ratio, nX, "temp-ratio.csv");*/



    /* Exponentially decay numer, dividing by running mean and leaving result in
     * numer. Result (i.e., numer) is called normPowNumer in MATLAB. */
    expDecay(numer, nX, sRate, NULL, ep->decayTime, ep->decayTime,
	     ep->ignoreThresh / bwNumer, ep->ignoreLimT, 1);

    /* Find clicks - peaks in x that also satisfy the ERMA ratio test. Click
     * detection times are put into fc. */
    findClicks(numer, nX, ratio, nRatio, sRate, ep, avgSam, filtDelaySam,
	       bwNumer, fc);
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


/* Time-average the signals num and den (which have length nNum) over avgSam
 * samples, then calculate their ratio. Also divide the ratio by an additional
 * factor (which is bandwidth ratio) and store the result in 'ratio'. ratio ends
 * up shorter than nNum by avgSam-1 samples.
 */
static void calcAverageRatio(float *num, float *den, int32 nNum,
			     int32 avgSam, float *ratio, int32 *pNRatio)
{
    float numSum = 0, denSum = 0;
    for (int32 i = 0; i < avgSam-1; i++) {
	numSum += num[i];
	denSum += den[i];
    }
    for (int32 i = avgSam, j = 0; i < nNum; i++, j++) {
	/* Currently numSum and denSum have sum of the last avgSam-1 samples */
	numSum += num[i];
	denSum += den[i];
	/* The average would be numSum (or denSum) divided by avgSam; but avgSam
	 * disappears here because we're taking the ratio of the two. */
	ratio[j] = numSum / denSum;
	numSum -= num[j];
	denSum -= den[j];
    }
    *pNRatio = nNum - (avgSam - 1);
}


/* Find clicks - peaks in x and ratio where (a) the start of the peak in x is
 * above thresh, (b) there aren't past values in x within refractorySam samples
 * that are above peak, (c) the location of the peak is adjusted to the highest
 * value within nbdSam samples, and (d) the ratio at the peak is above another
 * threshold. The peaks found are stored in fc.
 */
void findClicks(float *x, int32 nX, float *ratio, int32 nRatio, float sRate,
		ERMAPARAMS *ep, int32 avgSam, int32 filtDelaySam, float bwNumer,
		FILECLICKS *fc)
{
    int32 nbdSam = round(ep->peakNbdT * sRate);
    int32 refractorySam = round(ep->refractoryT * sRate);
    float powerThreshPerKHz = ep->powerThresh / bwNumer;
    int32 nLow = 0;
    int32 i = 0;
    while (i < nRatio) {	/* i can get changed inside the loop */
	if (x[i] <= powerThreshPerKHz) {
	    nLow += 1;
	} else {
	    /* Found a value above thresh. Is it a new peak? */
	    if (nLow >= refractorySam) {
		/* Found the start of a peak. Find its high point. */
		int32 ixN = peakNear(x, nX, i, nbdSam);
		/* Find corresponding high point in ratio[]. */
		int32 ixR = peakNear(ratio, nRatio, i + filtDelaySam - avgSam,
				     nbdSam);
		if (ratio[ixR] > ep->ratioThresh) {
		    BUFGROW(fc->timeS, fc->n + 1, ERMA_NO_MEMORY_PEAK);
		    fc->timeS[fc->n] = ixR / sRate;
		    fc->n += 1;
		    /* Advance i past this peak. Gets incremented below too. */
		    i = MAX(i, ixR - filtDelaySam + avgSam);
		}
	    }
	    nLow = 0;
	}
	i += 1;
    }

/*    printf("findClicks: saving click times in temp file\n");*/
/*    writeFileClicks(fc, "temp-clicktimes.csv");*/
}


/* For debugging.
 */
void writeFileClicks(FILECLICKS *fc, char *filename)
{
    FILE *fp = fopen(filename, "w");
    for (int32 i = 0; i < fc->n; i++)
	fprintf(fp, "%.3f\n", fc->timeS[i]);
    fclose(fp);
}


/* For debugging.
 */
void writeAllClicks(ALLCLICKS *ac, char *filename)
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
    
    int32 maxPos = maxIx(&x[i0], i1 - i0, NULL);
    return maxPos;
}


/* Save detections to outPath. The first detection is stored as seconds since
 * the Epoch, then successive ones are seconds since the previous detection.
 */
void saveAllClicks(ALLCLICKS *allC, char *outPath, char *inPath)
{
    const double secPerDay = 24*60*60;
    FILE *fp = fopen(outPath, "a");

    if (fp != NULL) {
	if (allC->n > 0) {
	    double lastT = (allC->timeD[0] - lastT) * 24*60*60;
	    fprintf(fp, "$clickDet,%s,%lf.3", pathFile(inPath), (double)lastT);
	    for (int32 i = 0; i < allC->n; i++) {
		fprintf(fp, ",%.3lf.3", (allC->timeD[i] - lastT) * secPerDay);
		lastT = allC->timeD[i];
	    }
	    fprintf(fp, "\n");
	}
	fclose(fp);
    }
}
