
#include "shokCrypt.h"

const uint32_t SHoK_Key[] = SHoK_KEY;

bool SHoK_Encrypt(uint32_t *data, uint32_t len)
{
	if(len % 4 || len < 8)
		return false;
	
	SHoK_XXTEA_Encrypt(data, len / 4, SHoK_Key);
	return true;
}

bool SHoK_Decrypt(uint32_t *data, uint32_t len)
{
	if(len % 4 || len < 8)
		return false;
	
	SHoK_XXTEA_Decrypt(data, len / 4, SHoK_Key);
	return true;
}

void SHoK_XXTEA_Encrypt(uint32_t *data, uint32_t blocks, const uint32_t *key)
{
    int32_t sum = 0;
    uint32_t last = data[blocks - 1];
		
    for(uint32_t rounds = 52 / blocks + 6; rounds; rounds--)
    {
        sum -= SHoK_DELTA;
		
        for(uint32_t pos = 0; pos < blocks - 1; pos++)
			last = data[pos] += SHoK_FEISTEL(sum, data[pos + 1], last, pos, key);

		last = data[blocks - 1] += SHoK_FEISTEL(sum, data[0], last, blocks - 1, key);
    }
}

void SHoK_XXTEA_Decrypt(uint32_t *data, uint32_t blocks, const uint32_t *key)
{
	uint32_t rounds = 52 / blocks + 6;
    int32_t sum = -SHoK_DELTA * rounds;
	uint32_t last = data[0];
	
    for(; rounds; rounds--)
    {
        for(uint32_t pos = blocks - 1; pos > 0; pos--)
			last = data[pos] -= SHoK_FEISTEL(sum, last, data[pos - 1], pos, key);

		last = data[0] -= SHoK_FEISTEL(sum, last, data[blocks - 1], 0, key);
		sum += SHoK_DELTA;
    }
}