#ifndef _ERMAERRORS_H_
#define _ERMAERRORS_H_

/* These are exit codes. They might possibly provide useful information if the
 * program is bombing on the glider RPi.
 */
#define ERMA_NO_MEMORY_FILTERCOEFFS	1	/* ermaConfig.c */
#define ERMA_NO_MEMORY_DECIMBUF		2	/* ermaNew.c */
#define ERMA_NO_MEMORY_APPENDCLICKS	3	/* ermaNew.c */
#define ERMA_NO_MEMORY_FILTER_WARMUP	4	/* ermaFilt.c */
#define ERMA_NO_MEMORY_CONFIGVARBUF	5	/* ermaConfig.c */
#define ERMA_NO_MEMORY_CONFIGVAR	6	/* ermaConfig.c */
#define ERMA_NO_MEMORY_FILELIST		7	/* ErmaMain.c */
#define ERMA_NO_MEMORY_UNPROCESSED	8	/* ErmaMain.c */
#define ERMA_NO_MEMORY_AVGPOWER		9	/* findQuiet.c */
#define ERMA_NO_MEMORY_QUIETTIME	10	/* findQuiet.c */
#define ERMA_NO_MEMORY_WISPR_SNDBUF	11	/* wisprFile.c */
#define ERMA_NO_MEMORY_WISPR_PSND	12	/* wisprFile.c */
#define ERMA_NO_MEMORY_ERMACONFIG	13	/* ermaConfig.c */
#define ERMA_NO_MEMORY_STRSAVE		14	/* ermaGoodies.h */
#define ERMA_NO_MEMORY_WISPR_READDATA	15	/* wisprFile.c */
#define ERMA_NO_MEMORY_WISPR_PRINT	16	/* wisprFile.c */
#define ERMA_NO_MEMORY_NUMER_DENOM	17	/* ermaNew.c */
#define ERMA_NO_MEMORY_PEAK		18	/* ermaNew.c */
#define ERMA_NO_MEMORY_NHITS		19	/* encounters.c */
#define ERMA_NO_MEMORY_ENCCOUNT		20	/* encounters.c */
#define ERMA_NO_MEMORY_ENCOUNTERS	21	/* encounters.c */
#define ERMA_NO_MEMORY_CREATECONFIG	22	/* ermaConfig.c */
#define CANT_READ_WAVE			23	/* wisprFile.c */
#define CANT_READ_WISPR_SAMPLES		24	/* wisprFile.c */
#define CANT_READ_WISPR_FLOATS		25	/* wisprFile.c */
#define WISPR_BAD_SAMPLE_SIZE		26	/* wisprFile.c */
#define ERMA_NO_MEMORY_NUMER_DENOM_AVG	27	/* ermaNew.c */
#define ERMA_NO_MEMORY_READFLOATS	28	/* ermaGoodies.c */
#define ERMA_NO_MEMORY_GETTHRESH	29	/* quietTimes.c */
#define CANT_DO_WINDOW_TYPE		30	/* fft.c */

#endif	/* _ERMAERRORS_H */
