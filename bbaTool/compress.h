
#include "s6data.h"
#include <stdio.h>

#define CHUNK 1024*1024 //16384
extern "C" {
int inf(FILE *source, FILE *dest, S6FileHeader *header);
int def(FILE *source, FILE *dest, int level, S6FileHeader *fileHeader, uint32_t *crc32_in, uint32_t *crc32_out);
}