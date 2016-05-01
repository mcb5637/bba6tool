#ifndef TREEREADER_H
#define TREEREADER_H

#include <stdint.h>

typedef struct DSE DirStructEntry;

struct DSE
{
	int type;
	DirStructEntry *nextSibling;
	DirStructEntry *firstChild;
	DirStructEntry *nextLinear;
	DirStructEntry *prevLinear;
	uint32_t offset;
	uint32_t compressedCRC32;
	uint32_t compressedSize;
	uint32_t decompressedCRC32;
	uint32_t decompressedSize;
	uint32_t dirOffset;
	char path[2];				//dynamic!

};


//DirStructEntry *readFolder(char *dirname, int *counter, int *strLenCnt, DirStructEntry **lastElm);
DirStructEntry *ReadRootFolder(DirStructEntry *root, char *dirname, int *counter, int *fileNameCounter, DirStructEntry **lastElement);
#endif