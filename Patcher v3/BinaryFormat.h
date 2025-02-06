#pragma once
#include<windows.h>
#include <unordered_map>
//#include "FileMapper.h"

//#define MD5_OFF		0	// ����
//#define MD5_FULL		1	// �������� ���� ������
//#define MD5_IN		2	// �������� ������ �������� ������
//#define MD5_ACS		3	// ��������� ������ Assembly-CSharp.dll


class BinaryFormat
{
public:
	uint32_t DeltaFormatHeaderLength = 9;
	uint32_t CopyCommand = 0x60;
	uint32_t DataCommand = 0x80;
	uint32_t Version = 0x01;

public:
	// ������ ��������� �����
	std::string baseFileHashAlgorithm;	// MD5 ��� ������ ��������
	std::string baseFileHash;			// ������ � base64
		
	// ������ ���������� �����
	std::string expectedFileHashAlgorithm;	// MD5 ��� ������ ��������
	std::string expectedFileHash;			// ������ � base64
};