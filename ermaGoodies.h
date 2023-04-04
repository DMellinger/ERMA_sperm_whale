#ifndef _ERMAGOODIES_H_
#define _ERMAGOODIES_H_

/*#include <stdlib.h>	*//* for size_t */
/*#include <stdio.h>	*//* for FILE */
/*#include <stdint.h>	*//* for int32_t */

int bufgrow(void *buf1, size_t *byteSize, size_t newByteSize, void **auxPtr);
char *strsave(char *str);
char *pathFile(char *pathname);
char *pathExt(char *pathname);
char *pathRoot(char *buf, char *pathname);
char *pathDir(char *buf, char *pathname);
int32 findInList(char *str, char **strlist);
int dirExists(char *dirPath);
float meanF(float *x, int32 n);
int32 maxIx(float *x, int32 nX, float *pMaxVal);
int32 minIx(float *x, int32 nX, float *pMinVal);
void printSignalToFile(float *X, int32 nX, char *filename);
void printFloatArray(char *title, float *X, int32 nX);
void writeShortFromFloat(float *X, int32 nX, char *filename);
void writeFloatArray(float *X, int32 nX, char *filename);
size_t readFloatArray(float **pX, size_t *pXSize, size_t maxN, char *filename);
time_t my_timegm(struct tm *tm);
char *timeStrE(char *buf, time_t tE);
char *timeStrD(char *buf, double tD);
float percentile(float *x, size_t xLen, float pct);	//x[] GETS ALTERED!!!
void int16ToFloat(float *dst, int16 *src, size_t n);


/* This allocates a buffer buf large enough to hold n elements and calling
 *  exit() with error code errorNum if out of memory. <buf> should be name
 *  (specifically something I can take the address of) that has your array, and
 *  you should also declare a size_t variable in the same scope as buf with the
 *  name <buf>Size. So plain variable names are fine for <buf>, as are things
 *  like struc->fieldname. Initialize buf to NULL and bufSize to 0 before using
 *  this the first time.
 *
 * This makes bufgrow() easier to use for the most common cases, and avoids the
 * overhead of a function call for inner loops. Alas you can't use the auxPtr
 * feature of bufgrow.
 */
#define BUFGROW(buf, n, errorNum)					\
    if ((buf##Size) < (n) * sizeof((buf)[0])) {				\
    	if (bufgrow(&(buf), &(buf##Size), (n) * sizeof((buf)[0]), NULL))\
	    exit(errorNum);						\
    } else	//the if/else swallows the ';' after a BUFGROW invocation


/* Swap operation. The if/else here swallows up a trailing ';'. */
#define SWAP(a,b) \
    if (1) { typeof(a) tmp = (b); (b) = (a); (a) = tmp; } else


/* Use these "read" functions get data from a file and fix its endianness. */
int readLittleEndian16(int16_t *buf, size_t n, FILE *fp);
int readLittleEndian24(void    *buf, size_t n, FILE *fp);
int readLittleEndian32(int32_t *buf, size_t n, FILE *fp);
int readLittleEndian64(int64_t *buf, size_t n, FILE *fp);
void swapBytes16(int16_t *buf, size_t n);
void swapBytes24(void    *buf, size_t n);
void swapBytes32(int32_t *buf, size_t n);
void swapBytes64(int64_t *buf, size_t n);

#define NUM_OF(x)	(sizeof(x) / sizeof((x)[0]))

#endif    /* _ERMAGOODIES_H_ */
