
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
 * with
 * 
 *            [b,a] = ellip(5, 2, 60, 25/90)
 *	      [b,a] = ellip(4, 2, 60, [2 24]/90)
 *
 * On the first one, I think the filter order has to be odd (i.e., an even
 * number of filter coefficients), as when I tried ellip(4, ...) it didn't work.
 *
 * On the second one, the call ellip has order 4, but because it's a passband
 * filter the resulting filter has order 8 (i.e., 9 coefficients). The 2-kHz
 * lower cutoff is to remove loud noise near DC.
 */
/*static float dsfB[] =						    *//* B */
/*    { 0.006646197059847, 0.004917300441647, 0.010588989094936,*/
/*      0.010588989094936, 0.004917300441647, 0.006646197059847 };*/
/*static float dsfA[] =						    *//* A */
/*      { 1.000000000000000, -3.471368956527336, 5.574936187728706,*/
/*	-4.980558386032349, 2.463946468964540, -0.542650340940701 };*/

static float dsfB[] =
    { 0.007746422853218, -0.020325895603531, 0.019457000749097,
      -0.018777975278226, 0.023800903698914, -0.018777975278226,
      0.019457000749097, -0.020325895603532, 0.007746422853218 };
static float dsfA[] =
    { 1.000000000000000, -6.700362216212774, 20.190470411506297,
      -35.855408280734331, 41.124782451865656, -31.217964151667751,
      15.312823366384027, -4.434007111143975, 0.579674670031067 };


IIRFILTER downsampleFilter =	/* default filter; rpi.cnf may override it */
    { {0, 25e3},		/* passband, Hz */
      NUM_OF(dsfB),		/* n: length of B and A */
      dsfB, dsfA,		/* B, A */
      NULL, NULL		/* B1, A1 */
    };


/* This filter is for the numerator of the ERMA calculation for a 60 kHz sample
 * rate. (The denominator one is below.) It was made in MATLAB (testFilters.m)
 * with
 *
 *		[b,a] = ellip(3, 2, 60, [4 8] / 30)
 */
static float numerB[] =
    { 0.004949674561963, -0.007591117532359, 0.000607613894629,
      0.000000000000000, -0.000607613894629, 0.007591117532359,
      -0.004949674561963 };
static float numerA[] =
    { 1.000000000000000, -4.607661069228133, 9.732733677096491,
      -11.856007135368305, 8.779320675676932, -3.749344788838549,
      0.735251386501633 };
IIRFILTER numerFilter =		/* default filter; rpi.cnf may override it */
    { {4e3, 8e3},		/* passband, Hz */
      NUM_OF(numerB),		/* n: length of B and A */
      numerB, numerA,		/* B, A */
      NULL, NULL		/* B1, A1 */
    };


/* This filter is for the denominator - the "guard band" - of the ERMA
 * calculation for a 60 kHz sample rate. (The numerator one is above.) It was
 * made in MATLAB (testFilters.m) with
 *
 *		[b,a] = ellip(3, 2, 60, [22 23.5] / 30)
 */
static float denomB[] =
    { 0.001115247785115, 0.002801466558323, 0.002542844341920,
      -0.000000000000000, -0.002542844341920, -0.002801466558323,
      -0.001115247785115 };
static float denomA[] =
    { 1.000000000000000, 4.268476814187771, 8.950132499671438,
      11.080651372260499, 8.611736489648441, 3.951719189802946,
      0.890860742005225 };
IIRFILTER denomFilter =		/* default filter; rpi.cnf may override it */
    { {22e3, 23.5e3},		/* passband, Hz */
      NUM_OF(denomB),		/* n: length of B and A */
      denomB, denomA,		/* B, A */
      NULL, NULL		/* B1, A1 */
    };
/************************ end of filter definitions **************************/


/* Define the warmup vectors. These CANNOT be reused if there are multiple
 * instances of a given filter running - each filter run needs its own.
 */
float *downsampleWarmup = NULL, *numerWarmup = NULL, *denomWarmup = NULL;

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
    if (initIirFilter(&downsampleFilter, &downsampleWarmup) ||
	initIirFilter(&numerFilter,      &numerWarmup)      ||
	initIirFilter(&denomFilter,	 &denomWarmup)        )
	exit(ERMA_NO_MEMORY_FILTER_WARMUP);
}


/* Downsample a signal by a factor of decim, first lowpass-filtering it so it
 * doesn't alias. Y must be pre-allocated as long as nX. Returns the length of
 * the new signal (= floor(nX/decim)) in *pNY.
 */
void ermaDownsample(float *X,  int32_t nX,		/* in */
		    long decim,				/* in */
		    float *Y, int32_t *pNY,		/* out */
		    float inSRate, float *outSRate)	/* in, out */
{
    /* Lowpass-filter the signal for anti-aliasing */
    iirFilter(&downsampleFilter, X, nX, downsampleWarmup, Y);
    
    /* Decimate it */
    int32 j = 0;			//is used after loop
    for (int32 i = 0; i < nX; i += decim, j++)
	Y[j] = Y[i];
    *pNY = j;
    *outSRate = inSRate / (float)decim;
}


/* Run the numerator and denominator filters for the ERMA calculation on the
 * input signal X. Results are put in numer and denom, which should be at least
 * as long as X.
 */
void ermaNumerDenomFilt(float *X, long nX,		/* in */
			float *numer, float *denom)	/* out */
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
