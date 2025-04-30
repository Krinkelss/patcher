/*
* This is an open source non-commercial project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
*/
#pragma once
#include <windows.h>
#include <stdint.h>

class FILEWrapper
{
public:
	FILEWrapper( const char *filePath, bool writeAccess );
	~FILEWrapper();

public:
	DWORD readfile( void *buffer, uint32_t ReadLen, const char *ErrorText );
	void writefile( void *buffer, uint32_t WriteLen, const char *ErrorText );
	// Функция для чтения 7-битного сжатого целого числа
	uint32_t read_7bit_encoded_int( void );
	uint32_t getint( const char *ErrorText );
	void fseekfile( uint32_t offset, const char *ErrorText );
	HANDLE get( void );

private:
	HANDLE hFile;
};