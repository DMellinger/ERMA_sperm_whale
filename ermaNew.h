#ifndef _ERMANEW_H_
#define _ERMANEW_H_

/* FILECLICKS holds the whale clicks found in a single file.
 */
typedef struct {
    float *timeS;	/* times of clicks in file, s */
    size_t timeSSize;	/* for bufgrow */
    int32_t n;		/* number of clicks found this file */
} FILECLICKS;


/* ALLCLICKS holds the whale clicks found in all files. As such, it encodes time
 * using doubles so as to have enough bit resolution (to at least milliseconds).
 */
typedef struct {
    double *timeD;	/* click times, in *days* since the Epoch (1/1/1970) */
    size_t timeDSize;	/* for bufgrow */
    int32_t n;		/* number of elements in timeD */
} ALLCLICKS;


void initFILECLICKS(FILECLICKS *fc);
void resetFILECLICKS(FILECLICKS *fc);
void initALLCLICKS(ALLCLICKS *ac);
void ermaSegments(float *snd, WISPRINFO *wi, ERMAPARAMS *ep, QUIETTIMES *qt,
		  FILECLICKS *fc);
void ermaNew(float *seg, int32_t nSam, float segT0, float sRate, ERMAPARAMS *ep,
	     FILECLICKS *fc);
void appendClicks(ALLCLICKS *ac, FILECLICKS *fc, double fileTime);
void saveNewClicks(ALLCLICKS *allC, int32 startN, double fileTimeE,
		   char *outPath, char *inPath);


#endif	/* _ERMANEW_H_ */
