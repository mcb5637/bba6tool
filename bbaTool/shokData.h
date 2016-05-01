#pragma pack(4)
#include <stdint.h>

typedef struct
{
	uint32_t compressionHeader;
	uint32_t dataLength;
	uint32_t compressedSize;
	uint32_t decompressedSize;
	uint32_t adler32;
} CompressedFileHeader;

typedef struct
{
	char archiveHeader[3];
	uint8_t archiveVersion;
	uint32_t archiveLength;

	char BAH_Header[3];
	uint8_t BAH_Version;
	uint32_t BAH_Length;
	uint32_t unknownField;
	uint32_t gameVersion;

	char fileDataHeader[3];
	uint8_t fileDataVersion;
	uint32_t fileDataLength;

} FileHeader;

typedef struct
{
	
	char dirHeader[3];
	uint8_t dirVersion;
	uint32_t dirLength;
	char entryHeader[3];
	uint8_t entryVersion;
	uint32_t fileEntriesLength;
	uint32_t compressionHeader;
	uint32_t dataLength;
	uint32_t compressedSize;
	uint32_t decompressedSize;
	uint32_t adler32;
} DirectoryHeader;

typedef struct
{
	uint32_t entryType;
	uint32_t fileOffset;
	uint32_t fileSize;
	uint16_t fileNameLength;
	uint16_t dirPart;
	int32_t firstChildDirOffset;
	int32_t nextSiblingDirOffset;
	uint64_t timestamp;
	char fileName[4];  //minimum length
} DirectoryEntry;

typedef struct
{
	char BAh_Header[3];
	uint8_t BAh_Version;
	uint32_t BAh_Length;

	uint32_t hashTableSize;

} HashTableHeader;

typedef struct
{
	uint32_t hash;
	uint32_t BAeOffset;

} HashTableElement;


