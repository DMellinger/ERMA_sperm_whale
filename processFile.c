
#include "erma.h"

/*#include <stdio.h>		*//* for FILE */
/*#include <stdlib.h>*/
/*#include <time.h>		*//* for time_t */
/*#include <sys/stat.h>*/
/*#include "ermaConfig.h"*/
/*#include "wisprFile.h"*/
/*#include "ermaGoodies.h"*/
/*#include "ermaFilt.h"*/
/*#include "processFile.h"*/
/*#include "findQuiet.h"*/
/*#include "ermaNew.h"*/
/*#include "encounters.h"*/


/* This is for a 180-kHz signal downsampled to 60 kHz */
float B[] = { 1, 2, 3 };
float A[] = { };
long nB = NUM_OF(B);



static float *snd = NULL;	/* the sound signal for this file */
static size_t sndSize = 0;	/* for bufgrow */
static QUIETTIMES quietT;	/* (start,stop) times when glider motors off */
static ENCOUNTERS enc;		/* whale encounter times */
static FILECLICKS fileC;	/* clicks found in one file */

void processFile(char *inPath, char *outPath, ERMAPARAMS *ep, ALLCLICKS *allC,
		 double *pTMinE, double *pTMaxE)
{
    WISPRINFO wi;
    static int count = 0;
    static int firstTime = 1;		/* controls initialization */
    FILE *fp;
    char dirPath[256];

    if (firstTime) {
	firstTime = 0;
	ermaFiltPrep(ep);
	initQUIETTIMES(&quietT);
	initFILECLICKS(&fileC);
	initENCOUNTERS(&enc);
    }

    wisprInitWISPRINFO(&wi);
    wisprReadHeader(&wi, inPath);
/*    if (count++ < 2) {*/
/*	printf("Output file name: %s\n", outPath);*/
/*	wisprPrintInfo(&wi);*/
/*    }*/

    *pTMinE = MIN(*pTMinE, wi.timeE);
    *pTMaxE = MAX(*pTMaxE, wi.timeE + wi.nSamp / wi.sRate);
    
    /* Ensure the directory for the output file exists, creating it if not */
    pathDir(dirPath, outPath);		/* get dir name sans file part */
    if (!dirExists(dirPath)) {
	if (mkdir(dirPath, 0777) != 0)
	    return;		/* can't create dir for output file! */
    }

    /* Read sound samples, convert to float */
    wisprReadSamples(&wi, &snd, &sndSize);

    /* Find the useful data spans */
    resetQuietTimes(&quietT);
    findQuietTimes(snd, wi.nSamp, wi.sRate, ep, &quietT);
/*    printf("processFile %s\n", inPath); printQuietTimes(&quietT);*//* DEBUG */

    /* Run ERMA, getting click times in this file in fileC. Append these to allC
     * as Epoch times. */
    ermaSegments(snd, &wi, ep, &quietT, &fileC);
    appendClicks(allC, &fileC, wi.timeE);

    /* Save all detected clicks. */
    saveAllClicks(allC, outPath, inPath);

    wisprCleanup(&wi);
}
