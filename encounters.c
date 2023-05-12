
#include "erma.h"

/* Defined below */
static void addEncounter(int32 encStart, int32 encEnd,
			 int32 minBlock, float blocksPerDay, ENCOUNTERS *enc);


void initENCOUNTERS(ENCOUNTERS *enc)
{
    enc->tSpanD = NULL;
    enc->tSpanDSize = 0;
    enc->n = 0;
}


/* Find the whale 'encounters' from a set of detected clicks. For example, we
 * can say an encounter is when there are 15 or more clicks in a minute, and
 * this happens for at least 5 of the minutes in a 10-minute span.
 *
 * In more detail: To find an encounter, first look at each time period (each
 * block) of length 'blockLenS' (e.g., 1 minute) and see if there are at least
 * 'clicksPerBlock' clicks (e.g., 15) in that block. If so, that block is
 * considered a 'hit'. When there is a set of 'consecBlocks' consecutive blocks
 * (e.g., 10) with at least 'hitsPerEnc' (e.g., 5) of them being hits, then
 * we've found an encounter.
 */

void findEncounters(ALLCLICKS *allC, ERMAPARAMS *ep, ENCOUNTERS *enc)
{
    int32 secPerDay = 24*60*60;
    float blocksPerDay = (float)secPerDay / ep->blockLenS;
    float blockLenD = ep->blockLenS / secPerDay;	//block length in days

    if (allC->n == 0)					//special case
	return;
    
    /* Find first and last times of clicks */
    double minTimeD = DBL_MAX, maxTimeD = -DBL_MAX;
    for (int32 i = 0; i < allC->n; i++) {
	if (allC->timeD[i] < minTimeD) minTimeD = allC->timeD[i];
	if (allC->timeD[i] > maxTimeD) maxTimeD = allC->timeD[i];
    }
    int32 minBlock = floor(minTimeD * blocksPerDay);
    int32 maxBlock =  ceil(maxTimeD * blocksPerDay);
    int32 nBlocks = maxBlock - minBlock;

    static int32 *nHits = NULL, *isHit = NULL;
    static size_t nHitsSize = 0, isHitSize = 0;
    BUFGROW(isHit, nBlocks, ERMA_NO_MEMORY_NHITS);
    BUFGROW(nHits, nBlocks, ERMA_NO_MEMORY_NHITS);

    for (int32 i = 0; i < nBlocks; i++) {
	/* Find start and end of block i, in days */
	double blockT0_D = (minBlock+i) / blocksPerDay;//block start time, days
	double blockT1_D = blockT0_D + blockLenD;      //block  end  time, days
	/* Count how many hits are in block i */
	int32 nn = 0;
	for (int32 j = 0; j < allC->n; j++)
	    nn += (allC->timeD[j] >= blockT0_D && allC->timeD[j] < blockT1_D);
	nHits[i] = nn;
	isHit[i] = (nn >= ep->clicksPerBlock);
    }

    static int32 *count = NULL;
    static size_t countSize = 0;
    BUFGROW(count, nBlocks, ERMA_NO_MEMORY_ENCCOUNT);

    for (int32 i = 0; i < nBlocks; i++)
	count[i] = 0;
    
    int32 nInConsec = 0;	//# of hits in the span (p0,p1]  (!)
    int inEnc = 0;		//currently in an encounter?
    int32 encStartBlk;		//start block # of encounter
    for (int32 p0 = -ep->consecBlocks, p1 = 0; p0 < nBlocks+1; p0++, p1++) {
	nInConsec -= (p0 >= 0      ? isHit[p0] : 0);
	nInConsec += (p1 < nBlocks ? isHit[p1] : 0);
	if (p1 < nBlocks)
	    count[p1] = nInConsec;
	
	/* If we're in an encounter, continue in it as long as count stays high
	 * enough; as soon as count drops below, find the last time that count
	 * was high enough and end the encounter there, and record it.
	 */
	int encHere = (nInConsec >= ep->hitsPerEnc);
	if (inEnc && encHere)	 {
	    /* Encounter continues; don't need to do anything. */
	} else if (inEnc && ~encHere) {
	    /* Finish encounter. It ends at last hit in the span [p0,p1). */
	    int32 encEndBlk = MAX(0, p0);	//set it in case j loop finishes
	    for (int32 j = MIN(nBlocks,p1) - 1; j >= MAX(0, p0); j--) {
		if (isHit[j]) {
		    encEndBlk = j;
		    break;
		}
	    }
	    addEncounter(encStartBlk, encEndBlk, minBlock, blocksPerDay, enc);
/*	    BUFGROW(enc->tSpanD, enc->n + 1, ERMA_NO_MEMORY_ENCOUNTERS);*/
/*	    enc->tSpanD[enc->n][0] = (encStartBlk + minBlock) / blocksPerDay;*/
/*	    enc->tSpanD[enc->n][1] = (encEndBlk   + minBlock) / blocksPerDay;*/

	} else if (~inEnc && encHere) {
	    /* Start an encounter at the first hit in the current p0:p1 span. */
	    for (int32 j = MAX(0,p0); j < MIN(nBlocks,p1+1); j++) {
		if (isHit[j]) {
		    encStartBlk = j;
		    break;
		}
	    }
	} else if (~inEnc && ~encHere) {
	    /* Non-encounter continues; don't need to do anything. */
	}
	
	inEnc = encHere;
    }	//for (p0,p1...)
    if (inEnc) {
	//The set of files ended in the middle of an encounter. Add it.
	addEncounter(encStartBlk, nBlocks-1, minBlock, blocksPerDay, enc);
    }
    printf("findEncounters: found %d encounters so far in this run\n", enc->n);
}


static void addEncounter(int32 encStartBlk, int32 encEndBlk,
			 int32 minBlock, float blocksPerDay, ENCOUNTERS *enc)
{
    BUFGROW(enc->tSpanD, enc->n + 1, ERMA_NO_MEMORY_ENCOUNTERS);
    enc->tSpanD[enc->n].t0 = (encStartBlk + minBlock) / (double)blocksPerDay;
    enc->tSpanD[enc->n].t1 = (encEndBlk   + minBlock) / (double)blocksPerDay;
    enc->n++;
}


/*
blocksPerDay = secPerDay / p.blockLenS;
blockLenDay = p.blockLenS / secPerDay;

minBlock = floor(min([clk.t0]) * blocksPerDay);
maxBlock =  ceil(max([clk.t1]) * blocksPerDay);
nBlocks = maxBlock - minBlock;

nHits = zeros(1, nBlocks);
for i = 1 : nBlocks
  blockT0 = (minBlock + i-1) / blocksPerDay;	% block start time, datenum fmt
  blockT1 = blockT0 + blockLenDay;		% block end time
  nHits(i) = sum([clk.clickDN] > blockT0 & [clk.clickDN] < blockT1);
end
isHit = (nHits > p.clicksPerBlock);

isEnc = false(1, nBlocks);
count = zeros(1, nBlocks);
nInConsec = 0;
p0 = 1 - p.consecBlocks;
p1 = 1;
enc = false;	% called isEnc in C
while (p1 < nBlocks + p.consecBlocks)
  if (p1 <= nBlocks), nInConsec = nInConsec + isHit(p1); end
  if (p0 >= 1),       nInConsec = nInConsec - isHit(p0); end
  if (p1 <= nBlocks), count(p1) = nInConsec; end

  % Set isEnc. If we're in an encounter, continue in it as long as count
  % stays high enough; as soon as count drops below, find the last time that
  % count was high enough and end the encounter there.
  encHere = (nInConsec >= p.hitsPerEnc);
  if (enc && encHere)	
    % Encounter continues; don't need to do anything.
  elseif (enc && ~encHere)
    % Finish the encounter. It ends at the last hit in the current p0:p1 span.
    encEnd = max(1, p0-1);				% set it just in case
    for j = min(nBlocks,p1) : -1 : max(1,p0)
      if (isHit(j)), encEnd = j; break; end
    end
    isEnc(encStart : encEnd) = true;
  elseif (~enc && encHere)				% start an encounter
    % Start an encounter. It starts at the first hit in the current p0:p1 span.
    for j = max(1,p0) : min(nBlocks,p1)
      if (isHit(j)), encStart = j; break; end
    end
  elseif (~enc && ~encHere)				% no old/new encounter
    % Non-encounter continues; don't need to do anything.
  end

  enc = encHere;
  p0 = p0 + 1;
  p1 = p1 + 1;
end
*/


/* Save some detections from each encounter in goodDetsPath. The number of
 * detections is proportional to the duration of the encounter, relative to the
 * total duration of all encounters.  Also append the name of the output file to
 * encFileListPath (wispr_dtx_list.txt).
 *
 * Need to also save an average spectrum.
 */
void saveEncounters(ENCOUNTERS *enc, ALLCLICKS *allC, char *piEncDetsPath,
		    char *wisprEncDetsPath, double tMinE, double tMaxE,
		    char *encFileListPath, int32 clicksToSave,
		    time_t startTime)
{
    FILE *fp = fopen(piEncDetsPath, "a");
    const double secPerDay = 24 * 60 * 60;

    printf("saveEncounters: %d encounter(s)\n", enc->n);

    if (fp != NULL) {
	/* Add up the duration of all encounters. */
	double totalDur = 0.0;
	for (int32 i = 0; i < enc->n; i++)
	    totalDur += TIMESPAN_DUR(enc->tSpanD[i]);

	/* Record the start and stop of processing */
	char buf0[20], buf1[20];
	fprintf(fp, "$analyzed,%s,%s\n",
		(tMaxE < 0) ? "0" : timeStrE(buf0, (time_t)floor(tMinE)),
		(tMaxE < 0) ? "0" : timeStrE(buf1, (time_t) ceil(tMaxE)));

	/* Write out [some of] the clicks in this encounter. */
	for (int32 i = 0; i < enc->n; i++) {
	    //Figure out how many clicks to save from this encounter.
	    double proportion = TIMESPAN_DUR(enc->tSpanD[i]) / totalDur;
	    int32 nClicks = (int32)round(proportion * clicksToSave);

	    //Print start- and stop-times of the encounter.
	    double t0D = enc->tSpanD[i].t0, t1D = enc->tSpanD[i].t1;
	    fprintf(fp, "$enc,%s,%s", timeStrD(buf0, t0D), timeStrD(buf1, t1D));

	    //Find indices of first (i0) & last (i1) clicks in this encounter.
	    int32 i0 = -1, i1 = -1;
	    for (int32 j = 0; j < allC->n; j++) {
		if (t0D <= allC->timeD[j] && allC->timeD[j] <= t1D) {
		    if (i0 == -1) i0 = j;
		    i1 = j;
		}
	    }
	    fprintf(fp, ",%d", i1 - i0);	//write number of clicks

	    //Save the clicks in the MIDDLE of the encounter, since those might
	    //be the loudest and thus most representative of the encounter.
	    int32 cMid = (i0 + i1)/2;
	    int32 c0 = MAX(i0, cMid - nClicks/2);
	    int32 c1 = MIN(i1, cMid + (nClicks+1)/2);
	    for (int32 ci = c0; ci < c1; ci++)
		fprintf(fp, ",%.3lf", (allC->timeD[ci] - t0D) * secPerDay);
	    fprintf(fp, "\n");
	}   //for i
	fprintf(fp, "$processtimesec,%ld\n", time(NULL) - startTime);
	fclose(fp);
    } //if (fp != NULL)

    /* Write name of that output file to encFileListPath. Use the WISPR version
     * of this name. */
    fp = fopen(encFileListPath, "a");
    if (fp != NULL) {
	fprintf(fp, "%s\n", pathFile(wisprEncDetsPath));
	fclose(fp);
    }
}
