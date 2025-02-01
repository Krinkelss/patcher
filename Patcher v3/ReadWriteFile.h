#pragma once
#include <unordered_map>
#include "FileMapper.h"

#define MD5_OFF		0	// ����
#define MD5_FULL	1	// �������� ���� ������
#define MD5_IN		2	// �������� ������ �������� ������
#define MD5_ACS		3	// ��������� ������ Assembly-CSharp.dll


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
	// ������ ��������� �����
	std::string expectedFileHashAlgorithm;	// MD5 ��� ������ ��������
	std::string expectedFileHash;			// ������ � base64
	
	// ������ ���������� �����
	std::string baseFileHashAlgorithm;	// MD5 ��� ������ ��������
	std::string baseFileHash;			// ������ � base64

private:
//	char* cursor;

	PVOID ToFileData;
	PVOID FinisFileData;
	char *PatchData;

	DWORD fileSize;
};