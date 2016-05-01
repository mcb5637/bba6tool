#ifndef S6DATA_H
#define S6DATA_H

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>

#define ELM_DIR				256
#define ELM_COMPRESSED		1
#define ELM_UNCOMPRESSED	0

#define BUFSZ	1024*1024			

#pragma pack(push)
#pragma pack(4)

typedef struct
{
	char archiveHeader[3];			//BAF
	uint8_t archiveVersion;			//4
	uint32_t unknown5;				//5
	uint32_t headerSize;			//64;
	uint32_t headerEncrID;			//0x29D58DC5
} BBAHeader;

typedef struct
{
	uint32_t directoryOffset;
	uint32_t null;
	uint32_t directoryLength;
	uint32_t CRC32;
	uint32_t dirEncrID;
	uint32_t unknown1;
	uint32_t filler[10];
} S6ArcHeader;

typedef struct 
{
	uint32_t compressedSize;
	uint32_t decompressedSize;
	uint32_t data[2];				//dynamic
} CompressedData;

/*// For each file
uint32 {8}    - Timestamp
uint32 {4}    - Decompressed Filesize
uint32 {4}    - CRC32 Checksum
uint32 {4}    - Filetype Identifier
uint32 {4}    - null
uint32 {4}    - Offset
uint32 {4}    - null
uint32 {4}    - Compressed Filesize
uint32 {4}    - CRC32 Checksum
uint32 {8}    - null
uint32 {4}    - Filename Length
uint32 {4}    - Filename Offset
int32 {4}     - First Offset
int32 {4}     - Next Offset
char {x}      - Filename
byte {0-3}    - Padding */
typedef struct 
{
	uint64_t timestamp;
	uint32_t decompressedSize;
	uint32_t CRC32;
	uint32_t filetype;
	uint8_t null1[4];
	uint32_t offset;
	uint8_t null2[4];
	uint32_t compressedSize;
	uint32_t compressedCRC32;
	uint8_t null3[8];
	uint32_t filenameLength;
	uint32_t dirPart;
	int32_t firstChild;
	int32_t nextSibling;
	char filename[4];				//dynamic
} S6DirEntry;
/*
uint32 {4}    - Directory Header Size (64)
uint32 {4}    - Offset File Entries
uint32 {4}    - Offset File Hashtable
byte {52}     - null

char {3}      - Archive Header (BAF)
byte {1}      - Version (4)
uint32 {4}    - Unknown (5)
uint32 {4}    - Header Size (64)
uint32 {4}    - Header Encryption Identifier (0x29D58DC5)

uint32 {16}   - null
uint32 {4}    - Directory Encryption Identifier
byte {44}     - null

uint32 {32}   - Encryption Table
uint32 {32}   - null
uint32 {32}   - null
uint32 {32}   - null*/
typedef struct 
{
	uint32_t headerSize;			//64
	uint32_t fileEntriesOffset;
	uint32_t hashtableOffset;
	uint8_t null1[52];
	char archiveHeader[3];			//BAF
	uint8_t archiveVersion;			//4
	uint8_t unknown5;				//5
	uint32_t headerSize2;			//64;
	uint32_t headerEncrID;			//0x29D58DC5
	uint8_t null2[16];
	uint32_t dirEncrID;				//	E5 80 16 16 ?
	uint32_t unknown1;				//  B5 1B C8 D1 ??
	uint8_t null3[40];				
	uint32_t encryptionTable[8];	
	uint8_t null4[96];
	uint32_t entriesCount;
	S6DirEntry entries[1];			//dynamic!
} S6Directory;

typedef struct 
{
	uint32_t compressedSize;
	uint32_t decompressedSize;
	uint32_t fileHeader[2];
} S6FileHeader;

typedef struct 
{
	uint32_t CRC32;
	uint32_t offset;
} S6HTEntry;

typedef struct
{
	uint32_t entriesCount;
	S6HTEntry entries[1];	//dynamic
} S6HashTable;

#pragma pack(pop)
#endif