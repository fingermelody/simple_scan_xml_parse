
/***************************************************************
 * MPQ hash algorithm is designed by Bizzard Corporation, it can
 * reduce the key conflict efficiently.
 ***************************************************************/


#include "MPQ_hash.h"
static int cryptTable[0x500];

void prepareCryptTable()
{
    unsigned long seed = 0x00100001, index1 = 0, index2 = 0, i;

    for( index1 = 0; index1 < 0x100; index1++ )
    {
        for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )
        {
            unsigned long temp1, temp2;

            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp1 = (seed & 0xFFFF) << 0x10;

            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp2 = (seed & 0xFFFF);

            cryptTable[index2] = ( temp1 | temp2 );
        }
    }
}



/*
 * hash a string to a long value.
 * */
unsigned long MPQ_HashString( char *index, unsigned long dwHashType)
{
    unsigned char *key  = (unsigned char *)index;
	unsigned long seed1 = 0x7FED7FED;
	unsigned long seed2 = 0xEEEEEEEE;
    int ch;

    while( *key != 0 )
    {
        ch = toupper(*key++);

        seed1 = cryptTable[(dwHashType << 8) + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
    }
    return seed1;
}

void* MPQ_Generate_Key(char* string){
	MPQ_KEY* key = (MPQ_KEY*)malloc(sizeof(MPQ_KEY));
	key->dwHash1 = MPQ_HashString(string,1);
	key->dwHash2 = MPQ_HashString(string,2);
	return key;
}

int MPQ_comparator(const void *l, const void *r){
	MPQ_KEY* mpq_key =(MPQ_KEY*)l;
	if((mpq_key->dwHash1 == MPQ_HashString((char*)r,1))&&(mpq_key->dwHash2 == MPQ_HashString((char*)r,2)))
		return 0;
	return -1;
}

size_t hash_map_MPQ_hash_func(const void *key, size_t capacity){
	const int HASH_OFFSET=0;
	return MPQ_HashString((char*)key,HASH_OFFSET)%capacity;
}
