#include <windows.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "CheckGameFile.h"
#include "SelectDialog.h"
#include "Utils.h"
#include "filewripper.h"
#include "Options.h"
#include "md5.h"
#include "base64_decode.h"

void FreeMem( char **memBuf )
{
	VirtualFree( *memBuf, 0, MEM_RELEASE );
	*memBuf = nullptr;
}

//! Выводить в консоль или всё или только "битые" файлы
void checkfile( void )
{
	std::string filePath;
	uint32_t gFileSize = 0;
	CMd5 md5Context;
	uint64_t length;
	uint32_t ReadDataLength = 0;
	unsigned char md5Digest[ 16 ]; // MD5-хэш состоит из 16 байт
	std::string digesthash;

	//Выберем папку с игрой	
	SelectDialog sDialog;
	HRESULT hr = sDialog.PickFolder( L"Выберите папку с игрой" );
	if( FAILED( hr ) )
	{
		wprintf( L"Папка не выбрана, выходим\n" );
		return;
	}
	std::string GamePath = wstring_to_string( sDialog.GetResult() );	// Путь к папке с игрой

	// Проверим наличие EscapeFromTarkov.exe
	if( std::filesystem::exists( GamePath + "\\ConsistencyInfo" ) == false )
	{
		std::wstring Tmp = L"Файл ConsistencyInfo не обнаружен по пути:\n" + sDialog.GetResult();
		MessageBox( nullptr, Tmp.c_str(), L"Внимание", MB_ICONERROR );
		return;
	}

	uint32_t cipSize = std::filesystem::file_size( GamePath + "\\ConsistencyInfo" );
	if( cipSize == 0 )
		return;

	char *memBuf = ( char * )malloc( cipSize + 1 );
	memset( memBuf, 0, cipSize + 1 );
	
	FILEWrapper PatchFile( ( GamePath + "\\ConsistencyInfo" ).c_str(), false );
	PatchFile.readfile( memBuf, cipSize, "Ошибка чтение файла ConsistencyInfo" );

	nlohmann::json jsonData;

	try
	{
		jsonData = nlohmann::json::parse( memBuf );
	}
	catch( const nlohmann::json::parse_error& e )
	{
		free( memBuf );
		throw std::runtime_error( e.what() );
	}

	free( memBuf );

	// Версия EscapeFromTarkov.exe из ConsistencyInfo
	std::string Ver = jsonData[ "Version" ];

	printf( "%s\n", Ver.c_str() );
		
	// Выделим 10 мегабайт
	memBuf = ( char * )VirtualAlloc( NULL, SMALL_MEM, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	if( memBuf == NULL )
	{
		throw std::runtime_error( "Ошибка выделения памяти через VirtualAlloc. Errorcode: " + GetLastError() );
	}
		
	uint32_t jsonDataSize = jsonData[ "Entries" ].size();
	uint32_t i = 1;
	wprintf( L"Будет проверено %d файлов\n", jsonDataSize );

	// Начнём проверять файлы
	// Будем считать что выбрано INTEGRITY_CHECK_ALL
	for( const nlohmann::json &entry : jsonData[ "Entries" ] )
	{
		printf( "[%d\\%d] %s\n", i++, jsonDataSize, std::string( entry[ "Path" ].get<std::string>() ).c_str() );

		filePath = GamePath + "\\" + entry[ "Path" ].get<std::string>();

		if( std::filesystem::exists( filePath ) == false )
		{
			wprintf( L"Файл %s не найден, пропускаем\n", AnsiToUnicode( entry[ "Path" ].get<std::string>() ).c_str() );
			continue;
		}

		gFileSize = std::filesystem::file_size( filePath );

		// Самое простое, сраним размеры
		if( gFileSize != entry[ "Size" ].get<uint32_t>() )
		{
			//FreeMem( &memBuf );
			//throw std::runtime_error( "Размер файла " + entry[ "Path" ].get<std::string>() + " и значение из ConsistencyInfo не совпадают" );
			wprintf( L"Размер файла %s и значение из ConsistencyInfo не совпадают\n", AnsiToUnicode( entry[ "Path" ].get<std::string>() ).c_str() );
			continue; // Нет смысла дальше продолжать
		}
		
		// Теперь сравним хэши
		Md5_Init( &md5Context );

		FILEWrapper gFile( filePath.c_str(), false );
		
		length = gFileSize;

		while( length != 0 )
		{
			ReadDataLength = min( SMALL_MEM, length );

			gFile.readfile( memBuf, ReadDataLength, "Ошибка чтения файла" );

			Md5_Update( &md5Context, ( unsigned char * )memBuf, ReadDataLength );

			length -= ReadDataLength;
		}
				
		Md5_Final( &md5Context, md5Digest );

		digesthash = binToHex( ( char * )md5Digest, 16 );
		std::transform( digesthash.begin(), digesthash.end(), digesthash.begin(), ::tolower );

		if( entry[ "Hash" ].get<std::string>() != digesthash )
		{
			//FreeMem( &memBuf );
			//throw std::runtime_error( "Хэш файла " + entry[ "Path" ].get<std::string>() + " и хэш из ConsistencyInfo не совпадают" );
			wprintf( L"Хэш файла %s и хэш из ConsistencyInfo не совпадают\n", AnsiToUnicode( entry[ "Path" ].get<std::string>() ).c_str() );
		}
	}

	FreeMem( &memBuf );

	return;
}