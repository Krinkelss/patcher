#include <string>
#include "FileMapper.h"
#include <filesystem>

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

	std::filesystem::path path( filePath );
	FileName = path.filename().string();

	if( FileExists( filePath ) == false && writeAccess == false )
	{
		throw std::runtime_error( "Файл " + FileName + " не найден" );
	}

	/*if( FileReadOnly( filePath ) == true && writeAccess == false )
	{
		throw WindowsException( L"Файл только для чтения", GetLastError() );
	}*/
		
	hFile = CreateFileW( filePath, access, 0, NULL, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )
	{
		if( GetLastError() == ERROR_SHARING_VIOLATION )
		{
			throw std::runtime_error( "File is locked by another process and will be skipped" );
		}

		throw std::runtime_error( "Failed to open file." );
	}

	fileSize = fGetFileSize( filePath );
	if( fileSize == INVALID_FILE_SIZE && writeAccess == false )
	{
		throw std::runtime_error( "Не могу получить размер файла." );
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
		throw std::runtime_error( "Failed to create file mapping." );
	}

	pView = ( BYTE * )MapViewOfFile( hMapping, viewAccess, 0, 0, 0 );
	if( pView == NULL )
	{
		CloseHandle( hMapping );
		CloseHandle( hFile );
		throw std::runtime_error( "Failed to map view of file." );
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
		throw std::runtime_error( "Смещение больше чем сам файл" );

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

std::string FileMapper::ConvertUnicodeToAnsi( const std::wstring& unicodeString )
{
	int ansiLength = WideCharToMultiByte( CP_ACP, 0, unicodeString.c_str(), -1, NULL, 0, NULL, NULL );
	if( ansiLength == 0 )
	{
		throw std::runtime_error( "Failed to convert Unicode to ANSI." );
	}

	std::string ansiString( ansiLength, 0 );
	WideCharToMultiByte( CP_ACP, 0, unicodeString.c_str(), -1, &ansiString[ 0 ], ansiLength, NULL, NULL );

	return ansiString;
}