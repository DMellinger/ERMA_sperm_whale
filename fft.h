#ifndef _FFT_H
#define _FFT_H

typedef float ffttype;


/* Takes a real array, an imaginary array, and m=log2(n).
 * See Ishmael.h for definition of ffttype.
 */
void fft(ffttype *re, ffttype *im, int32 m);


/* Inverse FFT -- args as for fft above.
 */
void ifft(ffttype *re, ffttype *im, int32 m);


//Fill up an n-vector of ffttype's with numbers representing the FFT window
//type specified by typ.  The return value is the input vector.  If normalize
//is non-zero, normalize the window so that different-length FFTs of white
//Gaussian noise will have result values in the same range.
ffttype *fftMakeWindow(ffttype *vector, int32 typ, int32 n, int32 normalize);


//Window types for fftWindow 'typ' arg.
#define WIN_HAMMING       1
#define WIN_HANNING       2
#define WIN_BLACKMAN      3
#define WIN_BARTLETT      4
#define WIN_RECTANGULAR   5
#define WIN_KAISER        6

#endif	//_FFT_H

