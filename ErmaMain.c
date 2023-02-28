
#include "erma.h"

/* This is a system for detecting the echolocation clicks of odontocetes
 * (toothed whales, dolphins, etc.) in a set of WISPR recordings.
 *
 * It works by looking for any available unprocessed input files that match a
 * template name, processing them to detect the target species, and writing a
 * detection report (a file). It keeps track of which files have been processed
 * in a bookkeeping file so that a given file gets processed only once.
 * Parameters controlling the processing may be specified in a configuration
 * file, although there are defaults so the code will still work if the config
 * file is missing.
 *
 * The file and directory names for these tasks are set up in the Configuration
 * section just below.
 */


/* This flag says whether we're compiling this on a Raspberry Pi system.
 * (Initially the code is being developed in Windows.)
 */
#define ON_RPI	__arm__	/* say "#if ON_RPI" to use (NOT #ifdef!!) */


/**************************************************************************/
/****************************** Configuration *****************************/
/**************************************************************************/
/* baseDir is a directory containing the various files and directories below. It
 * should end with a '/' character.
 *
 * NB: This parameter can't be changed during a run, as it's part of the file
 * name (namely <baseDir>/<configFileName>) that's read to change parameters.
 */
#if ON_RPI
/*    char *baseDir = "/mnt/sd0";*/
    char *baseDir = "/media/pi/wispr_sd0";
#else
/*    char *baseDir = "C:/Dave/sounds/PacWavePerimeterData"; *//* for testing */
/*      char *baseDir = "C:/Dave/sounds/HI_Apr22_gliders/sperm";*/
char *baseDir = "C:/Dave/sounds/GoM_gliders_2017/resampledAt187kHz";
#endif


/* configFileName is an optional configuration file in the baseDir directory
 * specifying a bunch of parameters that control ERMA processing. It's okay for
 * it to be missing, as default values are used.
 *
 * NB: This parameter can't be changed during a run, as it's part of the file
 * name (namely <baseDir>/<configFileName>) that's read to change parameters.
 */
char *configFileName = "rpi.cnf"; 


/* These parameters are for detecting sperm whale clicks. They can be overridden
 * by settings in the <baseDir>/<configFileName> file (normally rpi.cnf). The
 * numeric parameters are from GoSperm.m.
 *
 * NB! If you change anything here, also change the definition of ERMAPARAMS in
 * ermaConfig.h (being careful to keep the order of entries parallel to here!)
 * AND change argument parsing in ermaConfig.c/gatherErmaParams.
 */
static ERMAPARAMS ep =
    {/**************************** file names *****************************/
     //filesProcessed: a file in the baseDir directory where Erma keeps track of
     //which soundfiles have been processed so far so that none of them are
     //processed more than once.
     "files_processed.txt",

     //infilePattern: a template for files in the baseDir directory used
     //* to find the WISPR soundfiles to process. It can have '*' wildcards in
     //the file part of the name, but not in the directory name.
     /*"PERI-1_longer_dataset/WISPR*.dat";*/ //WISPR files but no sperm whales
     /*"initialSet/sg680*.wav";*/	//sperm whales, but recorded by PMAR
     "*.wav",

     //outDir: where in the outDir directory to put the detection report files
     //that are the output of this ERMA process.
     "output",

     //allDetsFiles: a list of output files from the ERMA detector that has
     //times of all detected clicks. The detector generally creates one output
     //file per run, where a run includes processing of all unprocessed
     //soundfiles. So usually allDetsFiles has one entry (one line) per run.
     "det_reports_file.txt",

     //encFileList: whenever a detection file is created for upload to the
     //basestation, append its name to this file. WISPR removes the name when
     //the file is uploaded.
     "wispr_dtx_list.txt",  

     //allDetsPrefix: used to construct the file names for recording the times
     //of all detected clicks. This prefix gets the timestamp of the first
     //processed file appended so that successive runs use different names. In
     //contrast to encDetsPrefix just below, these files are in the <outDir>
     //directory (usually "output/").
     "all_dets",
     
     //encDetsPrefix: used to construct the file names for recording the times
     //of [some of the] detected clicks that are in encounters. This prefix gets
     //the timestamp of the first processed file appended so that successive
     //runs use different names. The full name is written to encFileList so that
     //the file gets uploaded to the basestation. In contrast to allDetsPrefix,
     //these files are in the <baseDir> directory.
     "encounter_dets",
     /************************** end of file names ***************************/

     /* GPIO pins */
     /* Two GPIO pins on the RPi card are used: One (ep.gpioWisprActive) is
      * set by WISPR; when high, it tells the RPi that the SD card is accessible
      * and it should start processing soundfiles. WISPR can later set it low to
      * tell RPi to finish up, so the RPi should check it periodically and shut
      * down if it's low.
      *
      * The other (gpioRPiActive) is set by the RPi and tells WISPR that
      * the RPi is busy processing; when it goes low, the RPi is shutting down
      * and WISPR should wait a few seconds and power off the RPi.
      */
     6,		//gpioWisprActive: input pin # to tell RPi to process files
     12,	//gpioRPiActive: output pin # to tell WISPR that RPi is busy

     /* Stuff for filtering. NB: These filter params differ from other params
      * below in that their default values aren't here but are in ermaFilt.c */
     NULL,NULL,0,//dsfA/B/N: IIR filter for downsampling
     3,		 //decim: decimation factor for downsampling
     NULL,NULL,0,//numerA/B/N: IIR filter for numerator in ERMA calc
     NULL,NULL,0,//denomA/B/N: IIR filter for denominator in ERMA calc

     /* stuff for ERMA algorithm: */
     0.25,	//decayTime: exponential-decay constant for expdecay()
     200,	//powerThresh: detection threshold per kHz in numerator band
     0.10,	//refractoryT: stop peaks w/in this after start of a click
     0.005,	//peakNbdT: peak is highest of anything within this time
     0.005,	//peakDurLims: peak can't last longer than this
     0.005,	//avgT: averaging time before computing ERMA ratio
     15,	//ratioThresh: minimum ERMA ratio (this is linear, not dB)
     50,	//ignoreThresh: sams this loud don't affect running mean
     0.1,	//ignoreLimT: ...unless they last for this long
     
     /* stuff for testClickDets: */
     40,	//minRate: min required click period over whole file, s
     {0.3, 1.5},//iciRange: range of valid inter-click intervals, s
     0.33,	//minIciFraction: this many of ICIs must fall in iciRange
     0.25,	//avgTimeS: averaging time after calculating ratio
     
     /* stuff for findEncounters: */
     60,	//blockLenS: length of a 'block', s
     10,	//clicksPerBlock: min clicks per block to count as a 'hit'
     10,	//consecBlocks: number of consecutive blocks to consider
     5,		/* hitsPerEnc: min number of block hits in the consecutive
		 * blocks needed to count as an encounter */
     
     /* stuff for glider noise removal: */
//    0.1,	*///ns_tBlockS: block duration for measuring noise, s
     0.01,	//ns_tBlockS: block duration for measuring noise, s
     0.3,	//ns_tConsecS: consecutive time needed to decide 'noise', s
/*     2e4,     *///ns_thresh: noise threshold 
     4e8,	//ns_thresh: noise threshold
     0.1,	//ns_padSec: pad noise periods w/this for ramp up/down
     0.1,	//ns_minQuietS: minimum length of a noise section
    };


/**************************************************************************/
/************************** End of configuration **************************/
/**************************************************************************/


/* Declarations for stuff defined down below */
char **readFilesProcessed(char *filename);
char **getNewFiles(char **filesProcessedList, char *dir, char *inputSoundDir);
void appendToProcessed(char *filepart, char *filesProcessed);

/* This holds all the clicks that have been found this run. */
static ALLCLICKS allC;		/* all clicks found in all files */
static ENCOUNTERS enc;

int main(int argc, char **argv)
{
    long nFiles, i;
    char **filesProcessedList;		//list of files done on prev runs
    char **unprocessedFiles;		//list of files to do on this run
    char allDetsPath[256];		//stores all click dets
    char encDetsPath[256];		//stores click dets in encounters
    char buf[256];			//temp buffer for making allDetsPath
    char encFileListPath[256];		//stores list of encounter output files
    char filesProcessedPath[256];	//full path for filesProcessed
    unsigned int pinval;

    #if ON_RPI
    /* Set RPI_ACTIVE GPIO pin high to indicate I'm busy */
    gpio_export(ep.gpioRPiActive);		//make the pin accessible
    gpio_set_direction(ep.gpioRPiActive, 1);	//say this pin is for output
    gpio_set_value(ep.gpioRPiActive, 1);

    /* Wait for WISPR_ACTIVE GPIO pin to go high */
    gpio_export(ep.gpioWisprActive);		 //make the pin accessible
    gpio_set_direction(ep.gpioWisprActive, 0);//say this pin is for input
    while (1) {
	if (!gpio_get_value(ep.gpioWisprActive, &pinval) && pinval)
	    break;
	sleep(1);				//wait 1 second
    }
    #endif

    /* Initialization */
    initENCOUNTERS(&enc);
    initALLCLICKS(&allC);
    ERMACONFIG *ec = ermaReadConfigFile(baseDir, configFileName);
    gatherErmaParams(ec, &ep);

    /* printERMACONFIG(ec);*/
    
    /* Get a list of files that have already been processed */
    snprintf(filesProcessedPath, sizeof(filesProcessedPath), "%s/%s",
	     baseDir, ep.filesProcessed);
    filesProcessedList = readFilesProcessed(filesProcessedPath);

    /* Make a list of unprocessed sound files. It's NULL if there's an error. */
    unprocessedFiles =
	getNewFiles(filesProcessedList, baseDir, ep.infilePattern);

    /* Process the unprocessed files. BEFORE processing each file, the filename
     * is saved to filesProcessed (files_processed.txt); that way, if some file
     * makes this program hang up or bomb, it is skipped on future runs.
     */
    if (unprocessedFiles != NULL && unprocessedFiles[0] != NULL) {
	/* Construct the name of the output file. The output file for this run
	 * (allDetsPath) is named with the part of the first input soundfile
	 * after the "WISPR_" prefix - this is normally the date/time stamp -
	 * and with a .csv extension.
	 */
	pathRoot(buf, pathFile(unprocessedFiles[0]));
	char *fileTimestamp = !strncmp(buf,"WISPR_",6) ? buf+6 : buf;
	snprintf(allDetsPath, sizeof(allDetsPath), "%s/%s/%s-%s.csv",
		 baseDir, ep.outDir, ep.allDetsPrefix, fileTimestamp);
	//encDetsPath is in baseDir, not baseDir/outDir.
	snprintf(encDetsPath, sizeof(encDetsPath), "%s/%s-%s.csv",
		 baseDir, ep.encDetsPrefix, fileTimestamp);
	snprintf(encFileListPath, sizeof(encDetsPath), "%s/%s",
		 baseDir, ep.encFileList);

	/* Process the files! */
	double tMinE = DBL_MAX, tMaxE = -DBL_MAX;
	for (i = 0; unprocessedFiles[i] != NULL; i++) {
/*	for (i = 0; unprocessedFiles[i] != NULL && i < 3; i++) {  *//* DEBUG */
/*	    printf("******* DEBUG: ErmaMain doing only a few **********\n");*/
	    appendToProcessed(pathFile(unprocessedFiles[i]),filesProcessedPath);
/*	    printf("******* DEBUG: not appending to files_processed.txt**\n");*/

	    processFile(unprocessedFiles[i], allDetsPath, &ep, &allC,
			&tMinE, &tMaxE);

	    /* Check to be sure that WISPR wants RPi to keep running */
	    #if ON_RPI
	    if (!gpio_get_value(ep.gpioWisprActive, &pinval) && pinval)
		break;
	    #endif
	}

	/* Find and save encounters. */
	findEncounters(&allC, &ep, &enc);
	saveEncounters(&enc, &allC, encDetsPath, tMinE, tMaxE, encFileListPath);
    }

    /* Set the RPI_ACTIVE GPIO pin to low to indicate we're done and the RPi can
     * be shut off.
     */
#if ON_RPI
    gpio_set_value(ep.gpioRPiActive, 0);	/* set the pin low */
/*    system("sudo shutdown now");*/		/* would this work? */
#endif

    return 0;
}


/* Read the file that has a list of the files that have been processed (or
 * attempted to be processed).
 */
char **readFilesProcessed(char *filename)
{
    char **filelist, **p;
    size_t filelistSize;
    long nFiles, lnSize;
    char *ln;
    FILE *fp;

    fp = fopen(filename, "r");	/* check for fp==NULL is below */
    filelist = NULL;
    filelistSize = 0;
    nFiles = 0;
    while (1) {
	BUFGROW(filelist, nFiles + 1, ERMA_NO_MEMORY_FILELIST);
	filelist[nFiles] = NULL;
	if (fp == NULL)		/* can only happen first time thru loop */
	    return filelist;	/* need filelist to be defined upon return */
	ln = NULL;
	lnSize = 0;
	if (getline(&ln, &lnSize, fp) < 0) {	/* reached end of file? */
	    fclose(fp);
	    return filelist;
	}
	
	if (lnSize > 0) {
	    /* Strip trailing \r or \n from ln, then stash it away */
	    if (ln[strlen(ln)-1] == '\r' || ln[strlen(ln)-1] == '\n')
		ln[strlen(ln)-1] = '\0';
	    filelist[nFiles++] = ln;
	}
    }
}


/* Get a list of unprocessed files - those that match the pattern (template) in
 * infilePattern and aren't yet in the file list filesProcessedList. Returns a
 * new NULL-terminated list of pathnames with malloc'ed strings having the names
 * that aren't yet processed, or NULL if there's something wrong with the
 * infilePattern.
 */
char **getNewFiles(char **filesProcessedList, char *dir,char *infilePattern)
{
    glob_t globbuf;
    long nFiles, i;
    char templt[256];

    snprintf(templt, sizeof(templt), "%s/%s", dir, infilePattern);
    /* Get a list of files that match the input pattern. */
    if (glob(templt, GLOB_NOESCAPE, NULL, &globbuf) != 0)
	return NULL;

    /* Walk through that list of files. For each one, see if it's in the
     * list of processed files, and if not, add it to the unprocessed list.
     */
    char **unprocessed = NULL;
    size_t unprocessedSize = 0;		//for bufgrow
    nFiles = 0;
    for (i = 0; 1; i++) {		//need to call bufgrow at least once
	BUFGROW(unprocessed, nFiles + 1, ERMA_NO_MEMORY_UNPROCESSED);
	if (i >= globbuf.gl_pathc)	//loop termination condition
	    break;

	/* Get next WISPR filename, but at the point after the last / or \ */
	if (findInList(pathFile(globbuf.gl_pathv[i]), filesProcessedList) < 0)
	    unprocessed[nFiles++] = strsave(globbuf.gl_pathv[i]);
    }
    unprocessed[nFiles] = NULL;

/*    printf("getNewFiles: %d new files to process\n", nFiles);*/
    
    return unprocessed;
}


/* Append a filepart (the part of a filename without the directory name(s)) to
 * the list of processed files in filesProcessedPath. The latter file is opened
 * and closed every time this is called to reduce the chance that it gets
 * mangled if the system crashes or powered down.
 */
void appendToProcessed(char *filepart, char *filesProcessedPath)
{
    FILE *fp;

    fp = fopen(filesProcessedPath, "a");
    if (fp != NULL) {
	fprintf(fp, "%s\n", filepart);
	fclose(fp);
    }
}
