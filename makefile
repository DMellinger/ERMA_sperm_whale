
# This makes ErmaMain.exe, which looks for any available unprocessed
# input files in a target directory, processes them to detect sperm
# whales, and writes detection reports to an output directory.  It
# keeps track of which files have been processed in a bookkeeping
# file. See ErmaMain.c for the relevant file and directory names.


# This makefile can make either the "stub" version of processFile,
# which doesn't actually process data from each input file but merely
# writes the file's name to the output file, or the real version,
# which isn't done yet. To use the real version use processFile.o in
# the ErmaMain line below, and to use the stub version put
# processFileStub.o there.

# Choose one of these, the first for debugging, the second for speed:
#CFLAGS = -g
CFLAGS = -O2

LDLIBS = -lm

ErmaMain: ErmaMain.o processFile.o wisprFile.o wavFile.o ermaConfig.o \
	ermaGoodies.o gpio.o iirFilter.o ermaFilt.o quietTimes.o ermaNew.o \
	encounters.o expDecay.o

# This is a list of all the include files in this project. Everything
# depends on it, so whenever you touch one, everything recompiles.
ALLINCLUDES = encounters.h erma.h ermaConfig.h ermaErrors.h	\
		ermaFilt.h ermaGoodies.h ermaNew.h expDecay.h	\
		quietTimes.h gpio.h iirFilter.h processFile.h	\
		wavFile.h wisprFile.h

ErmaMain.o:	${ALLINCLUDES}
wisprFile.o:	${ALLINCLUDES}
wavFile.o:	${ALLINCLUDES}
ermaConfig.o:	${ALLINCLUDES}
ermaGoodies.o:	${ALLINCLUDES}
gpio.c:		${ALLINCLUDES}
processFile.o:  ${ALLINCLUDES}
processFileStub.o: ${ALLINCLUDES}
iirFilter.o:	${ALLINCLUDES}
#	cc -O3 -c iirFilter.c
ermaFilt.o:     ${ALLINCLUDES}
quietTimes.o:	${ALLINCLUDES}
encounters.o:	${ALLINCLUDES}
ermaNew.o:	${ALLINCLUDES}
expdecay.o:	${ALLINCLUDES}
