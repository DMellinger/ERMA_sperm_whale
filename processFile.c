
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
/*float B[] = { 1, 2, 3 };*/
/*float A[] = { };*/
/*int32 nB = NUM_OF(B);*/



static float *snd = NULL;	/* the sound signal for this file */
static size_t sndSize = 0;	/* for bufgrow */
static QUIETTIMES quietT;	/* (start,stop) times when glider motors off */
static ENCOUNTERS enc;		/* whale encounter times */
static FILECLICKS fileC;	/* clicks found in one file */

void processFile(char *inPath, char *outPath, ERMAPARAMS *ep, ALLCLICKS *allC,
		 double *pTMinE, double *pTMaxE, char *baseDir)
{
    static int firstTime = 1;		/* controls initialization */
    if (firstTime) {
	firstTime = 0;
	ermaFiltPrep(ep);
	initQUIETTIMES(&quietT);
	initFILECLICKS(&fileC);
	initENCOUNTERS(&enc);
	printf("processFile: file with all clicks: %s\n", outPath);
    }

    WISPRINFO wi;
    wisprInitWISPRINFO(&wi);
    if (!wisprReadHeader(&wi, inPath))		//returns NULL on bad header
	return;
    static int count = 0;
    if (count++ < 2) {
/*	wisprPrintInfo(&wi);*/
    }

    *pTMinE = MIN(*pTMinE, wi.timeE);
    *pTMaxE = MAX(*pTMaxE, wi.timeE + wi.nSamp / wi.sRate);
    
    /* Ensure the directory for the output file exists, creating it if not */
    char dirPath[256];
    pathDir(dirPath, outPath);		/* get dir name sans file part */
    if (!dirExists(dirPath)) {
	if (mkdir(dirPath, 0777) != 0)
	    return;		/* can't create dir for output file! */
    }

    /* Read sound samples, convert to float */
    wisprReadSamples(&wi, &snd, &sndSize);

    /* Find the useful data spans */
    resetQuietTimes(&quietT);
    findQuietTimes(snd, wi.nSamp, wi.sRate, ep, &quietT, baseDir);
    #ifdef PRINT_QUIET_TIMES
    printQuietTimes(&quietT);		/* DEBUG */
    #endif

    /* Run ERMA (in ermaNew.c), getting click times in this file in
     * fileC. Append these to allC as Epoch times. */
    int32 startClickNo = allC->n;
    ermaSegments(snd, &wi, ep, &quietT, &fileC);
    appendClicks(allC, &fileC, wi.timeE);

    /* Save all detected clicks. */
    saveNewClicks(allC, startClickNo, wi.timeE, outPath, inPath);

    wisprCleanup(&wi);
}
