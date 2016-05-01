#ifndef S6CRYPT_H
#define S6CRYPT_H

#include "s6data.h"
#include "stdbool.h"
#include "stdint.h"

#define S6_FEISTEL(sum, value0, value1, num, key)	S6_FEISTEL2(sum, value0, value1, num, key)
#define S6_FEISTEL2(s, v0, v1, n, k)				(((s ^ v0) + (v1 ^ k[((s >> 2) & 3) ^ n & 3])) ^ ((16 * v1 ^ (v0 >> 3)) + ((v1 >> 5) ^ 4 * v0)))

bool S6_Decrypt(void *dataPtr, uint32_t size, uint32_t encryptionID);
bool S6_Encrypt(void *dataPtr, uint32_t size, uint32_t encryptionID);
bool S6FileHeader_Decrypt(S6FileHeader *header, S6DirEntry *dirEntry);
void S6FileHeader_Encrypt(S6FileHeader *header, S6DirEntry *dirEntry);

typedef struct 
{
	uint32_t encryptionID;

	uint32_t key[4];
	uint32_t start;
	uint32_t delta;
} XXTEA_Key;

#define S6_NUM_KEYS			3
#define S6_HEAD_CRYPTID		0x29D58DC5
#define S6_MAP_CRYPTID		0x161680E5
#define S6_BBA_CRYPTID	    0x605E90C5
#define S6_FILE_CRYPTID		0x9BB3F065

const XXTEA_Key S6KeyList[] = 
{
	{ S6_HEAD_CRYPTID, { 0x912201AB, 0x81D90511, 0xFA842E62, 0x21475421 }, 0xDAA66D2B, 0x61C88647 },
	{ S6_BBA_CRYPTID,  { 0x3AD22B55, 0xC5263F15, 0x459E4F55, 0xFC5969CC }, 0x0915E073, 0x6C6A96CB },
	{ S6_MAP_CRYPTID,  { 0x804B7535, 0x953F7021, 0x32A58E66, 0x04DB71E8 }, 0x5384540F, 0x61C88647 }
};

const uint32_t S6FileKey[] = { 0xAB0FDAE6, 0x8A423249, 0x8A5EC670, 0x67F8261D };


#endif