
#include "erma.h"

/* Read a configuration file for ERMA and store it in an ERMACONFIG structure.
 * ERMA config files have a succession of lines of the form
 *
 *		varname = value
 *
 * The values are stored as strings in the ERMACONFIG that is returned. If for
 * some reason the config file can't be opened, the returned ERMACONFIG exists
 * but has nothing in it.
 */
ERMACONFIG *ermaReadConfigFile(char *dir, char *filename)
{
    FILE *fp;
    ERMACONFIG *ec;
    char *ln, *newVar, *newVal;
    size_t szLn, nbyte, newVarSize, newValSize;
    int64_t last;
    int32 nRead;
    char pathname[256];

    /* Create and initialize a new ERMACONFIG */
    if ((ec = newERMACONFIG()) == NULL)
	exit(ERMA_NO_MEMORY_CREATECONFIG);

    snprintf(pathname, sizeof(pathname), "%s/%s", dir, filename);
    fp = fopen(pathname, "r");
    if (fp == NULL)
	return ec;
    
    /* Read successive lines of the form "varname = value" and store them in
     * ec. */
    ln = newVar = newVal = NULL;
    szLn = newVarSize = newValSize = 0;
    while ((nRead = getline(&ln, &szLn, fp)) > 0) {
	/* Strip trailing CR/LF and whitespace */
	last = nRead - 1;
	while (last >= 0 && strchr("\012\015 \t", ln[last]))
	    ln[last--] = '\0';

	/* Skip over comment lines */
	if (last >= 0 && ln[0] == '%')
	    continue;

	/* Ensure buffers for holding new varname and value are big enough */
	BUFGROW(newVar, strlen(ln)+1, ERMA_NO_MEMORY_CONFIGVARBUF);
	BUFGROW(newVal, strlen(ln)+1, ERMA_NO_MEMORY_CONFIGVARBUF);
	if (sscanf(ln, "%s = %[^\012]", newVar, newVal) == 2 &&
	    strlen(newVar) > 0 && strlen(newVal) > 0) {
	    nbyte = (ec->n + 1) * sizeof(char *);
	    BUFGROW(ec->varname, nbyte, ERMA_NO_MEMORY_CONFIGVAR);
	    BUFGROW(ec->value,   nbyte, ERMA_NO_MEMORY_CONFIGVAR);
	    ec->varname[ec->n] = strsave(newVar);
	    ec->value  [ec->n] = strsave(newVal);
	    if (ec->varname[ec->n] == NULL || ec->value[ec->n] == NULL)
		continue;		/* out of memory? Just skip this line */
	    ec->n++;
	}
    }
    return ec;
}


/* Create and initialize a new ERMACONFIG.
 */
ERMACONFIG *newERMACONFIG(void)
{
    ERMACONFIG *ec;

    ec = (ERMACONFIG *)malloc(sizeof(ERMACONFIG));
    if (ec == NULL)
	exit(ERMA_NO_MEMORY_ERMACONFIG);
    ec->n = ec->varnameSize = ec->valueSize = 0;
    ec->varname = ec->value = NULL;

    return ec;
}


/* Delete an ERMACONFIG and everything malloc'ed in it.
 */
void deleteERMACONFIG(ERMACONFIG *ec)
{
    for (int32 i = 0; i < ec->n; i++) {
	free(ec->varname[i]);
	free(ec->value[i]);
    }
    if (ec->varname != NULL) free(ec->varname);
    if (ec->value   != NULL) free(ec->value);
    free(ec);
}


void printERMACONFIG(ERMACONFIG *ec)
{
    printf("ERMACONFIG:\n");
    for (int32 i = 0; i < ec->n; i++)
	printf("\t'%s' is '%s'\n", ec->varname[i], ec->value[i]);
}


/* This makes space for n filter coefficients in the A array and n in the B
 * array.
 */
static void allocFilterCoeffs(int32 n, float **pA, float **pB)
{
    if (n > 0) {
	*pA = calloc(n, sizeof((*pA)[0]));
	*pB = calloc(n, sizeof((*pB)[0]));
	if (*pA == NULL || *pB == NULL)
	    exit(ERMA_NO_MEMORY_FILTERCOEFFS);
    }
}


/* Convert each string in an ERMACONFIG into its corresponding value as a
 * float/int32/whatever in an ERMAPARAMS. Any members of the ERMAPARAMS that
 * didn't have anything specified for them in the ERMACONFIG (i.e., in the
 * rpi.cnf config file) will be the default value, which is set in
 * ErmaMain.c.
 *
 * The order of things here mostly doesn't matter; it doesn't have to match the
 * order of members of ERMAPARAMS. The exception is that array lengths [e.g.,
 * for filters] must be specified before the array values [e.g., filter coeffs].
 */
void gatherErmaParams(ERMACONFIG *ec, ERMAPARAMS *ep)
{
    /* File names: */
    ermaGetString(ec, "filesProcessed",	&ep->filesProcessed);
    ermaGetString(ec, "infilePattern", 	&ep->infilePattern);
    ermaGetString(ec, "outDir",		&ep->outDir);
    ermaGetString(ec, "allDetsFiles",	&ep->allDetsFiles);
    ermaGetString(ec, "encFileList",	&ep->encFileList);
    ermaGetString(ec, "wisprEncFileDir",&ep->wisprEncFileDir);
    ermaGetString(ec, "allDetsPrefix",	&ep->allDetsPrefix);
    ermaGetString(ec, "encDetsPrefix",	&ep->encDetsPrefix);
    ermaGetString(ec, "pctFileName",	&ep->pctFileName);

    /* GPIO pins */
    ermaGetInt32(ec, "gpioWisprActive", &ep->gpioWisprActive);
    ermaGetInt32(ec, "gpioRPiActive",   &ep->gpioRPiActive);

    /* Stuff for filtering: */
    /* These 'N' params must be read before the corresponding A and B ones so
     * allocFilterCoeffs can be called to allocate space for the A/B arrays */
    ermaGetInt32     (ec, "dsfN",   &ep->dsfN);	/* before dsfA/B! */
    ermaGetInt32     (ec, "numerN", &ep->numerN);	/* before numerA/B! */
    ermaGetInt32     (ec, "denomN", &ep->denomN);	/* before denomA/B! */
    allocFilterCoeffs(ep->dsfN,     &ep->dsfA,   &ep->dsfB);
    allocFilterCoeffs(ep->numerN,   &ep->numerA, &ep->numerB);
    allocFilterCoeffs(ep->denomN,   &ep->denomA, &ep->denomB);
    ermaGetFloatArray(ec, "dsfA",   ep->dsfA,   ep->dsfN);
    ermaGetFloatArray(ec, "dsfB",   ep->dsfB,   ep->dsfN);
    ermaGetFloatArray(ec, "numerA", ep->numerA, ep->numerN);
    ermaGetFloatArray(ec, "numerB", ep->numerB, ep->numerN);
    ermaGetFloatArray(ec, "denomA", ep->denomA, ep->denomN);
    ermaGetFloatArray(ec, "denomB", ep->denomB, ep->denomN);
    ermaGetInt32     (ec, "decim",  &ep->decim);

    /* stuff for ERMA algorithm: */
    ermaGetFloat(ec, "decayTime",	&ep->decayTime);
    ermaGetFloat(ec, "powerThresh",	&ep->powerThresh);
    ermaGetFloat(ec, "refractoryT",	&ep->refractoryT);
    ermaGetFloat(ec, "peakNbdT",	&ep->peakNbdT);
    ermaGetFloat(ec, "peakDurLims",	&ep->peakDurLims);
    ermaGetFloat(ec, "avgT",		&ep->avgT);
    ermaGetFloat(ec, "ratioThresh",	&ep->ratioThresh);
    ermaGetFloat(ec, "ignoreThresh",	&ep->ignoreThresh);
    ermaGetFloat(ec, "ignoreLimT",	&ep->ignoreLimT);
    ermaGetFloat(ec, "specLenS",	&ep->specLenS);

    /* stuff for testClickDets: */
    ermaGetFloat(ec, "minRate",		&ep->minRate);
    ermaGetFloatArray(ec, "iciRange",	ep->iciRange, NUM_OF(ep->iciRange));
    ermaGetFloat(ec, "minIciFraction",	&ep->minIciFraction);
    ermaGetFloat(ec, "avgTimeS",	&ep->avgTimeS);
    
    /* stuff for findEncounters: */
    ermaGetFloat(ec, "blockLenS",	&ep->blockLenS);
    ermaGetFloat(ec, "clicksPerBlock",	&ep->clicksPerBlock);
    ermaGetFloat(ec, "consecBlocks",	&ep->consecBlocks);
    ermaGetFloat(ec, "hitsPerEnc",	&ep->hitsPerEnc);
    ermaGetInt32(ec, "clicksToSave",	&ep->clicksToSave);
    
    /* stuff for glider noise removal: */
    ermaGetFloat(ec, "ns_tBlockS",	&ep->ns_tBlockS);
    ermaGetFloat(ec, "ns_tConsecS",	&ep->ns_tConsecS);
    ermaGetFloat(ec, "ns_pctile",	&ep->ns_pctile);
    ermaGetInt32(ec, "ns_nRecent",	&ep->ns_nRecent);
    ermaGetFloat(ec, "ns_medianMult",	&ep->ns_medianMult);
    /*ermaGetFloat(ec, "ns_thresh",	&ep->ns_thresh);*/ //obsolete
    ermaGetFloat(ec, "ns_padSec",	&ep->ns_padSec);
}


/* Find a given varname in ec and return its corresponding value (a string).
 */
char *ermaFindVar(ERMACONFIG *ec, char *varname)
{
    for (int32 i = 0; i < ec->n; i++)
	if (!strcmp(varname, ec->varname[i]))
	    return ec->value[i];
    
    return NULL;
}


/* Find varname in the ERMACONFIG ec and put its value into *val as an int32.
 * If varname isn't in ec, *val isn't changed. Returns 1 on success, 0 on
 * failure (not being able to find varname or not being able to scan the value
 * as the right type).
 */
int ermaGetInt32(ERMACONFIG *ec, char *varname, int32 *val)
{
    char *foundValueStr = ermaFindVar(ec, varname);

    if (foundValueStr == NULL)
	return 0;

    return (sscanf(foundValueStr, "%ld", val) == 1);
}


/* Find varname in the ERMACONFIG ec and put its value into *val as a float.  If
 * varname isn't in ec, *val isn't changed. Returns 1 on success, 0 on failure
 * (not being able to find varname or not being able to scan the value as the
 * right type).
 */
int ermaGetFloat(ERMACONFIG *ec, char *varname, float *val)
{
    char *foundValueStr = ermaFindVar(ec, varname);

    if (foundValueStr == NULL)
	return 0;

    return (sscanf(foundValueStr, "%f", val) == 1);
}


/* Find varname in the ERMACONFIG ec and put its value into *val as a double.
 * If varname isn't in ec, *val isn't changed. Returns 1 on success, 0 on
 * failure (not being able to find varname or not being able to scan the value
 * as the right type).
 */
int ermaGetDouble(ERMACONFIG *ec, char *varname, double *val)
{
    char *foundValueStr = ermaFindVar(ec, varname);

    if (foundValueStr == NULL)
	return 0;

    return (sscanf(foundValueStr, "%lf", val) == 1);
}


/* Find varname in the ERMACONFIG ec and put its value into *val as a
 * string. The string is a stored elsewhere -- it's not a copy -- and should NOT
 * be modified or freed by the caller. If varname isn't in ec, *val isn't
 * changed. Returns 1 on success, 0 on failure (not being able to find varname).
 */
int ermaGetString(ERMACONFIG *ec, char *varname, char **val)
{
    char *foundValueStr = ermaFindVar(ec, varname);

    if (foundValueStr == NULL)
	return 0;

    *val = foundValueStr;
    return 1;
}


/* Find varname in the ERMACONFIG ec and put its (multiple, comma-separated)
 * values into *val as a succession of floats. arrLen specifies the maximum
 * length of *val, which you should have allocated before calling this. On
 * success, returns the number of array items scanned, 0 on failure (not being
 * able to find varname). Note that this makes it impossible to differentiate
 * not finding varname, and finding varname but scanning 0 items from it.
 */
int ermaGetFloatArray(ERMACONFIG *ec, char *varname, float *val, int32 arrLen)
{
    char *foundValueStr = ermaFindVar(ec, varname);
    int nScan;		//must be int for sscanf %n

    if (foundValueStr == NULL)
	return 0;

    int32 pos = 0;
    for (int32 i = 0; i < arrLen; i++) {
	/* scan the next float in the sequence */
	if (sscanf(&foundValueStr[pos], "%f%n", &val[i], &nScan) == 0)
	    return i;
	pos += nScan;
	/* swallow the ',' if any */
	sscanf(&foundValueStr[pos], "%n,%n", &nScan, &nScan);
	pos += nScan;
    }
    return arrLen;
}
