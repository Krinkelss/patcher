#pragma once
#include <windows.h>
#include <string>

class FileMapper
{
public:
	FileMapper( wchar_t *filePath, bool writeAccess = false );
	~FileMapper();
	void* GetView();
	DWORD GetFileSize();
	void WriteData( byte *pData, size_t bytes );
	void ReadData( byte *Data, size_t Offset, size_t Len );

private:
	void Map();
	void ReMap( DWORD needed );
	bool FileExists( wchar_t* fname );
	bool FileReadOnly( wchar_t* fname );
	DWORD fGetFileSize( wchar_t* filePath );
	std::string ConvertUnicodeToAnsi( const std::wstring& unicodeString );

private:
	HANDLE hFile;
	HANDLE hMapping;
	BYTE *pView;
	DWORD fileSize;
	//uint8_t* cursor;
	PVOID pData;
	UINT CurrentOffset = 0;
	std::string FileName;

private:
	DWORD mappingAccess = PAGE_READONLY;
	DWORD viewAccess = FILE_MAP_READ;
};