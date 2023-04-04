
#include "erma.h"

/*********************************************************************
 * This module handles reading of WISPR-format files. A WISPR file has a
 * 512-byte header with ASCII text, mainly specifying the values of a bunch of
 * parameters (the first line also has the WISPR version number), followed by
 * 2-byte samples in little-endian (least followed by most significant bytes)
 * order.
 *
 * There is no code here yet for writing WISPR-format files.
 **********************************************************************/


/* Buffers here have an extra byte of space to allow for an appended '\0' */
#define BUFSZ	(WISPR_HEADER_SIZE + 1)

/* Initialize a WISPR structure.
 */
void wisprInitWISPRINFO(WISPRINFO *wi)
{
    wi->wisprVersion[0]	= '\0';
    wi->sRate		= 0;
    wi->nSamp		= 0;
    wi->sampleSize	= 0;
    wi->timeE		= 0.0;
    wi->fp		= NULL;
    wi->isWave		= 0;
}


/* wisprReadHeader: Read and parse the information in the header of a WISPR
 * file. On entry, 'wi' should be a new or unused WISPRINFO * and 'filename' the
 * name of an unopened file. On return, wi's fields are populated with
 * information from the header, wi->fp is the open file, and the file pointer is
 * set to the start of the sample data. If things are successful, wi is
 * returned, while if there is an error, NULL is returned.
 */
WISPRINFO *wisprReadHeader(WISPRINFO *wi, char *filename)
{
    size_t nScanned, fsize, lnSize, last;
    ssize_t nRead, nReadTotal;
    char *ln, varname[BUFSZ], value[BUFSZ];
    struct tm tm;

    /* Handle .wav files */
    if (strlen(filename) > 4 && !strcmp(filename+strlen(filename)-4, ".wav"))
	return wavReadHeader(wi, filename);

    wi->isWave = 0;
    wi->sampleSize = 2;			//default
    wi->fp = fopen(filename, "r");
    if (wi->fp == NULL)
	return NULL;

    /* nBytesFromHeader=0 here is a flag value. If the WISPR file header has a
     * file_size line, nBytesFromHeader gets set by that; if it doesn't have
     * such a line, nBytesFromHeader stays 0. (-1 would be a better flag value
     * but size_t is unsigned.) */
    size_t nBytesFromHeader = 0;

    /* Main loop: Read a header line, parse it, and store the value in wi. The
     * loop exits when it hits a \0 (NUL) character (the normal case), or after
     * reading WISPR_HEADER_SIZE (512) characters (so we don't read sound data
     * as header).
     */
    nReadTotal = 0;		/* number of bytes */
    ln = NULL;			/* gets malloc'ed by getline() */
    lnSize = 0;
    wi->wisprVersion[0] = '\0';
    for (int32 lineNum = 0; nReadTotal < WISPR_HEADER_SIZE; lineNum++) {
	nRead = getline(&ln, &lnSize, wi->fp);
	/* Reached end of header lines, as indicated by a \0 character? */
	if (nRead <= 0 || (nRead > 0 && ln[0] == '\0')) {
	    break;
	}
	nReadTotal += nRead;

	/* Precaution: Stop sscanf from buffer overwrite on mal-formed line */
	if (lnSize >= WISPR_HEADER_SIZE + 1)
	    ln[WISPR_HEADER_SIZE] = '\0';

	/* Strip trailing CR/LF, whitespace, and ';' */
	last = nRead - 1;
	while (last >= 0 && strchr("\012\015 \t;", ln[last]))
	    ln[last--] = '\0';
	
	/* On first line in file, try to read WISPR version number */
	if (lineNum == 0 && sscanf(ln, "%% WISPR %s", wi->wisprVersion) > 0) {
	    /* don't need to do anything */
	} else {
	    /* Parse an ordinary "varname = value;" line. If the line isn't
	     * this format and sscanf fails, just ignore the line. 
	     */
	    if (sscanf(ln, "%s = %[^\015\012]", varname, value) == 2) {
		if (!strcmp(varname, "sampling_rate")) {
		    sscanf(value, "%f", &wi->sRate);
		} else if (!strcmp(varname, "sample_size")) {
		    sscanf(value, "%d", &wi->sampleSize);
		} else if (!strcmp(varname, "time")) {
		    nScanned = sscanf(value, "'%d:%d:%d:%d:%d:%d",
				      &tm.tm_mon, &tm.tm_mday, &tm.tm_year,
				      &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
		    if (nScanned == 6) {	/* was sscanf successful? */
			tm.tm_isdst = 0;	/* no daylight saving */
			tm.tm_mon -= 1;		/* 0-based indexing */
			tm.tm_year += 100;	/* 2-digit year is 2000's */
			wi->timeE = (double)my_timegm(&tm);
		    }
		} else if (!strcmp(varname, "file_size")) {
		    /* Parse a file size. file_size in the WISPR file header is
		     * the number of 512-byte (WISPR_BLOCK_SIZE) blocks. */
		    fsize = -1;
		    if (sscanf(value, "%ld", &fsize) == 1)
			nBytesFromHeader = fsize * WISPR_BLOCK_SIZE;

		}
	    }
	}
    }
    if (ln != NULL)
	free(ln);

    /* Figure out how many samples are in the file. This is the lesser of how
     * many the file claims are there, and how many are actually there based on
     * file length. */
    fseek(wi->fp, 0L, SEEK_END);
    size_t nBytesFromLen = ftell(wi->fp) - WISPR_HEADER_SIZE;
    size_t nBytes = (nBytesFromHeader == 0)
	? nBytesFromLen : MIN(nBytesFromLen, nBytesFromHeader);
    rewind(wi->fp);
    wi->nSamp = nBytes / wi->sampleSize;

    /* Reset file-reading pointer */
    fseek(wi->fp, WISPR_HEADER_SIZE, SEEK_SET);

    /* Check various fields for validity. */
    if (wi->sRate >= 50 &&
	wi->nSamp > 1000 &&
	(wi->sampleSize == 2 || wi->sampleSize == 3) &&
	wi->timeE >= 946684800) {		//that's 1/1/2000
	return wi;
    } else {
	return NULL;
    }
}


/* Read all the samples as an array of 2- or 3-byte ints and store them as an
 * array of floats in *pSnd (whose size of bufgrow is *pSndSize). Allocates
 * space for the array in *pSnd as needed.
 */
void wisprReadSamples(WISPRINFO *wi, float **pSnd, size_t *pSndSize)
{
    /* store the read-in data temporarily until conversion to float */
    static void *sndBuf = NULL;
    static size_t sndBufSize = 0;
    
    /* Ensure there's enough space in both sndBuf and *pSnd. */
    void *x = sndBuf;
    size_t xSize = sndBufSize;
    if (bufgrow(&sndBuf, &sndBufSize, wi->nSamp * wi->sampleSize, NULL)) {
	fprintf(stderr, "wisprReadSamples: bufgrow failed:\n");
	fprintf(stderr, "	sndBuf x%x->x%x, sndBufSize %ld->%ld\n",
		x, sndBuf, xSize, wi->nSamp * wi->sampleSize);
	exit(ERMA_NO_MEMORY_WISPR_SNDBUF);
    }
/*    fprintf(stderr, "wisprReadSamples: BUFGROW(x%x[len = %ld], %ld) ...",*/
/*	    *pSnd, *pSndSize, wi->nSamp * sizeof((*pSnd)[0]));*/
    BUFGROW(*pSnd, wi->nSamp, ERMA_NO_MEMORY_WISPR_PSND);
/*    fprintf(stderr, " succeeded\n");*/
    if (wi->isWave) {
	if (wavReadData(sndBuf, wi, 0, wi->nSamp))
	    exit(CANT_READ_WAVE);
	int16ToFloat(*pSnd, sndBuf, wi->nSamp);
    } else {
	if (wisprReadFloat(*pSnd, sndBuf, 0, wi->nSamp, wi))
	    exit(CANT_READ_WISPR_FLOATS);
    }
}


/* Read nSam shorts into sams[] from the WISPR file in wi->fp, which should have
 * been opened by wisprReadHeader. The data are read starting at sample
 * offsetSam (the start of the file is offsetSam==0), though if offsetSam is <
 * 0, it means to read at the current position of wi->fp's file-reading
 * pointer. sams should point to an array to hold nSam float samples. Returns 0
 * on success, 1 on failure.
 */
size_t wisprReadFloat(float *sams, void *sndBuf, long offsetSam, size_t nSam,
		      WISPRINFO *wi)
{
    if (offsetSam > wi->nSamp) {
	fprintf(stderr, "wisprReadFloat fail: offsetSam %ld > wi->nSamp %ld\n",
		offsetSam, wi->nSamp);
	return 1;
    }
    nSam = MIN(nSam, wi->nSamp - offsetSam);
    if (offsetSam >= 0)
	fseek(wi->fp, WISPR_HEADER_SIZE + offsetSam * wi->sampleSize, SEEK_SET);
    if (nSam == 0)
	return 0;
    
    /* Read data, fixing endianness if needed. WISPR files are little-endian. */
    switch(wi->sampleSize) {
    case 2:
	/* Read 2-byte samples of whatever endianness and convert to float. */
	readLittleEndian16(sndBuf, nSam, wi->fp);
	int16ToFloat(sams, (int16 *)sndBuf, wi->nSamp);
	break;
    case 3:
	/* Read 3-byte samples of whatever endianness and convert to float. */
	if (fread(sndBuf, 3, nSam, wi->fp) != nSam) {
	    fprintf(stderr, "wisprReadFloat fail: sndBuf x%x, nSam %ld, fp x%x (#%d)",
		    sndBuf, nSam, wi->fp, wi->fp - stdout);
	    exit(CANT_READ_WISPR_SAMPLES);
	}

	int one = 1;				//for testing endianness
	if (*(char *)&one) {			//test endianness
	    //Little endian.
	    unsigned char *s = (unsigned char *)sndBuf;
	    for (size_t i = 0; i < nSam; i++, s += 3) {
		int32 samVal = s[2] << 16 | s[1] << 8 | s[0];
		samVal |= (s[2] & 0x80) ? 0xff000000 : 0;   //sign-extend
		sams[i] = (float)samVal;
	    }
	} else {
	    //Big endian.
	    unsigned char *s = (unsigned char *)sndBuf;
	    for (size_t i = 0; i < nSam; i++, s += 3) {
		int32 samVal = s[0] << 16 | s[1] << 8 | s[2];
		samVal |= (s[0] & 0x80) ? 0xff000000 : 0;   //sign-extend
		sams[i] = (float)samVal;
	    }
	}
	break;
    default:
	exit(WISPR_BAD_SAMPLE_SIZE);
    }	//switch

    return 0;
}


/* Print out some useful information from a WISPRINFO. You must have read the
 * WISPR file header (via wisprReadHeader) first.
 */
void wisprPrintInfo(WISPRINFO *wi)
{
    int32 nSamToPrint = 50;

    printf("WISPRINFO:\n");
    printf(wi->fp == NULL ? "\tfile closed" : "\tfile open, at position %d\n",
	   ftell(wi->fp));
    printf("\tWISPR file version is %s\n", wi->wisprVersion);
    printf("\tn sam is %ld (= %d * %.2f / %d)\n", wi->nSamp, WISPR_BLOCK_SIZE,
	   (float)wi->nSamp / ((float)WISPR_BLOCK_SIZE / wi->sampleSize),
	   wi->sampleSize);
    time_t t = (time_t)floor(wi->timeE);	//gmtime wants pointer to time_t
    printf("\ttime is %s", asctime(gmtime(&t)));
    printf("\tsample size is %d\n", wi->sampleSize);
    printf("\tsample rate is %f\n", wi->sRate);

    /* Print some samples */
    void *rawBuf = malloc(nSamToPrint * wi->sampleSize);
    float *floatBuf = (float *)malloc(nSamToPrint * sizeof(float));
    if (rawBuf == NULL || floatBuf == NULL)
	exit(ERMA_NO_MEMORY_WISPR_PRINT);
    size_t startPos = ftell(wi->fp);
    if (!wisprReadFloat(floatBuf, rawBuf, 0, nSamToPrint, wi)) {
	fseek(wi->fp, startPos, SEEK_SET);
	printf("\tFirst %d samples:", nSamToPrint);   //the \n is supplied below
	for (int32 i = 0; i < nSamToPrint; i++)
	    printf("%s%5d", (i%10 == 0) ? "\n\t" : " ", floatBuf[i]);
	printf("\n");
    }
    free(floatBuf);
    free(rawBuf);
}


/* Clean up a WISPRINFO object that has been initialized.
 */
void wisprCleanup(WISPRINFO *wi)
{
    if (wi->fp != NULL)
	fclose(wi->fp);
}


/* EXAMPLE WISPR FILE HEADER
 * Note that each line ends with CR (^M) and then LF (newline). This is from
 * C:\Dave\sounds\fileFormatExamples\WISPR\WISPR_220527_000107.dat
 *
 * [NB: Keep this at the end of this file as ^M messes up gcc's line-counting.]
------------------------------------
% WISPR 2.1
time = '05:27:22:00:01:07';
instrument_id = 'PERI_1';
location_id = 'PACPNE';
volts = 18.60;
blocks_free = 98.00;
version = 1.2;
file_size = 58575;
buffer_size = 16896;
samples_per_buffer = 8448;
sample_size = 2;
sampling_rate = 50000;
gain = 0;
decimation = 16;
adc_vref = 5.000000;
[The above is 311 bytes; it is followed by 201 '\0' bytes to make the header 512
bytes total.]
------------------------------------
*/
