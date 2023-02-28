#ifndef _ENCOUNTERS_H_
#define _ENCOUNTERS_H_



typedef struct {
    double (*tSpanD)[2];// start- and stop-times of encounters, in days
    long n;	    	// number of valid elements in t
    size_t tSpanDSize;	// for bufgrow()
} ENCOUNTERS;

void initENCOUNTERS(ENCOUNTERS *enc);
void findEncounters(ALLCLICKS *allClicks, ERMAPARAMS *ep, ENCOUNTERS *enc);
void saveEncounters(ENCOUNTERS *enc, ALLCLICKS *allC, char *encDetsPath,
		    double tMinE, double tMaxE, char *encFileListPath);


#endif /* _ENCOUNTERS_H_ */
