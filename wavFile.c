
#include "erma.h"

/* This is defined in mmreg.h, but that file has a bunch of stuff that breaks
 * the compilation. So it's just defined here.
 */
#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif

/* Find a chunk in the WAVE file with a given name. Returns 1 on failure; on
 * success, returns 0 and leaves the chunk size in *pChunkSize and the file-read
 * pointer fp just after the chunk size.
 */
static int findChunk(FILE *fp, char *chunkName, size_t *pChunkSize)
{
    char buf[4];
    int32 chunkSize32;

    fseek(fp, 12, SEEK_SET);
    while (1) {
	if (fread(buf, 1, 4, fp) != 4 ||
	    !readLittleEndian32(&chunkSize32, 1, fp)) {
	    return 1;
	}
	*pChunkSize = (size_t)chunkSize32;
	size_t len = strlen(chunkName);
	if (!strncasecmp(buf, chunkName, len)) {
	    /* Watch out for chunk names with only 3 letters */
	    if (len == 4 || (len == 3 && (buf[3] == ' ' || buf[3] == '\0')))
		return 0;
	}
	fseek(fp, *pChunkSize, SEEK_CUR);
    }
}



/* Open a WAVE (.wav) file and read its header. Upon success, fills in the data
 * in wi leaves the file open, and returns wi (which has fp). Upon failure,
 * returns NULL with the file closed.
 */
WISPRINFO *wavReadHeader(WISPRINFO *wi, char *filename)
{
    unsigned char buf12[12];
    size_t chunkSize;
    uint32_t sRateLong;
    uint16_t sampleFmt, nChans, bitsPerSam;

    wi->fp = fopen(filename, "r");
    if (wi->fp == NULL)
	return NULL;

    /* Read initial header, check for RIFF/WAVE. */
    fread(buf12, 1, 12, wi->fp);
    if ((!strncasecmp(&buf12[0], "RIFF", 4)) &&
	(!strncasecmp(&buf12[8], "WAVE", 4)))  {
	
	/* Find 'fmt' chunk. */
	sRateLong = -1;
	if (!findChunk(wi->fp, "fmt", &chunkSize)      &&
	    readLittleEndian16(&sampleFmt,  1, wi->fp) &&
	    readLittleEndian16(&nChans,     1, wi->fp) &&
	    readLittleEndian32(&sRateLong,  1, wi->fp) &&
	    !fseek(wi->fp, 6, SEEK_CUR)                &&
	    readLittleEndian16(&bitsPerSam, 1, wi->fp) &&
	    (bitsPerSam == 16 || bitsPerSam == 24)     &&
	    sampleFmt == WAVE_FORMAT_PCM) {	/* see above */

	    /* Skip rest of fmt chunk. */
	    wi->sRate = (float)sRateLong;
	    if (!findChunk(wi->fp, "data", &chunkSize)) {
		wi->wisprVersion[0] = '\0';
		wi->sRate = (float)sRateLong;
		wi->sampleSize = bitsPerSam / 8;	/* bytes per sample */
		wi->nSamp = chunkSize / sizeof(short);
		wi->timeE = getTimeFromName(filename);
		wi->isWave = 1;
		return wi;
	    }
	}
    }

    /* Failure. */
    fclose(wi->fp);
    wi->fp = NULL;
    return NULL;
}


/* Given a WISPRINFO with an open file pointer wi->fp, read nSamp samples of
 * data (correcting for endianness if needed) and store it in buf. Returns 0 on
 * success, 1 on failure.
*/
int wavReadData(int16_t *buf, WISPRINFO *wi, size_t offset, size_t nSamp)
{
    size_t chunkSize;

    if (wi->fp != NULL                                    &&
	!findChunk(wi->fp, "data", &chunkSize)            &&
	chunkSize / wi->sampleSize >= (offset + nSamp)    &&
	!fseek(wi->fp, offset * wi->sampleSize, SEEK_CUR) &&
	readLittleEndian16(buf, nSamp, wi->fp)            ) {
	return 0;
    }
    return 1;
}


/* Close out a WAVE file.
 */
void wavCloseFile(WISPRINFO *wi)
{
    if (wi->fp != NULL)
	fclose(wi->fp);
    wi->fp = NULL;
}
    

/* Given a name like WISPR_230220-170345, extract the date value. THis is
 * seconds since The Epoch (1/1/1970), but unlike a time_t it can include
 * milliseconds.
 */
double getTimeFromName(char *fn)
{
    int32 len = strlen(fn);
    static char middleChar[] = "-_Tt";	/* separators between day and hour */
    char pat[100];
    struct tm tm;
    int msec = 0, pos;

    /* Walk thru fn looking for string matching YYMMDD-hhmmss[.msec] */
    for (int32 i = 0; i < len - 13; i++) {
	/* Walk thru possible characters between YYMMDD and hhmmss. */
	for (int32 j = 0; j < NUM_OF(middleChar); j++) {
	    sprintf(pat, "%%02d%%02d%%02d%c%%02d%%02d%%02d.%%d%n",
		    middleChar[j], &pos);
	    int32 nDigit = pos - 14;
	    if (sscanf(&fn[i], pat, &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
		       &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &msec) >= 6) {
		tm.tm_year += 100;	//make it based on 2000, not 1900
		tm.tm_mon -= 1;		//tm stores January as month 0
		tm.tm_isdst = 0;	//no daylight saving
		return (double)my_timegm(&tm) + (double)msec/pow(10.0,-nDigit);
	    }
	}
    }
    return -1;
}

#ifdef NEVER
  //Read initial file header.
  if (   (strncmp(&first12[0], "RIFF", 4) && strncmp(&first12[0], "riff", 4))
      || (strncmp(&first12[8], "WAVE", 4) && strncmp(&first12[8], "wave", 4)))
  {
    return STR_NOT_WAVE;
  }
     
  ::lseek(sf->fd, 12L, 0);

  //Walk through chunks, looking for 'fmt' (which must come before 'data').
  ULONG chunkLen;   
  char buf[12];            //use char, not TCHAR
  //The badBits code was an attempt to correct an AURAL flaw.  I ended up not
  //using it (by renaming the AURAL files to *.WVE and using sfBinary to read). 
  //bool badBits = false;    //an AURAL file with bits set wrong in header?
  bool ok = true;
  while (ok) {
    ok = (::read(sf->fd, buf, 4) == 4) && sf->swapReadLong(&chunkLen);
    if (   (!strncmp(buf, "fmt", 3) || !strncmp(buf, "FMT", 3))
        && (buf[3] == 0 || buf[3] == ' '))
    {                                        
      //badBits = (chunkLen & 0x2000000) != 0;
      //Found a fmt chunk.  Read the various parameters.
      USHORT nChansShort, bitsPerSam, fmt;
      ULONG sRateLong;
      ok = ok && sf->swapReadShort(&fmt)
              && sf->swapReadShort(&nChansShort)
              && sf->swapReadLong(&sRateLong);
      ::lseek(sf->fd, 6, SEEK_CUR); //skip bytesPerSec (4 bytes), blockAlign (2)
      ok = ok & sf->swapReadShort(&bitsPerSam);
           
      //Fix bad bits in some AURAL files.
      //if (badBits) {
        //chunkLen    &= ~0x2000200;    //normally 16
        //fmt         &= ~0x200;
        //nChansShort &= ~0x200;
        //sRateLong   &= ~0x2000000;
        //bitsPerSam  &= ~0x200;        //need to allow 64-bit samples?
      //}

      ::lseek(sf->fd, chunkLen - 16, SEEK_CUR);   //skip rest of chunk
      //Check and convert values.  See mmreg.h for WAVE_FORMAT_*.
      if (   fmt != WAVE_FORMAT_PCM
          && fmt != WAVE_FORMAT_MULAW
          && fmt != WAVE_FORMAT_IEEE_FLOAT)
      {
        return STR_BAD_FORMAT;
      }
      sf->nChans = nChansShort;
      _clear87();              //float WAVE files trigger a leftover error here
      sf->sRate = (double)sRateLong;
      sf->samSize = (bitsPerSam + 7)/8 * (fmt==WAVE_FORMAT_IEEE_FLOAT ? -1 : 1);
      sf->mulaw = (fmt == WAVE_FORMAT_MULAW);
      break;
    }
    ::lseek(sf->fd, chunkLen, SEEK_CUR);     //advance to next chunk
  }

  //Finished the fmt chunk (or hit EOF).
  if (ok) {
    //Find the data chunk.
    while (ok) {
      ok = (::read(sf->fd, buf, 4) == 4) && sf->swapReadLong(&chunkLen);
      //if (badBits) chunkLen &= ~0x2000200;
      //if (!strncmp(buf, "MTE1", 4)) chunkLen = 452; //may fail for other AURALs! 
      if (ok && (!strncmp(buf, "data", 4) || !strncmp(buf, "DATA", 4)))
        break;
      ::lseek(sf->fd, chunkLen, SEEK_CUR);     //advance to next chunk
    }
    //Use minimum of declared size (from chunkLen) and actual file size.
    sf->byteOffset = ::lseek(sf->fd, 0, SEEK_CUR);
    sf->nSamsPerChan = MIN(chunkLen, sf->fileSize - sf->byteOffset)
                                               / sf->nChans / ABS(sf->samSize);
  }
  sf->interleaved = true;
  sf->timeStamp = 0.0;
  sf->setStandardParams();
  return ok ? NULL : STR_UNEXPECTED_EOF;
}
#endif
