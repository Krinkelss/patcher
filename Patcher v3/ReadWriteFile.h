#pragma once
#include <unordered_map>
#include "FileMapper.h"

class ReadWrite
{
public:
	ReadWrite();
	~ReadWrite();
	void Apply( FileMapper *ToPatchFile, FileMapper *ToFinishFile, FileMapper *ToPatch );

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
//	char* cursor;

	PVOID ToFileData;
	PVOID FinisFileData;
	char *PatchData;

	DWORD fileSize;
};