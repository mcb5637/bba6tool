#include "stdafx.h"

#include "s6data.h"
#include "s6Crypt.h"


const XXTEA_Key *GetKeyFromID(uint32_t encryptionID)
{
	for(int i = 0; i < S6_NUM_KEYS; i++)
		if(S6KeyList[i].encryptionID == encryptionID)
			return &(S6KeyList[i]);

	return 0;
}

void S6_XXTEA_Decrypt(void *dataPtr, uint32_t size, const XXTEA_Key *keyBase)
{
	uint32_t *data = (uint32_t *) dataPtr;
	uint32_t blocks = size / 4;
	int32_t sum = keyBase->start; //(uint32_t)-(rounds * SHoK_DELTA); //0xC849E4E6 ^ 0x12EF89CD; //-SHoK_DELTA * rounds;
	uint32_t last = data[0];
	
    for(; sum != 0 ;)
    {
        for(uint32_t pos = blocks - 1; pos > 0; pos--)
		{
			uint32_t val = S6_FEISTEL(sum, last, data[pos - 1], pos, keyBase->key);
			last = data[pos] -= val;
		}

		uint32_t val = S6_FEISTEL(sum, last, data[blocks - 1], 0, keyBase->key);
		last = data[0] -= val;
		sum += keyBase->delta;
    }
}

void S6_XXTEA_Encrypt(void *dataPtr, uint32_t size, const XXTEA_Key *keyBase)
{
	uint32_t *data = (uint32_t *) dataPtr;
	uint32_t blocks = size / 4;
    int32_t sum = 0;
    uint32_t last = data[blocks - 1];
		
	for(; sum != keyBase->start; )
    {
		sum -= keyBase->delta;
		
        for(uint32_t pos = 0; pos < blocks - 1; pos++)
			last = data[pos] += S6_FEISTEL(sum, data[pos + 1], last, pos, keyBase->key);

		last = data[blocks - 1] += S6_FEISTEL(sum, data[0], last, blocks - 1, keyBase->key);
    }
}

bool S6_Decrypt(void *dataPtr, uint32_t size, uint32_t encryptionID)
{
	const XXTEA_Key *key = GetKeyFromID(encryptionID);
	if(key == 0)
		return false;
	S6_XXTEA_Decrypt(dataPtr, size, key);
	return true;
}

bool S6_Encrypt(void *dataPtr, uint32_t size, uint32_t encryptionID)
{
	const XXTEA_Key *key = GetKeyFromID(encryptionID);
	if(key == 0)
		return false;
	S6_XXTEA_Encrypt(dataPtr, size, key);
	return true;
}

bool S6FileHeader_Decrypt(S6FileHeader *header, S6DirEntry *dirEntry)
{
	uint32_t *data = (uint32_t *)header;
	uint32_t *entry = (uint32_t *)dirEntry;

	for(int i = 3; i >= 0; i--)
	{
		int selector = (3 + i) & 3;

		data[i] ^= data[selector] ^ entry[i] ^ S6FileKey[3-i];
	}

	return header->compressedSize == dirEntry->compressedSize;
}

void S6FileHeader_Encrypt(S6FileHeader *header, S6DirEntry *dirEntry)
{
	uint32_t *data = (uint32_t *)header;
	uint32_t *entry = (uint32_t *)dirEntry;

	for(int i = 0; i < 4; i++)
	{
		int selector = (3 + i) & 3;

		data[i] ^= data[selector] ^ entry[i] ^ S6FileKey[3-i];
	}
}