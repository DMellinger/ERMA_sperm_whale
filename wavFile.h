#ifndef _WAVEFILE_H_
#define _WAVEFILE_H_

/*#include "wisprFile.h"		*//* for WISPRINFO */

WISPRINFO *wavReadHeader(WISPRINFO *wi, char *filename);
int wavReadData(int16_t *buf, WISPRINFO *wi, size_t offset, size_t nSamp);
void wavCloseFile(WISPRINFO *wi);
double getTimeFromName(char *fn);


#endif	/* _WAVEFILE_H_ */
