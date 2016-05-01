#include "stdafx.h"

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include "compress.h"
#include "s6data.h"
#include "zlib/zlib.h"

extern "C" {
/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */

	unsigned char in[CHUNK];
    unsigned char out[CHUNK];

int inf(FILE *source, FILE *dest, S6FileHeader *header)
{
	bool fixHeader = true;
    int ret;
    unsigned have;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
		if(fixHeader)
		{
			((uint32_t *)in)[0] = header->fileHeader[0];
			((uint32_t *)in)[1] = header->fileHeader[1];
			fixHeader = false;
		}
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            //assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;

            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level, S6FileHeader *fileHeader, uint32_t *crc32_in, uint32_t *crc32_out)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
	uint32_t crcOut = 0;
	uint32_t crcIn = 0;
	uint32_t headerReadBytes = 0;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;

			crcIn = crc32(crcIn, in, strm.avail_in);
            ret = deflate(&strm, flush);    /* no bad return value */
            //assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
			if(headerReadBytes == 8)
				crcOut = crc32(crcOut, out, have);
			else
			{
				uint32_t remaining = 8 - headerReadBytes;
				uint32_t readBytes = (have > remaining) ? remaining : have;
				for(uint32_t i = 0; i < readBytes; i++)
				{
					((uint8_t *)fileHeader->fileHeader)[headerReadBytes] = out[i];
					headerReadBytes++;
				}
				if(have > readBytes)
					crcOut = crc32(crcOut, out+readBytes, have-readBytes);
			}

            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        //assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    //assert(ret == Z_STREAM_END);        /* stream will be complete */

	*crc32_in = crcIn;
	*crc32_out = crcOut;
	fileHeader->compressedSize = strm.total_out;
	fileHeader->decompressedSize = strm.total_in;

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}
}