#ifndef _ERMACONFIG_H_
#define _ERMACONFIG_H_

/*#include <stdlib.h>*/
/* ERMACONFIG holds a vector of parameter names (varname) and a vector of
 * corresponging values (value, which has strings). These are read from the
 * rpi.cnf file. The strings are turned into other data types, like floats,
 * longs, etc., in ermaConfig.c/gatherErmaParams().
 */
typedef struct {
    char **varname;	/* array of varnames */
    char **value;	/* array of value strings */
    size_t varnameSize, valueSize;	/* for bufgrow */
    int32 n;		/* number of valid members of varname[] and value[] */
} ERMACONFIG;


/* ERMAPARAMS has the configurable settings used to run ERMA. The default values
 * for these parameters are in ermaConfig.c, but any of them can be overridden
 * by an entry in the rpi.cnf file. This overriding is done in
 * ermaConfig.c/gatherErmaParams().
 *
 * NB: If you change anything here, be sure to keep the list of default values
 * in the same order in the definition of ep in ErmaMain.c. Also update
 * ermaConfig.c/gatherErmaParams to parse the value from the rpi.cnf.
 */
typedef struct 
{   /* file names (excluding baseDir and configFileName, which are fixed) */
    char *filesProcessed;//file w/list of files ERMA has attempted to process
    char *infilePattern;//picks out files to process in baseDir directory
    char *outDir;	//where to put results of processing
    char *allDetsFiles;	//list of files that have ALL detections
    char *encFileList;	//has list of files to upload by WISPR, which clears it
    char *wisprEncFileDir;//pathname of baseDir when SD card is mounted on WISPR
    char *allDetsPrefix;//prefix for files holding times of all clicks detected
    char *encDetsPrefix;//prefix for files holding times of dets in encounters

    /* GPIO pins: */
    int32 gpioWisprActive;//input pin # to tell RPi to process files
    int32 gpioRPiActive;	//output pin # to tell WISPR that RPi is busy

    /* stuff for filtering: */
    float *dsfA, *dsfB;	//IIR filter coefficients for downsampling filter
    int32 dsfN;		//length of dsfA and dsfB (= filter order + 1)
    int32 decim;		//decimation factor
    float *numerA, *numerB; //IIR filter coefficients for numerator filter
    int32 numerN;	//length of dsfA and dsfB (= filter order + 1)
    float *denomA, *denomB; //IIR filter coefficients for denominator filter
    int32 denomN;	//length of dsfA and dsfB (= filter order + 1)

    /* stuff for ERMA algorithm: */
    float decayTime;	//exponential-decay constant for expdecay()
    float powerThresh;  //det threshold per kHz in numerator band
    float refractoryT;	//stop peaks w/in this many s after start of click
    float peakNbdT;	//peak is highest of any within this many s
    float peakDurLims;	//peak can't last longer than this many s
    float avgT;		//averaging time before computing ERMA ratio
    float ratioThresh;	//minimum ERMA ratio (linear, not dB)
    float ignoreThresh;	//sams this loud don't affect running mean
    float ignoreLimT;	//...unless they last for this long
    
    /* stuff for testClickDets: */
    float minRate;	//min req'd click period over whole file, s
    float iciRange[2];	//range of valid inter-click intervals
    float minIciFraction; //this many of ICIs must fall in iciRange
    float avgTimeS;	//averaging time after calculating ratio
    
    /* stuff for findEncounters: */
    float blockLenS;	//length of a 'block', s
    float clicksPerBlock; //min clicks per block to count as a 'hit'
    float consecBlocks; //min consecutive blocks needed for an 'encounter'
    float hitsPerEnc;	//min number of block hits in consecutive
      			//blocks needed to count as an encounter
    int32 clicksToSave;	//number of clicks per encounter to save
    
    /* stuff for glider noise removal: */
    float ns_tBlockS;	//block duration for measuring noise, s
    float ns_tConsecS;	//consecutive time needed to decide 'noise', s
    float ns_thresh;	//noise threshold
    float ns_padSec;  	//pad noise periods w/this for ramp up/down
    float ns_minQuietS;	//minimum length of a noise section
} ERMAPARAMS;


ERMACONFIG *newERMACONFIG(void);
void deleteERMACONFIG(ERMACONFIG *ec);
void printERMACONFIG(ERMACONFIG *ec);
ERMACONFIG *ermaReadConfigFile(char *dir, char *filename);
void gatherErmaParams(ERMACONFIG *ec, ERMAPARAMS *ep);
char *ermaFindVar(ERMACONFIG *ec, char *varname);

/* These are for scanning ERMACONFIGs for a given varname and interpreting its
 * corresponding value as a int32/float/char/whatever. They return the number of
 * items successfully scanned, so for most of them it's 1 on success, 0 on
 * failure.
 */
int ermaGetInt32 (ERMACONFIG *ec, char *varname, int32 *val);
int ermaGetFloat (ERMACONFIG *ec, char *varname, float *val);
int ermaGetDouble(ERMACONFIG *ec, char *varname, double *val);
int ermaGetString(ERMACONFIG *ec, char *varname, char **val);
int ermaGetFloatArray(ERMACONFIG *ec, char *varname, float *val, int32 arrLen);


#endif    /* _ERMACONFIG_H_ */
