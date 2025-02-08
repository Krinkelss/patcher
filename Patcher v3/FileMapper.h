#pragma once
#include <windows.h>
#include <string>
#include "BinaryFormat.h"

class FileMapper : public BinaryFormat
{
public:
	FileMapper() = default;
	~FileMapper();
	void Init( const wchar_t *filePath, bool writeAccess = false );
	char* GetView();
	uint32_t GetPosition();
	DWORD GetFileSize();
	void ReadData( char *Data, size_t Offset, size_t Len );

public:
	/*template <typename T>
	T Read();*/
	uint8_t ReadByte();
	std::string ReadString( size_t length );
	void ReadBytes( char *Data, size_t length );

public:
	template <typename T>
	T Read()
	{
		T value = *reinterpret_cast< T* >( PatchData );
		PatchData += sizeof( T );
		return value;
	}

private:
	void Map();
	void ReMap( DWORD needed );
	bool FileExists( const wchar_t* fname );
	bool FileReadOnly( wchar_t* fname );
	DWORD fGetFileSize( const wchar_t* filePath );
	std::string ConvertUnicodeToAnsi( const std::wstring& unicodeString );
	
public:
	std::string FileName;
	DWORD fileSize = 0;
	char *PatchData;

private:
	HANDLE hFile;
	HANDLE hMapping;
	char *pView;	
	//uint8_t* cursor;
	PVOID pData;
	UINT CurrentOffset = 0;	
	bool EndWork;
	uint32_t Position = 0;

private:
	DWORD mappingAccess = PAGE_READONLY;
	DWORD viewAccess = FILE_MAP_READ;
};