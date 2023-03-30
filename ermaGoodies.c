
#include "erma.h"


/* Make sure that buf is large enough to hold at least newByteSize bytes.
 * byteSize is a auxiliary variable that holds the real size of buf.  If auxPtr
 * is non-NULL, it is assumed to point into buf, and is adjusted accordingly
 * whenever buf is moved by realloc.
 *
 * A typical call looks like this:
 *	bufgrow(&buf, &buf_size, newsize, &auxptr);
 *
 * buf and auxPtr (if used) should start out with NULL and byteSize with 0.
 * Returns 0 on success, 1 if out of memory.
 */
int bufgrow(void *buf1, size_t *byteSize, size_t newByteSize, void **auxPtr)
{
    void *newbuf;
    int tmp;
    void **buf = buf1;			/* to get around compiler complaint */

    if (newByteSize <= *byteSize)
	return 0;

    newByteSize = MAX(newByteSize, MAX(*byteSize * 2, 16));
    newbuf = *buf ? realloc(*buf, newByteSize) : malloc(newByteSize);
    if (newbuf == NULL)
	return 1;
    if (auxPtr != NULL)
	*auxPtr = (void *)((size_t)auxPtr + ((size_t)newbuf - (size_t)buf));
    *buf = newbuf;
    tmp = *byteSize;
    *byteSize = newByteSize;
    return 0;
}


/* Put a copy of a string in a newly-allocated buffer, and return a pointer to
 * the buffer. You should call free(buf) on the returned buffer when you're done
 * with it.
 *
 * [later] Just found out there's a library function, strdup, that does this.
 */
char *strsave(char *str)
{
    char *buf = malloc(strlen(str) + 1);

    if (buf == NULL)
	exit(ERMA_NO_MEMORY_STRSAVE);
    
    strcpy(buf, str);
    return buf;
}


/* Return the file part of a path name, that is, the part of the path name after
 * the last / or \. The return value is just a pointer into the passed-in
 * string, so it should NOT be freed later.
 */
char *pathFile(char *pathname)
{
    char *fn, *fn0, *fn1;

    //Need to handle the case in which pathname has both / and \ in it.
    fn0 = strrchr(pathname, '/');
    fn1 = strrchr(pathname, '\\');
    if (fn0 == NULL && fn1 == NULL)
	fn = pathname;			//no directory is present
    else if (fn0 == NULL)
	fn = fn1 + 1;			//only \ separator(s) are present
    else if (fn1 == NULL)
	fn = fn0 + 1;			//only / separator(s) are present
    else
	fn = MAX(fn0, fn1) + 1;		//both / and \ are present

    return fn;
}


/* Return the trailing extension from a pathname (without the '.').  The result
 * should NOT be freed, since it's a pointer into the original string.
 */
char *pathExt(char *pathname)
{
    char *c;

    /* Apply pathFile first in case there's no extension on the file but a
     * leading directory name does have a '.'.
     */
    if (c = strrchr(pathFile(pathname), '.'))
	return c + 1;
    else
	return NULL;
}


/* Return a pathname minus its extension but including leading directory
 * names. The result is put into buf, which should be big enough to hold it.
 * buf is returned.
 */
char *pathRoot(char *buf, char *pathname)
{
    char *e = pathExt(pathname);
    int32 n;

    n = strlen(pathname) - (e!=NULL ? strlen(e)+1 : 0);
    strncpy(buf, pathname, n);		/* doesn't append \0! */
    buf[n] = '\0';
    return buf;
}


/* Return the directory name of a path. The directory name is put in the given
 * buffer, which must be big enough to hold it, and the buffer is returned. The
 * buffer has no trailing / or \. If the path doesn't have a directory name, "."
 * is returned in the buffer.
 */
char *pathDir(char *buf, char *pathname)
{
    char *last0 = strrchr(pathname, '/');	/* might be NULL but that's ok*/
    char *last1 = strrchr(pathname, '\\');	/* might be NULL but that's ok*/
    char *last = MAX(last0, last1);
    
    if (last) {
	strncpy(buf, pathname, last - pathname);
	buf[last - pathname] = '\0';
    } else
	strcpy(buf, ".");

    return buf;
}


/* Find a string in a NULL-terminated list of strings. Returns the index of the
 * found item, or -1 if it's not there.
 */
int32 findInList(char *str, char **strlist)
{
    for (int32 i = 0; strlist[i] != NULL; i++)
	if (!strcmp(str, strlist[i]))
	    return i;

    return -1;
}


/* Return non-zero if a given directory exists, 0 if not.
 */
int dirExists(char *dirPath)
{
    struct stat statbuf;
    
    if (stat(dirPath, &statbuf) != -1)
	return S_ISDIR(statbuf.st_mode);
    
    return 0;
}
    

/* Calculate the mean of an array of n floats.
 */
float meanF(float *x, int32 n)
{
    double sum = 0.0L;
    for (int32 i = 0; i < n; i++)
	sum += x[i];

    return (float)(sum / (double)n);	//do the division with doubles
}


/* Return the *index* of the largest element in x[0 .. nX). If pMaxVal is
 * non-NULL, also put that largest value in it. If there are multiple copies of
 * the largest value, return the first one.
 */
int32 maxIx(float *x, int32 nX, float *pMaxVal)
{
    int32 maxIx = 0;
    float maxVal = -FLT_MAX;

    for (int32 i = 0; i < nX; i++) {
	if (x[i] > maxVal) {
	    maxVal = x[i];
	    maxIx = i;
	}
    }
    if (pMaxVal != NULL)
	*pMaxVal = maxVal;

    return maxIx;
}

	    
/* Return the *index* of the smallest element in x[0 .. nX). If pMinVal is
 * non-NULL, also put that smallest value in it. If there are multiple copies of
 * the smallest value, return the first one.
 */
int32 minIx(float *x, int32 nX, float *pMinVal)
{
    int32 minIx = 0;
    float minVal = FLT_MAX;

    for (int32 i = 0; i < nX; i++) {
	if (x[i] < minVal) {
	    minVal = x[i];
	    minIx = i;
	}
    }
    if (pMinVal != NULL)
	*pMinVal = minVal;

    return minIx;
}


/* Save a signal to a file. Used in debugging.
 */
void writeShortFromFloat(float *X, int32 nX, char *filename)
{
#if !ON_RPI
    FILE *fp = fopen(filename, "w");
    for (int32 i = 0; i < nX; i++) {
	short xShort = (short)X[i];
	fwrite(&xShort, sizeof(xShort), 1, fp);
    }
    fclose(fp);
#endif
}

	    
/* Save a signal to a file. Used in debugging.
 */
void writeFloatArray(float *X, int32 nX, char *filename)
{
#if !ON_RPI
    FILE *fp = fopen(filename, "w");
    fwrite(X, sizeof(X[0]), nX, fp);
    fclose(fp);
#endif
}

	    
/* Save a signal to a file. Used in debugging.
 */
void printSignal(float *X, int32 nX, char *filename)
{
#if !ON_RPI
    FILE *fp = fopen(filename, "w");
    for (int32 i = 0; i < nX; i++)
	fprintf(fp, "%.3f\n", X[i]);
    fclose(fp);
#endif
}

	    
/* Convert a struct tm that's in UTC into a time_t (seconds since 1/1/1970).
 * From StackOverflow. This is like the library function timegm, but that is
 * non-standard and not necessarily available everywhere.
 */
time_t my_timegm(struct tm *tm)
{
    time_t ret;
    char *tz = getenv("TZ");

    if (tz)
        tz = strdup(tz);
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(tm);
    if (tz) {
        setenv("TZ", tz, 1);
        free(tz);
    } else
        unsetenv("TZ");
    tzset();
    return ret;
}


/* Construct a string in the format YYMMDD-hhmmss from a (time_t) timestamp tE,
 * which represents seconds since The Epoch (1/1/1970). The string is written
 * into a caller-supplied buffer 'buf', which must have space for at least 14
 * bytes. Returns 'buf'.
 */
char *timeStrE(char *buf, time_t tE)
{
    struct tm t_tm = *gmtime(&tE);
    
    //Print start- and stop-times of the encounter.
    sprintf(buf, "%02d%02d%02d-%02d%02d%02d",
	    t_tm.tm_year % 100,	//just last two digits of year
	    t_tm.tm_mon + 1,	//tm_mon numbers months starting at 0
	    t_tm.tm_mday,	//day of month
	    t_tm.tm_hour,
	    t_tm.tm_min,
	    t_tm.tm_sec);
    return buf;
}


/* Construct a string in the format YYMMDD-hhmmss from a (double) day number tD,
 * which represents days since The Epoch (1/1/1970). The string is written into
 * a caller-supplied buffer 'buf', which must have space for at least 14
 * bytes. Returns 'buf'.
 */
char *timeStrD(char *buf, double tD)
{
    return timeStrE(buf, round(tD * 24*60*60));
}


/* Convert an array of shorts (i.e., int16s) to floats.
 */
void int16ToFloat(float *dst, int16 *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
	dst[i] = (float)src[i];
}


/************************ Endian-correction operations **********************/

/* Read 16-bit data into buf from an open file that has little-endian data (like
 * WISPR and WAVE files), making sure it's formatted correctly for this machine.
 * buf must be large enough to hold the data. n is the number of shorts to read
 * into buf[]. Returns 1 on success, 0 on failure.
 */
int readLittleEndian16(int16_t *buf, size_t n, FILE *fp)
{
    int one = 1;		/* for testing endianness */

    if (fread(buf, 2, n, fp) != n)
	return 0;

    if (*(char *)&one != 1)	/* test for big-endianness */
	swapBytes16(buf, n);	/* this is a big-endian machine; swap bytes */

    return 1;
}


/* Read int24 data into buf from an open file that has little-endian data (like
 * WISPR and WAVE files), making sure it's formatted correctly for this machine.
 * buf must be large enough to hold the data. n is the number of int24s to read
 * into buf[]. Returns 1 on success, 0 on failure.
 */
int readLittleEndian24(void *buf, size_t n, FILE *fp)	//int24 doesn't exist
{
    int one = 1;		/* for testing endianness */

    if (fread(buf, 3, n, fp) != n)
	return 0;
    
    if (*(char *)&one != 1)	/* test for big-endianness */
	swapBytes24(buf, n);	/* this is a big-endian machine; swap bytes */

    return 1;
}
    

/* Read int32 data into buf from an open file that has little-endian data (like
 * WISPR and WAVE files), making sure it's formatted correctly for this machine.
 * buf must be large enough to hold the data. n is the number of int32s to read
 * into buf[]. Returns 1 on success, 0 on failure.
 */
int readLittleEndian32(int32_t *buf, size_t n, FILE *fp)
{
    int one = 1;		/* for testing endianness */

    if (fread(buf, 4, n, fp) != n)
	return 0;
    
    if (*(char *)&one != 1)	/* test for big-endianness */
	swapBytes32(buf, n);	/* this is a big-endian machine; swap bytes */

    return 1;
}
    

/* Read int64 data into buf from an open file that has little-endian data (like
 * WISPR and WAVE files), making sure it's formatted correctly for this machine.
 * buf must be large enough to hold the data. n is the number of int64s to read
 * into buf[]. Returns 1 on success, 0 on failure.
 */
int readLittleEndian64(int64_t *buf, size_t n, FILE *fp)
{
    int one = 1;		/* for testing endianness */

    if (fread(buf, 4, n, fp) != n)
	return 0;
    
    if (*(char *)&one != 1)	/* test for big-endianness */
	swapBytes64(buf, n);	/* this is a big-endian machine; swap bytes */

    return 1;
}
    

/* Exchange two unsigned char values.
 */
#define SWAPBYTE_UCHAR(a,b) \
    { unsigned char tmp; tmp = (a); (a) = (b); (b) = tmp; }


/* Swap bytes in a buffer of shorts.
 */
void swapBytes16(int16_t *buf, size_t n)
{
    register unsigned char *p, *pEnd;
    register unsigned char tmp;

    pEnd = (unsigned char *)(buf + n);
    for (p = (unsigned char *)buf; p < pEnd; p += 2) {
	SWAPBYTE_UCHAR(p[0], p[1])
    }
}


/* Swap bytes in a buffer of int24s.
 */
void swapBytes24(void *buf, size_t n)	//use (void *) since int24 doesn't exist
{
    register unsigned char *p, *pEnd;

    pEnd = (unsigned char *)buf + n*3;		//cast buf BEFORE adding n*3
    for (p = (unsigned char *)buf; p < pEnd; p += 3) {
	SWAPBYTE_UCHAR(p[0], p[2])
    }
}


/* Swap bytes in a buffer of int32s.
 */
void swapBytes32(int32_t *buf, size_t n)
{
    register unsigned char *p, *pEnd;

    pEnd = (unsigned char *)(buf + n);
    for (p = (unsigned char *)buf; p < pEnd; p += 4) {
	SWAPBYTE_UCHAR(p[0], p[3])
	SWAPBYTE_UCHAR(p[1], p[2])
    }
}


/* Swap bytes in a buffer of int64s.
 */
void swapBytes64(int64_t *buf, size_t n)
{
    register unsigned char *p, *pEnd;

    pEnd = (unsigned char *)(buf + n);
    for (p = (unsigned char *)buf; p < pEnd; p += 8) {
	SWAPBYTE_UCHAR(p[0], p[7])
	SWAPBYTE_UCHAR(p[1], p[6])
	SWAPBYTE_UCHAR(p[2], p[5])
	SWAPBYTE_UCHAR(p[3], p[4])
    }
}
/********************* End of endian-correction operations *******************/
