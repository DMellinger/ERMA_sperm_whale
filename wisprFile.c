
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
    int fd, lineNum, nScanned;
    long fsize;
    size_t lnSize, last;
    ssize_t nRead, nReadTotal;
    char *ln, varname[BUFSZ], value[BUFSZ];
    struct tm tm;

    /* Handle .wav files */
    if (strlen(filename) > 4 && !strcmp(filename+strlen(filename)-4, ".wav"))
	return wavReadHeader(wi, filename);

    wi->isWave = 0;
    wi->fp = fopen(filename, "r");
    if (wi->fp == NULL)
	return NULL;

    /* Figure out how many samples are in the file, assuming sample size = 2.
     * This might get overridden below by the file_size line in the file.
     */
    fseek(wi->fp, 0L, SEEK_END);
    wi->nSamp = (ftell(wi->fp) - WISPR_HEADER_SIZE) / WISPR_SAMPLE_SIZE;
    rewind(wi->fp);

    /* Main loop: Read a header line, parse it, and store the value in wi. The
     * loop exits when it hits a \0 (NUL) character (the normal case), or after
     * reading WISPR_HEADER_SIZE (512) characters (so we don't read sound data
     * as header).
     */
    nReadTotal = 0;		/* number of bytes */
    ln = NULL;			/* gets malloc'ed by getline() */
    lnSize = 0;
    wi->wisprVersion[0] = '\0';
    for (lineNum = 0; nReadTotal < WISPR_HEADER_SIZE; lineNum++) {
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
		     * the number of 512-byte (WISPR_BLOCK_SIZE) blocks. If the
		     * file is shorter than this, the smaller value is used. */
		    fsize = -1;
		    sscanf(value, "%ld", &fsize);
		    wi->nSamp = MIN(wi->nSamp,
				   fsize*WISPR_BLOCK_SIZE / WISPR_SAMPLE_SIZE);
		}
	    }
	}
    }
    if (ln != NULL)
	free(ln);

    fseek(wi->fp, WISPR_HEADER_SIZE, SEEK_SET);
    return wi;
}


/* Read the samples as an array of shorts and store them as an array of floats.
 * Allocates space for the arrays as needed.
 */
void wisprReadSamples(WISPRINFO *wi, float **pSnd, size_t *pSndSize)
{
    /* store the read-in data temporarily until conversion to float */
    static int16_t *sndBuf = NULL;
    static size_t sndBufSize = 0;

    /* Ensure there's enough space. */
    
    BUFGROW(sndBuf, wi->nSamp, ERMA_NO_MEMORY_WISPR_SNDBUF);
    BUFGROW(*pSnd,  wi->nSamp, ERMA_NO_MEMORY_WISPR_SNDBUF);
    if (wi->isWave) {
	if (wavReadData(sndBuf, wi, 0, wi->nSamp))
	    exit(CANT_READ_WAVE);
    } else {
	if (wisprReadData(&sndBuf, wi, 0, wi->nSamp))
	    exit(CANT_READ_WISPR);
    }

    /* Convert shorts to floats. */
    for (long i = 0; i < wi->nSamp; i++)
	(*pSnd)[i] = (float)sndBuf[i];
}


/* Read nSam shorts into *bufp from the WISPR file in wi->fp, which should have
 * been opened by wisprReadHeader. The data are read starting at sample
 * offsetSam (the start of the file is offsetSam==0), though if offsetSam is
 * negative, it means to continue reading at the current position in the
 * file. *bufp should point to a buffer to hold nSampToRead samples of 2-byte
 * data. *bufp can also point to NULL, in which case it gets malloc'ed here (and
 * the caller should eventually free it) and *bufp gets set to the malloc'ed
 * buffer. Returns 0 on success, 1 on failure.
 */
long wisprReadData(int16_t **bufp, WISPRINFO *wi, long offsetSam, long nSam)
{
    unsigned char *p, *pEnd;
    unsigned char tmp;
    int one = 1;			/* for testing endianness */

    nSam = MIN(nSam, wi->nSamp - offsetSam);
    if (nSam <= 0)
	return 1;
    if (*bufp == NULL) {
	*bufp = malloc(nSam * WISPR_SAMPLE_SIZE);
	if (*bufp == NULL)
	    exit(ERMA_NO_MEMORY_WISPR_READDATA);
    }
    if (offsetSam >= 0)
	fseek(wi->fp, WISPR_HEADER_SIZE + offsetSam * WISPR_SAMPLE_SIZE,
	      SEEK_SET);
    
    /* Read data, fixing endianness if needed. WISPR files are little-endian. */
    readLittleEndian16(*bufp, nSam, wi->fp);

    return 0;
}


/* Print out some useful information from a WISPRINFO.
 */
void wisprPrintInfo(WISPRINFO *wi)
{
    long nSamToPrint = 50, nSamRead, i;
    short *buf;

    printf("WISPRINFO:\n");
    printf(wi->fp == NULL ? "\tfile closed" : "\tfile open, at position %d\n",
	   ftell(wi->fp));
    printf("\tWISPR file version is %s\n", wi->wisprVersion);
    printf("\tn sam is %ld (= %d * %.2f / %d)\n", wi->nSamp, WISPR_BLOCK_SIZE,
	   (float)wi->nSamp / ((float)WISPR_BLOCK_SIZE / WISPR_SAMPLE_SIZE),
	   WISPR_SAMPLE_SIZE);
    time_t t = (time_t)floor(wi->timeE);	//gmtime wants pointer to time_t
    printf("\ttime is %s", asctime(gmtime(&t)));
    printf("\tsample size is %d\n", wi->sampleSize);
    printf("\tsample rate is %f\n", wi->sRate);

    /* Print some samples */
    buf = (short *)malloc(nSamToPrint * WISPR_SAMPLE_SIZE);
    if (buf == NULL) exit(ERMA_NO_MEMORY_WISPR_PRINT);
    wisprReadData(&buf, wi, 0L, nSamToPrint);
    fseek(wi->fp, -nSamToPrint * WISPR_SAMPLE_SIZE, SEEK_CUR);
    printf("\tFirst %d samples:", nSamRead);	/* the \n is supplied below */
    for (i = 0; i < nSamRead; i++)
	printf("%s%5d", (i%10 == 0) ? "\n\t" : " ", buf[i]);
    printf("\n");
    free(buf);
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
