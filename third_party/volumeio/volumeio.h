// (c) by Stefan Roettger, licensed under GPL 2+
// renamed ddsbase.cpp -> volumeio.cpp @Adrian Derstroff
// http://www9.informatik.uni-erlangen.de/External/vollib/

#ifndef VOLUMEIO_H
#define VOLUMEIO_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define BOOLINT char

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#ifndef NULL
#define NULL (0)
#endif

#define ERRORMSG() errormsg(__FILE__,__LINE__)
inline void errormsg(const char *file, int line)
{
	fprintf(stderr, "fatal error in <%s> at line %d!\n", file, line);
	exit(EXIT_FAILURE);
}

inline int intmin(const int a, const int b) { return((a < b) ? a : b); }
inline int intmax(const int a, const int b) { return((a > b) ? a : b); }


void writeDDSfile(const char *filename,unsigned char *data,unsigned int bytes,unsigned int skip=0,unsigned int strip=0,BOOLINT nofree=FALSE);
unsigned char *readDDSfile(const char *filename,unsigned int *bytes);

void writeRAWfile(const char *filename,unsigned char *data,unsigned int bytes,BOOLINT nofree=FALSE);
unsigned char *readRAWfile(const char *filename,unsigned int *bytes);

void writePNMimage(const char *filename,unsigned char *image,unsigned int width,unsigned int height,unsigned int components,BOOLINT dds=FALSE);
unsigned char *readPNMimage(const char *filename,unsigned int *width,unsigned int *height,unsigned int *components);

void writePVMvolume(const char *filename,unsigned char *volume,
                    unsigned int width,unsigned int height,unsigned int depth,unsigned int components=1,
                    float scalex=1.0f,float scaley=1.0f,float scalez=1.0f,
                    unsigned char *description=NULL,
                    unsigned char *courtesy=NULL,
                    unsigned char *parameter=NULL,
                    unsigned char *comment=NULL);

unsigned char *readPVMvolume(const char *filename,
                             unsigned int *width,unsigned int *height,unsigned int *depth,unsigned int *components=NULL,
                             float *scalex=NULL,float *scaley=NULL,float *scalez=NULL,
                             unsigned char **description=NULL,
                             unsigned char **courtesy=NULL,
                             unsigned char **parameter=NULL,
                             unsigned char **comment=NULL);

int checkfile(const char *filename);
unsigned int checksum(unsigned char *data,unsigned int bytes);

void swapbytes(unsigned char *data,unsigned int bytes);
void convbytes(unsigned char *data,unsigned int bytes);
void convfloat(unsigned char *data,unsigned int bytes);

unsigned char *quantize(unsigned char *volume,
                        unsigned int width,unsigned int height,unsigned int depth,
                        BOOLINT linear=FALSE,BOOLINT nofree=FALSE);

#endif
