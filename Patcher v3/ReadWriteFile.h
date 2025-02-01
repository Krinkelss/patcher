#pragma once
#include <unordered_map>
#include "FileMapper.h"

#define MD5_OFF		0	// Выкл
#define MD5_FULL	1	// Проверка всех файлов
#define MD5_IN		2	// Проверка только входящих файлов
#define MD5_ACS		3	// Проверить только Assembly-CSharp.dll


class ReadWrite
{
public:
	ReadWrite();
	~ReadWrite();
	void Apply( FileMapper *ToPatchFile, FileMapper *ToFinishFile, FileMapper *ToPatch, DWORD MD5Check = MD5_OFF );

private:	
	template <typename T>
	T Read();
	uint8_t ReadByte();
	std::string ReadString( size_t length );
	void ReadBytes( byte *Data, size_t length );
	std::unordered_map<std::string, std::string> ParseJson( const std::string& jsonString );

private:
	uint8_t CopyCommand = 0x60;
	uint8_t DataCommand = 0x80;

private:
	// Данные входящего файла
	std::string expectedFileHashAlgorithm;	// MD5 или другой алгоритм
	std::string expectedFileHash;			// Строка в base64
	
	// Данные выходящего файла
	std::string baseFileHashAlgorithm;	// MD5 или другой алгоритм
	std::string baseFileHash;			// Строка в base64

private:
//	char* cursor;

	PVOID ToFileData;
	PVOID FinisFileData;
	char *PatchData;

	DWORD fileSize;
};