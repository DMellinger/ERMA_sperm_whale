//---------------------------------------------------------------------------
#include "erma.h"

/*
 * FFT routine from
 *	Kent S. Harris - consultant - 408-996-1294 - GEnie: K.HARRIS2
 *	ksh@scampi.sc-scicon.com == {ihnp4,hoptoad,leadsv,ames}!scampi!ksh
 *
 * Adapted for speed (mainly pointer arithmetic)
 * by David K. Mellinger, mellinger@pmel.noaa.gov .
 *
 *
 * Fast Fourier Transform -- decimation-in-time, in-place, radix-2  --
 * after Cooley, Lewis, and Welch.  (Actually this algorithm is closer
 * to that of Singleton.)
 *
 * Args:
 *	re 	is the real part of the array (input and output)
 *	im	is the imaginary part of the array (input and output)
 *	2**m	is number of points
 *
 * This algorithm uses the recursion relation:
 *
 *	 ql    q    q(l - 1)
 *	W   = W  * W
 *	 N     N    N
 *
 * where
 *
 *       q    -2pi/N
 *	W  = e
 *       N
 *
 * to generate the l'th coefficient from the (l-1)'st using direct
 * calculation of sines and cosines only for the evaluation of the
 * increment in W.  Cooley, Lewis, and Welch state that this
 * procedure requires only about 15% more operations for N=1024 than
 * would be required for table storage for the entire W(k) array.
 */

#define PI	((double)3.14159265358979323846)

/* Takes a real array, an imaginary array, and m=log2(n).
 */
void fft(ffttype *re, ffttype *im, int32 m)
{
    register ffttype	*pre, *pim, *qre, *qim, *pEnd;
    register ffttype	tre, tim;		/* temporary */
    double		Wre, Wim;		/* -1^(1/N) */
    double		Ure, Uim;		/* W^i */
    double		Tmp;
    register ffttype	ure, uim;		/* float version of U */
    int32		n, nv2, nm1, i, j, k, l, le, le1;
    
    if (m < 1) return;		/* illegal m */

    n = 1 << m;			/* actual number of points */
    nv2 = n / 2;
    nm1 = n - 1;

    /* in-place bit-reversing shuffle of input array */
    for (i = 0, j = 0; i < nm1; i++) {
	if (i < j) {
	    tre   = re[i];	tim   = im[i];		/* swap */
	    re[i] = re[j];	im[i] = im[j];
	    re[j] = tre;	im[j] = tim;
	}
	for (k = nv2; k <= j; k >>= 1)
	    j -= k;
	j += k;
    }

    /* transform shuffled input data */
    for (l = 1; l <= m; l++) {		/* l counts stages from 1 through m */
	le = 1 << l;
	le1 = le >> 1;
	Ure = 1.0;
	Uim = 0.0;
	Wre =  cos(PI / (double)le1);
	Wim = -sin(PI / (double)le1);

	/* j indexes the powers of w (the twiddle factors) as required
	 * by the different butterflies within a given stage.
	 *
	 */
	for (j = 0; j < le1; j++) {

	    /* convert double U to ffttype u */
	    ure = (ffttype)Ure;
	    uim = (ffttype)Uim;

	    /* p and q point to the two cells to butterfly */
	    pre = &re[j];		/* i */
	    pim = &im[j];
	    qre = pre + le1;		/* i + 2^(l-1) */
	    qim = pim + le1;
	    pEnd = &re[n];

	    for ( ; pre < pEnd; pre += le, pim += le, qre += le, qim += le) {
		/* do the butterfly calculation:
		 *    [in parallel]  f(p) = f(p) + W*f(q)
		 *                   f(q) = f(p) - W*f(q)
		 */
		tre = (*qre * ure) - (*qim * uim);	/* tmp = W*f(q) */
		tim = (*qre * uim) + (*qim * ure);
		*qre = *pre - tre;			/* f(q) = f(p) - tmp */
		*qim = *pim - tim;
		*pre += tre;				/* f(p) = f(p) + tmp */
		*pim += tim;
	    }
	    /* do recursion relation discussed above:   U = U * W    */
	    Tmp = (Ure * Wre) - (Uim * Wim);
	    Uim = (Ure * Wim) + (Uim * Wre);
	    Ure = Tmp;
	}
    }
}


/* Inverse FFT -- args as for fft above.
 */
void ifft(ffttype *re, ffttype *im, int32 m)
{
    int32 n = 1 << m;
    register ffttype n1, n2;
    register ffttype *pre, *pim, *pEnd;

    /* conjugate */
    for (pim = im, pEnd = pim + n; pim < pEnd; pim++)
	*pim = -(*pim);

    fft(re, im, m);

    /* conjugate and divide by n */
    n1 = (ffttype)1.0 / (ffttype)n;
    n2 = -n1;
    for (pre = re, pim = im, pEnd = pre + n; pre < pEnd; ) {
	*pre++ *= n1;
	*pim++ *= n2;
    }
}


//Fill up an n-vector of ffttype's with numbers representing the FFT window
//type specified by typ.
ffttype *fftMakeWindow(ffttype *vector, int32 typ, int32 n, int32 normalize)
{
    ffttype nm1 = (ffttype)n - (ffttype)1.0;
    ffttype np1 = (ffttype)n + (ffttype)1.0;
    int32 i;
    
    for (i = 0; i < n; i++) {
	switch(typ) {
	case WIN_HAMMING:
	    vector[i] = 0.54 - 0.46 * cos(2*M_PI * (ffttype)i / nm1);
	    break;
	case WIN_HANNING:
	    vector[i] = 0.5 - 0.5 * cos(2*M_PI * (ffttype)(i+1) / np1);
	    break;
	case WIN_BLACKMAN:
	    vector[i] = 0.42 - 0.5  * cos(2*M_PI * (ffttype)i / nm1)
		+ 0.08 * cos(4*M_PI * (ffttype)i / nm1);
	    break;
	case WIN_BARTLETT:		//same as triangular
	    vector[i] = 1.0 - fabs((ffttype)i - nm1/2.0) / (nm1/2.0);
	    break;
	case WIN_RECTANGULAR:
	    vector[i] = 1.0;
	    break;
	case WIN_KAISER:
	    exit(CANT_DO_WINDOW_TYPE);
	default:
	    exit(CANT_DO_WINDOW_TYPE);
	}
    }
    if (normalize) {
	//Normalize window so FFTs of white noise come out the same.
	//Normalization factor is sum of window coeffs divided by sqrt(n).
	ffttype sum = 0.0;
	int32 i;
	for (i = 0; i < n; i++)
	    sum = sum + vector[i];
	sum = sum / sqrt((float)n);
	for (i = 0; i < n; i++)
	    vector[i] = vector[i] / sum;
    }
    return vector;
}
