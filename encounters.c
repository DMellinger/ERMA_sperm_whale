
#include "erma.h"

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
	double blockT0_D = (minBlock + i) / blocksPerDay;// block start time, days
	double blockT1_D = blockT0_D + blockLenD;	  // block  end  time, days
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
    int32 encStart;		//start block # of encountver
    for (int32 p0 = -ep->consecBlocks, p1 = 0; p0 < nBlocks; p0++, p1++) {
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
	    int32 encEnd = MAX(0, p0);	//set it just in case j loop finishes
	    for (int32 j = MIN(nBlocks,p1) - 1; j >= MAX(0, p0); j--) {
		if (isHit[j]) {
		    encEnd = j;
		    break;
		}
	    }
	    BUFGROW(enc->tSpanD, enc->n + 1, ERMA_NO_MEMORY_ENCOUNTERS);
	    enc->tSpanD[enc->n][0] = (encStart + minBlock) / blocksPerDay;
	    enc->tSpanD[enc->n][1] = (encEnd   + minBlock) / blocksPerDay;

	} else if (~inEnc && encHere) {
	    /* Start an encounter at the first hit in the current p0:p1 span. */
	    for (int32 j = MAX(0,p0); j < MIN(nBlocks,p1+1); j++) {
		if (isHit[j]) {
		    encStart = j;
		    break;
		}
	    }
	} else if (~inEnc && ~encHere) {
	    /* Non-encounter continues; don't need to do anything. */
	}
	
	inEnc = encHere;
    }	/* for (p0,p1...) */
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


/* Save some detections -- namely, the first ep->nSaveDets (default 50) -- from
 * each encounter in goodDetsPath. Also append the name of the output file to
 * encFileListPath (wispr_dtx_list.txt).
 *
 * Need to also save an average spectrum.
 */
void saveEncounters(ENCOUNTERS *enc, ALLCLICKS *allC, char *encDetsPath,
		    double tMinE, double tMaxE, char *encFileListPath)
{
    FILE *fp = fopen(encDetsPath, "a");
    const double secPerDay = 24 * 60 * 60;

    if (fp != NULL) {
	/* Record the start and stop of processing */
	time_t t0 = (time_t)floor(tMinE), t1 = (time_t)floor(tMaxE);
	fprintf(fp, "$startErma,%.3lf,%s", tMinE, asctime(gmtime(&t0)));
	fprintf(fp,  "$stopErma,%.3lf,%s", tMaxE, asctime(gmtime(&t1)));

	/* Write out [some of] the clicks in this encounter. */
	for (int32 i = 0; i < enc->n; i++) {
	    double t0 = enc->tSpanD[i][0], t1 = enc->tSpanD[i][1];
	    fprintf(fp, "$enc,%.3lf,%.3lf", t0 * secPerDay, t1 * secPerDay);
	    int32 i0 = -1, i1 = -1;
	    for (int32 j = 0; j < allC->n; j++) {
		if (i0 == -1 && t0 < allC->timeD[j])
		    i0 = j;
		if (t1 < allC->timeD[j])
		    i1 = j;
	    }
	    for (int32 j = i0; j <= MIN(i1, i0+50); j++)
		fprintf(fp, ",%.3lf", (allC->timeD[j] - t0) * secPerDay);
	    fprintf(fp, "\n");
	}
	fclose(fp);
    }

    /* Write name of that output file to encFileListPath */
    fp = fopen(encFileListPath, "a");
    if (fp != NULL) {
	fprintf(fp, "%s\n", pathFile(encDetsPath));
	fclose(fp);
    }
}
