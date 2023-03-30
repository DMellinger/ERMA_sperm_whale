
#include "erma.h"

/* Do IIR filtering. See Oppenheim and Schafer, "Digital Signal Processing",
 * Prentice-Hall, for a description of how it works.
 *
 * You should call initIirFilter first, and then iirFilter to actually filter
 * your input signal X into an output signal Y. You can also process a long
 * signal in chunks: call initIirFilter once at the start of your signal, and
 * then call iirFilter for each successive chunk of the signal, using the value
 * of warmup that got returned from the previous call (and the same values of B1
 * and A1, which don't change). DON'T call initIirFilter again until you start a
 * new signal, as this will wipe out warmup.
 *
 * Compile using optimization:   cc -c filter.c -O3
 */


/* Prepare to filter using the digital filter coefficient vectors B and A, which
 * are inputs each of length n. You can have created vectors iif->B1 and iif->A1
 * of length n and iif->warmup of length 2*n, but if these are NULL, they are
 * allocated. These are filled with data upon return, and are then used in later
 * calls to iirFilter.
 *
 * Returns 0 on success, 1 on failure (out of memory).
 */
int initIirFilter(IIRFILTER *iif,	/* in (iif->A,B) and out (iif->A1,B1) */
		   float **warmup)	/* out; preallocate w/length n*2 */
{
    register int n = iif->n;

    if (iif->A1 == NULL) iif->A1 = malloc(n * sizeof(float));
    if (iif->B1 == NULL) iif->B1 = malloc(n * sizeof(float));
    if (*warmup == NULL) *warmup = malloc(n * sizeof(float) * 2);

    if (iif->A1 == NULL || iif->B1 == NULL || *warmup == NULL)
	return 1;	/* malloc failed - out of memory */

    /* Divide B and A by A[0] */
    for (int i = 0; i < n; i++) {
	iif->B1[i] = iif->B[i] / iif->A[0];
	iif->A1[i] = iif->A[i] / iif->A[0];
    }

    /* Initialize warmup */
    for (int i = 0; i < n * 2; i++)
	(*warmup)[i] = 0.0;

    return 0;
}
    

/* Do IIR filtering using the prepared filter coefficient vectors B and A as
 * returned from initIirFilter. n is the number of filter coefficients in B and
 * A. X is the signal to filter and Y is the result (the filtered signal); Y
 * should be a buffer the same length as X.
 *
 * You can use this function to process a long signal in chunks. Each time, use
 * the value of warmup that got returned from the previous call.
 *
 * Note: warmup contains the last n-1 values of Y (for IIR filters), followed by
 * the last n values of X. Incidentally, this is one more X-value than needed.
 */
void iirFilter(IIRFILTER *iif,		/* in */
	       float *X, int32 nX,	/* in */
	       float *warmup,		/* in & out; length 2n */
	       float *Y)		/* out; length n (same as X) */
{
    int32_t n = iif->n;
    int32_t nA1 = n - 1;

    /* warm up the filter */
    for (int32_t i = 0; i < n-1; i++) {
	double sum = iif->B1[0] * X[i];
/*	printf("Y%d = B%d*X%d", i, 0, i);*/
	for (int32_t k = 1; k < n; k++) {
	    if (i-k >= 0) {
		sum += iif->B1[k] * X[i-k] - iif->A1[k] * Y[i-k];
/*		printf(" + B%d*X%d - A%d*Y%d", k, i-k, k, i-k);*/
	    } else {
		sum += iif->B1[k] * warmup[2*n + i-k]
		    -  iif->A1[k] * warmup[  n + i-k];
/*		printf(" + B%d*w%d - A%d*w%d", k, 2*n + i-k, k, n + i-k);*/
	    }
	}
	Y[i] = sum;
/*	printf("\n");*/
    }
    
    /* main part */
    for (int32_t i = n-1; i < nX; i++) {
	double sum = iif->B[0] * X[i];		//the k=0 case
/*	if (i<10) printf("Y%d = B0*X%d", i, i);*/
	for (int k = 1, j = i-1; k < n; k++, j--) {
	    sum += iif->B1[k] * X[j] - iif->A1[k] * Y[j];
/*	    if (i<10) printf(" + B%d*X%d - A%d*Y%d", k, j, k, j);*/
	}
	Y[i] = sum;
/*	if (i<10) printf("\n");*/
/*	if (i<7) printf(" = Y[%d] = %.3f\n", i, Y[i]);*/
    }

    /* Create warmup vector for output. It has the last n values of Y followed
     * by the last n values of X. */
    for (int32_t i = 0; i < n; i++) {
	warmup[i]   = Y[nX-n + i];
	warmup[i+n] = X[nX-n + i];
    }

    return; 

}

#ifdef NEVER
    /* This assumes no warmup and A[0]=1 */
    for (int32_t i = 0; i < nX; i++) {
	double sum = 0.0;
	for (int k = 0; k < iif->n; k++)	//B loop
	    if (i-k >= 0) {
		sum += iif->B[k] * X[i-k];
		if (i < 7) printf(" + B%d*X%d", k, i-k);
	    }
	for (int k = 1; k < iif->n; k++)	//A loop - starts at A[1]
	    if (i-k >= 0) {
		sum -= iif->A[k] * Y[i-k];
		if (i < 7) printf(" - A%d*Y%d", k, i-k);
	    }
	Y[i] = sum;
	if (i<7) printf(" = Y[%d] = %.3f\n", i, Y[i]);
    }

    /* This assumes no warmup and A[0]=1 */
    for (int32_t i = 0; i < nX; i++) {
	double sum = iif->B[0] * X[i];
	if (i<7) printf("B0*X%d", i);
	for (int k = 1, kLim = MIN(iif->n, i+1); k < kLim; k++) {
	    sum += iif->B[k] * X[i-k] - iif->A[k] * Y[i-k];
	    if (i < 7) printf(" + B%d*X%d - A%d*Y%d", k, i-k, k, i-k);
	}
	Y[i] = sum;
	if (i<7) printf(" = Y[%d] = %.3f\n", i, Y[i]);
    }
#endif //NEVER


#ifdef MAIN

/* Test the routine.
 */

#define INFILE		"/u/dave/det/noncall/h970502-104002-ph1.b120"
#define OUTFILE		"filterTestOut"

/* bandpass filter: elliptical, sRate=12kHz, pass=[150 750], stop=[100 925]
 */
float B[] = {
     0.00273707904023379190,
    -0.02008166987780946000,
     0.06329466335171218800,
    -0.10710189767705458100,
     0.09096109180669032000,
    -0.00000000000034106051,
    -0.09096109180617872900,
     0.10710189767679878500,
    -0.06329466335159850100,
     0.02008166987778636800,
    -0.00273707904023168250,
};
int32 n = NUM_OF(B);

float A[] = {
       1.00000000000000000000,
      -9.49858973175016170000,
      40.86540934511850800000,
    -104.86814596395365100000,
     177.76351625482755000000,
    -207.98512869632430000000,
     170.10269023550396000000,
     -96.02599805401115400000,
      35.80923907627071400000,
      -7.96564410538765790000,
       0.80265166456697212000,
};

#define CHUNKSIZE 10000

main()
{
    int32 fd, i, nsamp;
    struct stat st;
    short *x;
    float *X, *Y, *Z, *warmup;
    
    if ((fd = open(INFILE, O_RDONLY)) < 0) {
	fprintf(stderr, "Can't open file %s\n", INFILE);
	exit(1);
    }
    fstat(fd, &st);
    nsamp = st.st_size / 2;
    printf("%d samples (n = %d)\n", nsamp, n);

    x = G_ALLOCN(short, nsamp);
    fprintf(stderr, "reading...");
    if (read(fd, x, st.st_size) < st.st_size) {
	fprintf(stderr, "Can't read file %s\n", INFILE);
	exit(1);
    }
    close(fd);

    X = G_ALLOCN(float, nsamp);
    Y = G_ALLOCN(float, nsamp);
    Z = G_ALLOCN(float, nsamp);
    fprintf(stderr, "copying...");
    for (i = 0; i < nsamp; i++)		/* copy to float array */
	X[i] = x[i];

    fprintf(stderr, "filtering...");
    warmup = NULL;
    iirFilter(B, A, n, X, nsamp, &warmup, Y);

    fprintf(stderr, "filtering in chunks...");
    warmup = NULL;
    for (i = 0; i < nsamp; i += CHUNKSIZE)
	filter(B, A, n, X + i, MIN(nsamp - i, CHUNKSIZE), &warmup, Z + i);

    for (i = 0; i < nsamp; i++)
	if (Y[i] != Z[i]) {
	    fprintf(stderr, "Error at sample %d.\n", i);
	    exit(1);
	}
    fprintf(stderr, "same result...");

    if ((fd = open(OUTFILE, O_WRONLY | O_TRUNC | O_CREAT, 0666)) < 0) {
	fprintf(stderr, "Can't open file %s\n", OUTFILE);
	exit(1);
    }
    fprintf(stderr, "writing...");
    if (write(fd, Y, nsamp * sizeof(float)) < nsamp * sizeof(float)) {
	fprintf(stderr, "Can't write all bytes to %s\n", OUTFILE);
	exit(1);
    }
    close(fd);
    fprintf(stderr, "\n");
}


#endif		/* MAIN */
