
#include <stdio.h>		/* for FILE */
/*#include <stdlib.h>*/
#include <time.h>		/* for time_t */
#include <sys/stat.h>
#include "ermaConfig.h"
#include "wisprFile.h"
#include "ermaGoodies.h"
#include "processFile.h"



void processFile(char *inPath, char *outPath, ERMACONFIG *rc)
{
    WISPRINFO wi;
    static int cnt = 0;
    FILE *fp;
    char dirPath[256];

    wisprInitWISPRINFO(&wi);
    wisprReadHeader(&wi, inPath);
    if (cnt++ < 2) {
	printf("Output file name: %s\n", outPath);
	wisprPrintInfo(&wi);
    }

    /* Ensure the directory for the output file exists, creating it if not */
    pathDir(dirPath, outPath);		/* get dir name sans file part */
    if (!dirExists(dirPath)) {
	if (mkdir(dirPath, 0777) != 0)
	    return;		/* can't create dir for output file! */
    }

    /* Save detections to outPath */
    if ((fp = fopen(outPath, "a")) >= 0) {
	fprintf(fp, "$inputfile,%s\n", pathFile(inPath));
	fclose(fp);
    }

    wisprCleanup(&wi);
}
