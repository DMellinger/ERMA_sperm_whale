#ifndef _ENCOUNTERS_H_
#define _ENCOUNTERS_H_


/* This holds an array of encounters, each one stored as start- and stop-times
 * in tSpanD.
 */
typedef struct {
    TIMESPAN *tSpanD;	//start- and stop-times of encounters, in days
    size_t tSpanDSize;	//for bufgrow()
    int32 n;	    	//number of valid elements in tSpanD
} ENCOUNTERS;

void initENCOUNTERS(ENCOUNTERS *enc);
void findEncounters(ALLCLICKS *allClicks, ERMAPARAMS *ep, ENCOUNTERS *enc);
void saveEncounters(ENCOUNTERS *enc, ALLCLICKS *allC, char *piEncDetsPath,
		    char *wisprEncDetsPath, double tMinE, double tMaxE,
		    char *encFileListPath, int32 clicksToSave,
		    time_t startTime);

#endif /* _ENCOUNTERS_H_ */
