
#include "erma.h"


/* This module does the filtering for the ERMA process - first the filtering for
 * downsampling by a factor of 3 (from 180 kHz to 60 kHz), then the numerator
 * and denominator filters for the ERMA calculation.
 */

/*************************** filter definitions *****************************/

/* These define the default filter coefficients. They can be re-used by multiple
 * instances of the filters running in parallel since their values, once set, do
 * not change. You need to call initIirFilter on each one to set the B1 and A1
 * values. These coefficients can also be overridden in rpi.cnf.
 */

/* These filters are for downsampling from 180 kHz to 60 kHz, leaving data below
 * 23.5 kHz as untouched as possible. They were made in MATLAB (testFilters.m)
 * with the calls to ellip() listed just before each set of filter coefficients.
 * 
 * On the second and third ones, the call ellip has order 4, but because it's a
 * bandpass filter the resulting filter has order 8 (i.e., 9 coefficients). The
 * 2-kHz lower cutoff is to remove loud noise near DC.
 */

/* From [b,a] = ellip(5, 2, 60, 25/90). I think the order for a lowpass filter
 * has to be odd (i.e., an even number of filter coefficients), as when I tried
 * ellip(4, ...) it didn't work. */
/*static float dsfB[] =						    *///B
/*    { 0.006646197059847, 0.004917300441647, 0.010588989094936,*/
/*      0.010588989094936, 0.004917300441647, 0.006646197059847 };*/
/*static float dsfA[] =						    *///A
/*      { 1.000000000000000, -3.471368956527336, 5.574936187728706,*/
/*	-4.980558386032349, 2.463946468964540, -0.542650340940701 };*/

/* From [b,a] = ellip(4, 2, 60, [2 24]/90). This is a bandpass filter, so the
 * number of coeffs is 2*order+1. */
/*static float dsfB[] =*/
/*    { 0.007746422853218, -0.020325895603531, 0.019457000749097,*/
/*      -0.018777975278226, 0.023800903698914, -0.018777975278226,*/
/*      0.019457000749097, -0.020325895603532, 0.007746422853218 };*/
/*static float dsfA[] =*/
/*    { 1.000000000000000, -6.700362216212774, 20.190470411506297,*/
/*      -35.855408280734331, 41.124782451865656, -31.217964151667751,*/
/*      15.312823366384027, -4.434007111143975, 0.579674670031067 };*/

/* From [b,a] = ellip(4, 2, 60, [3 24]/90). This is a bandpass filter, so the
 * number of coeffs is 2*order+1.  */
static float dsfB[] =
    { 0.006963836673885, -0.019053637420856, 0.020023609965423,
      -0.019909423243391, 0.023951274248473, -0.019909423243392,
      0.020023609965423, -0.019053637420856, 0.006963836673885 };

static float dsfA[] =
    { 1.000000000000000, -6.663681662931410, 20.033505429094703,
      -35.584638535052193, 40.908195659872050, -31.183646057073140,
      15.390352164262620, -4.494155069695583, 0.594114270117529 };


IIRFILTER downsampleFilter =	//default filter; rpi.cnf may override it
    { {0, 25e3},		//passband, Hz
      NUM_OF(dsfB),		//n: length of B and A
      dsfB, dsfA,		//B, A
      NULL, NULL		//B1, A1
    };


/* This filter is for the numerator of the ERMA calculation for a 60 kHz sample
 * rate. (The denominator one is below.) It was made in MATLAB (testFilters.m)
 * with
 *
 *		[b,a] = ellip(3, 2, 60, [4 8] / 30)
 */
static float numerB_60kHz[] =
    { 0.004949674561963, -0.007591117532359, 0.000607613894629,
      0.000000000000000, -0.000607613894629, 0.007591117532359,
      -0.004949674561963 };
static float numerA_60kHz[] =
    { 1.000000000000000, -4.607661069228133, 9.732733677096491,
      -11.856007135368305, 8.779320675676932, -3.749344788838549,
      0.735251386501633 };

/* Same, but with 50 kHz sRate:     [b,a] = ellip(3, 2, 60, [4 8] / 25)
 */
static float numerB_50kHz[] =
    { 0.007119415752776, -0.007972105997869, -0.004765768694883,
      0.000000000000000, 0.004765768694883, 0.007972105997869,
      -0.007119415752776 };
static float numerA_50kHz[] =
    { 1.000000000000000, -4.108868851635148, 8.183671546337083,
      -9.695682976738993, 7.232676605709873, -3.207474776547401,
      0.691636087128368 };

IIRFILTER numerFilter_50kHz =
    { {4e3, 8e3},		//passband, Hz
      NUM_OF(numerB_50kHz),	//n: length of B and A
      numerB_50kHz, numerA_50kHz,//B, A
      NULL, NULL		//B1, A1
    };
IIRFILTER numerFilter_60kHz =
    { {4e3, 8e3},		//passband, Hz
      NUM_OF(numerB_60kHz),	//n: length of B and A
      numerB_60kHz, numerA_60kHz,//B, A
      NULL, NULL		//B1, A1
    };
/* This is the filter that is actually used for the numerator. It receives a
 * copy of either numerFilter_60kHz, numerFilter_50kHz, or one the user
 * specified in rpi.cnf.
 */
IIRFILTER numerFilter =
    { { 0.0, 0.0 },		//passband, Hz
      0,			//n: length of B and A
      NULL, NULL,		//B, A
      NULL, NULL		//B1, A1
    };


/* This filter is for the denominator - the "guard band" - of the ERMA
 * calculation for a 60 kHz sample rate. (The numerator one is above.) It was
 * made in MATLAB (testFilters.m) with
 *
 *		[b,a] = ellip(3, 2, 60, [22 23.5] / 30)
 */
static float denomB_60kHz[] =
    { 0.001115247785115, 0.002801466558323, 0.002542844341920,
      -0.000000000000000, -0.002542844341920, -0.002801466558323,
      -0.001115247785115 };
static float denomA_60kHz[] =
    { 1.000000000000000, 4.268476814187771, 8.950132499671438,
      11.080651372260499, 8.611736489648441, 3.951719189802946,
      0.890860742005225 };
/* Same, but with 50 kHz sRate:    [b,a] = ellip(3, 2, 60, [22 23.5] / 25)
 */
static float denomB_50kHz[] =
    { 0.001401496233272, 0.004400335669003, 0.004601907723615,
      0.000000000000000, -0.004601907723615, -0.004400335669003,
      -0.001401496233272 };
static float denomA_50kHz[] =
    { 1.000000000000000, 5.628779051638011, 13.422689768912146,
      17.348255305470765, 12.815388984643835, 5.131239693594241,
      0.870524913784521 };

IIRFILTER denomFilter_60kHz =
    { {22e3, 23.5e3},		//passband, Hz
      NUM_OF(denomB_60kHz),	//n: length of B and A
      denomB_60kHz, denomA_60kHz,//B, A
      NULL, NULL		//B1, A1
    };
IIRFILTER denomFilter_50kHz =
    { {22e3, 23.5e3},		//passband, Hz
      NUM_OF(denomB_50kHz),	//n: length of B and A
      denomB_50kHz, denomA_50kHz,//B, A
      NULL, NULL		//B1, A1
    };

/* This is the filter that is actually used for the denominator. It receives a
 * copy of either denomFilter_60kHz, denomFilter_50kHz, or one the user
 * specified in rpi.cnf.
 */
IIRFILTER denomFilter =
    { { 0.0, 0.0 },		//passband, Hz
      0,			//n: length of B and A
      NULL, NULL,		//B, A
      NULL, NULL		//B1, A1
    };
			 
/************************ end of filter definitions **************************/


/* Define the warmup vectors. These CANNOT be reused if there are multiple
 * instances of a given filter running - each filter run needs its own.
 */
float *downsampleWarmup = NULL;
float *numerWarmup_60kHz = NULL, *denomWarmup_60kHz = NULL, *numerWarmup = NULL;
float *numerWarmup_50kHz = NULL, *denomWarmup_50kHz = NULL, *denomWarmup = NULL;

/* Prepare filters for use: install params from the ERMAPARAMS if they were
 * specified in rpi.cnf (if they weren't, ep->dsfA/numerA/denomA will be NULL
 * and the corresponding n is 0). Then set the B1 and A1 vectors in each filter
 * via initIirFilter.
 */
void ermaFiltPrep(ERMAPARAMS *ep)
{
    /* Prepare the downsampling filter */
    if (ep->dsfN > 0 && ep->dsfA != NULL && ep->dsfB != NULL) {
	downsampleFilter.B = ep->dsfB;
	downsampleFilter.A = ep->dsfA;
	downsampleFilter.n = ep->dsfN;
	/* B1 and A1 get made from B and A, so need to remake them too */
	downsampleFilter.B1 = calloc(ep->dsfN, sizeof(downsampleFilter.B1[0]));
	downsampleFilter.A1 = calloc(ep->dsfN, sizeof(downsampleFilter.A1[0]));
    }

    /* Prepare the ERMA numerator filter */
    if (ep->numerN > 0 && ep->numerA != NULL && ep->numerB != NULL) {
	numerFilter.B = ep->numerB;
	numerFilter.A = ep->numerA;
	numerFilter.n = ep->numerN;
	/* B1 and A1 get made from B and A, so need to remake them too */
	numerFilter.B1 = calloc(ep->dsfN, sizeof(numerFilter.B1[0]));
	numerFilter.A1 = calloc(ep->dsfN, sizeof(numerFilter.A1[0]));
    }

    /* Prepare the ERMA denominator filter */
    if (ep->denomN > 0 && ep->denomA != NULL && ep->denomB != NULL) {
	denomFilter.B = ep->denomB;
	denomFilter.A = ep->denomA;
	denomFilter.n = ep->denomN;
	/* B1 and A1 get made from B and A, so need to remake them too */
	denomFilter.B1 = calloc(ep->dsfN, sizeof(denomFilter.B1[0]));
	denomFilter.A1 = calloc(ep->dsfN, sizeof(denomFilter.A1[0]));
    }

    /* Construct the B1 and A1 coefficients in each filter. */
    if (initIirFilter(&downsampleFilter,  &downsampleWarmup) ||
	initIirFilter(&numerFilter_60kHz, &numerWarmup_60kHz)      ||
	initIirFilter(&numerFilter_50kHz, &numerWarmup_50kHz)      ||
	initIirFilter(&denomFilter_60kHz, &denomWarmup_60kHz)      ||
	initIirFilter(&denomFilter_50kHz, &denomWarmup_50kHz)        )
	exit(ERMA_NO_MEMORY_FILTER_WARMUP);
}


/* Downsample a signal by a factor of decim, first lowpass-filtering it so it
 * doesn't alias. Y must be pre-allocated as long as nX. Returns the length of
 * the new signal (= floor(nX/decim)) in *pNY.
 */
void ermaDownsample(float *X,  int32_t nX,		//in
		    int32 decim,				//in
		    float *Y, int32_t *pNY,		//out
		    float inSRate, float *pOutSRate)	//in, out
{
    if (inSRate > 100000) {
	//Assume signal is ~180 kHz. Lowpass-filter the signal into Y for
	//anti-aliasing, then decimate it.
	iirFilter(&downsampleFilter, X, nX, downsampleWarmup, Y);
	
	//Decimate it. The decimation factor is 3, which is because the signal
	//is assumed to be 180 kHz and we want 60 kHz.
	int32 j = 0;			//is used after loop
	for (int32 i = 0; i < nX; i += decim, j++)
	    Y[j] = Y[i];
	*pNY = j;
	*pOutSRate = inSRate / (float)decim;
    } else {
	//Assume signal is somewhere near 50 kHz, doesn't need downsampling.
	//Just make a copy in Y.
	for (int32 i = 0; i < nX; i++)
	    Y[i] = X[i];
	*pNY = nX;
	*pOutSRate = inSRate;
    }
    
    //If the 60 vs. 50 kHz filter hasn't been picked yet, do that. This could
    //get done below in ermaNumerDenomFilt, but we have the sample rate here and
    //don't there. First check whether B, A, and n are set, which might have
    //happened on a previous call here or else were set by user in rpi.cnf.
    if (numerFilter.B == NULL || numerFilter.A == NULL || numerFilter.n < 1) {
	if (inSRate < 55000) {
	    //sRate is closer to 50 kHz than 60 kHz.
	    numerFilter = numerFilter_50kHz;
	    numerWarmup = numerWarmup_50kHz;
	} else {
	    //sRate is closer to 60 kHz than 50 kHz.
	    numerFilter = numerFilter_60kHz;
	    numerWarmup = numerWarmup_60kHz;
	}
    }
    if (denomFilter.B == NULL || denomFilter.A == NULL || denomFilter.n < 1) {
	if (inSRate < 55000) {
	    //sRate is closer to 50 kHz than 60 kHz.
	    denomFilter = denomFilter_50kHz;
	    denomWarmup = denomWarmup_50kHz;
	} else {
	    //sRate is closer to 60 kHz than 50 kHz.
	    denomFilter = denomFilter_60kHz;
	    denomWarmup = denomWarmup_60kHz;
	}
    }
}


/* Run the numerator and denominator filters for the ERMA calculation on the
 * input signal X. Results are put in numer and denom, which should be at least
 * as long as X.
 */
void ermaNumerDenomFilt(float *X, int32 nX,		//in
			float *numer, float *denom)	//out
{
    iirFilter(&numerFilter, X, nX, numerWarmup, numer);
    iirFilter(&denomFilter, X, nX, denomWarmup, denom);
}


/* Return the bandwidths of the filters.
 */
void ermaFiltGetBandwidths(float *pNumerBW, float *pDenomBW)
{
    *pNumerBW = numerFilter.passband[1] - numerFilter.passband[0];
    *pDenomBW = denomFilter.passband[1] - denomFilter.passband[0];
}
