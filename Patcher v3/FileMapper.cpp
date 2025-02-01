#include <string>
#include "FileMapper.h"

class WindowsException
{
public:
	WindowsException( const std::wstring& message, DWORD errorCode ) : message( message ), errorCode( errorCode )
	{}

	std::wstring getMessage() const
	{
		return message;
	}

	DWORD getErrorCode() const
	{
		return errorCode;
	}

private:
	std::wstring message;
	DWORD errorCode;
};

FileMapper::FileMapper( wchar_t *filePath, bool writeAccess ) : hFile( INVALID_HANDLE_VALUE ), hMapping( NULL ), pView( NULL ), fileSize( 0 )
{
	DWORD access = GENERIC_READ;
	DWORD creationDisposition = OPEN_EXISTING;	
	
	if( writeAccess == true )
	{
		access = GENERIC_READ | GENERIC_WRITE;
		creationDisposition = CREATE_ALWAYS;
		mappingAccess = PAGE_READWRITE;
		viewAccess = FILE_MAP_WRITE;
	}

	/*if( FileExists( filePath ) == false && writeAccess == false )
	{
		throw WindowsException( L"Файл не найден", GetLastError() );
	}*/

	/*if( FileReadOnly( filePath ) == true && writeAccess == false )
	{
		throw WindowsException( L"Файл только для чтения", GetLastError() );
	}*/
		
	hFile = CreateFileW( filePath, access, 0, NULL, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )
	{
		if( GetLastError() == ERROR_SHARING_VIOLATION )
		{
			throw WindowsException( L"File is locked by another process and will be skipped", GetLastError() );
		}

		throw WindowsException( L"Failed to open file.", GetLastError() );
	}

	fileSize = fGetFileSize( filePath );
	if( fileSize == INVALID_FILE_SIZE && writeAccess == false )
	{
		throw WindowsException( L"Не могу получить размер файла.", GetLastError() );
	}
	if( fileSize == 0 )
		fileSize = 1;
	
	Map();

	//cursor = static_cast< uint8_t* >( pView );
}

FileMapper::~FileMapper()
{
	if( pView )
	{
		UnmapViewOfFile( pView );
	}
	if( hMapping )
	{
		CloseHandle( hMapping );
	}
	if( hFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hFile );
	}
}

void FileMapper::Map()
{
	hMapping = CreateFileMappingW( hFile, NULL, mappingAccess, 0, fileSize, NULL );
	if( hMapping == NULL )
	{
		CloseHandle( hFile );
		throw WindowsException( L"Failed to create file mapping.", GetLastError() );
	}

	pView = ( BYTE * )MapViewOfFile( hMapping, viewAccess, 0, 0, 0 );
	if( pView == NULL )
	{
		CloseHandle( hMapping );
		CloseHandle( hFile );
		throw WindowsException( L"Failed to map view of file.", GetLastError() );
	}
}

void FileMapper::ReMap( DWORD needed )
{
	fileSize += needed;
	UnmapViewOfFile( pView );
	pView = NULL;
	CloseHandle( hMapping );

	Map();
}

void* FileMapper::GetView()
{
	return pView;
}

DWORD FileMapper::GetFileSize()
{
	return fileSize;
}

void FileMapper::WriteData( byte *pData, size_t bytes )
{
	if( CurrentOffset + bytes > fileSize )
		ReMap( ( CurrentOffset + bytes ) - fileSize );

	memcpy( pView + CurrentOffset, pData, bytes );
	CurrentOffset += bytes;
}

void FileMapper::ReadData( byte *Data, size_t Offset, size_t Len )
{
	if( Offset >= fileSize )
		throw WindowsException( L"Смещение больше чем сам файл", GetLastError() );

	memcpy( Data, pView + Offset, Len );
}

//////////////////////////////////////////////////////////////////////////
bool FileMapper::FileExists( wchar_t* fname )
{
	return ::GetFileAttributes( fname ) != DWORD( -1 );
}

bool FileMapper::FileReadOnly( wchar_t* fname )
{
	return ::GetFileAttributes( fname ) & FILE_ATTRIBUTE_READONLY;
}

DWORD FileMapper::fGetFileSize( wchar_t* filePath )
{
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	LARGE_INTEGER fileSize = { 0 };

	if( GetFileAttributesExW( filePath, GetFileExInfoStandard, &fileInfo ) == false )
	{
		return INVALID_FILE_SIZE;
	}

	fileSize.LowPart = fileInfo.nFileSizeLow;
	fileSize.HighPart = fileInfo.nFileSizeHigh;

	return ( DWORD )fileSize.QuadPart;
}