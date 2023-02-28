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
#define ERMA_NO_MEMORY_ERMACONFIG	12	/* ermaConfig.c */
#define ERMA_NO_MEMORY_STRSAVE		13	/* ermaGoodies.h */
#define ERMA_NO_MEMORY_WISPR_READDATA	14	/* wisprFile.c */
#define ERMA_NO_MEMORY_WISPR_PRINT	15	/* wisprFile.c */
#define ERMA_NO_MEMORY_NUMER_DENOM	16	/* ermaNew.c */
#define ERMA_NO_MEMORY_PEAK		17	/* ermaNew.c */
#define ERMA_NO_MEMORY_NHITS		18	/* encounters.c */
#define ERMA_NO_MEMORY_ENCCOUNT		19	/* encounters.c */
#define ERMA_NO_MEMORY_ENCOUNTERS	20	/* encounters.c */
#define ERMA_NO_MEMORY_CREATECONFIG	21	/* ermaConfig.c */
#define CANT_READ_WAVE			22	/* wisprFile.c */
#define CANT_READ_WISPR			23	/* wisprFile.c */

#endif	/* _ERMAERRORS_H */
