#pragma once
#include <windows.h>

#define MD5_NUM_BLOCK_WORDS  16
#define MD5_NUM_DIGEST_WORDS  4

#define MD5_BLOCK_SIZE   (MD5_NUM_BLOCK_WORDS * 4)
#define MD5_DIGEST_SIZE  (MD5_NUM_DIGEST_WORDS * 4)

typedef struct
{
	UINT64 count;
	UINT64 _pad_1;
	// we want 16-bytes alignment here
	UINT state[ MD5_NUM_DIGEST_WORDS ];
	UINT64 _pad_2[ 4 ];
	// we want 64-bytes alignment here
	byte buffer[ MD5_BLOCK_SIZE ];
} CMd5;

void Md5_Init( CMd5 *p );
void Md5_Update( CMd5 *p, byte *data, size_t size );
void Md5_Final( CMd5 *p, byte *digest );