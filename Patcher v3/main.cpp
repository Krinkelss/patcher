#include <windows.h>
#include <exception>
#include "SelectDialog.h"
#include <filesystem>
#include "FileMapper.h"
#include "Utils.h"
#include "TmpFolder.h"
#include "base64_decode.h"
#include "md5.h"

#pragma comment(lib, "version.lib" )
#pragma warning(disable : 4996)

//#define MD5_OFF		0	// Выкл
//#define MD5_FULL		1	// Проверка всех файлов
//#define MD5_IN		2	// Проверка только входящих файлов
//#define MD5_ACS		3	// Проверить только Assembly-CSharp.dll

constexpr uint32_t MD5_OFF	= 0; // Выкл
constexpr uint32_t MD5_FULL = 1; // Проверка всех файлов
constexpr uint32_t MD5_IN	= 2; // Проверка только входящих файлов
constexpr uint32_t MD5_ACS	= 3; // Проверить только Assembly-CSharp.dll

bool ApplyPatch( std::wstring GamePath, std::wstring TmpPath, std::wstring fPatch, DWORD MD5Check );

int main( void )
{
	setlocale( LC_ALL, "Russian" );
	/*DWORD dwVersion = 0;
	DWORD dwMajorVersion = 0;
	DWORD dwMinorVersion = 0;
	DWORD dwBuild = 0;

	dwVersion = GetVersion();

	// Get the Windows version.

	dwMajorVersion = ( DWORD )( LOBYTE( LOWORD( dwVersion ) ) );
	dwMinorVersion = ( DWORD )( HIBYTE( LOWORD( dwVersion ) ) );

	// Get the build number.

	if( dwVersion < 0x80000000 )
		dwBuild = ( DWORD )( HIWORD( dwVersion ) );

	wprintf( L"Patcher v3[%d.%d (%d)]: Copyright (c) 2025 Krinkels [krinkels.org]\r\n", dwMajorVersion, dwMinorVersion, dwBuild );*/
	
	//! Не забыть обработать переменную
	int md5_check = MD5_ACS;

	//Для начала, выберем папку с игрой	
	SelectDialog sDialog;
	HRESULT hr = sDialog.PickFolder( L"Выберите папку с игрой" );
	if( FAILED( hr ) )
	{
		wprintf( L"12312 Папка не выбрана, выходим\n" );
		return 0;
	}
	std::wstring GamePath = sDialog.GetResult();	// Путь к папке с игрой

	// Теперь файл с патчем
	hr = sDialog.PickFile( L"Выберите файл с патчем" );
	if( FAILED( hr ) )
	{
		wprintf( L"12312 Папка не выбрана, выходим\n" );
		return 0;
	}
	std::wstring PatchFile = sDialog.GetResult();	// Путь к файлу с патчем
	
	// Проверим наличие EscapeFromTarkov.exe
	if( std::filesystem::exists( GamePath + L"\\EscapeFromTarkov.exe" ) == false )
	{
		std::wstring Tmp = L"Файл EscapeFromTarkov.exe не обнаружен по пути:\n" + GamePath;
		MessageBox( nullptr, Tmp.c_str(), L"Внимание", MB_ICONERROR );
		return 0;
	}

	// Теперь проверим Assembly-CSharp.dll, а то мало ли
	if( std::filesystem::exists( GamePath + L"\\EscapeFromTarkov_Data\\Managed\\Assembly-CSharp.dll" ) == false )
	{
		std::wstring Tmp = L"Файл Assembly-CSharp.dll не обнаружен по пути:\n" + GamePath + L"\\EscapeFromTarkov_Data\\Managed";
		MessageBox( nullptr, Tmp.c_str(), L"Внимание", MB_ICONERROR );
		return 0;
	}
	
	// При сбое файл 123.tmp остаётся, лучше бы его удалить
	std::wstring TmpFile = GamePath + L"\\123.tmp";
	if( std::filesystem::exists( TmpFile ) == true )
	{
		try
		{
			if( std::filesystem::remove( TmpFile ) == false )
			{
				MessageBox( nullptr, L"Не могу удалить файл 123.tmp", L"Внимание", MB_ICONERROR );
				return 0;
			}
		}
		catch( const std::filesystem::filesystem_error& e )
		{
			MessageBoxA( NULL, e.what(), "std::filesystem::remove", MB_OK | MB_ICONERROR );
			return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
		}		
	}

	//////////////////////////////////////////////////////////////
	// Проверим версию файла
	std::wstring versionexe = GetFileVersion( GamePath + L"\\EscapeFromTarkov.exe" );

	wprintf( L"Обнаружена версии Таркова: %s\n", versionexe.c_str() );

	std::wstring PatchFrom = ExtractVersion( GetFileName( PatchFile ) );

	if( versionexe != PatchFrom )
	{
		wprintf( L"Патч не подходит для этой версии игры" );
		return 0;
	}

	wprintf( L"Патч подходит для этой версии игры\n" );

	// Прежде чем что то делать сделаем бэкап EscapeFromTarkov.exe
	// Если пойдёт что то не так то всегда его можно восстановить
	// Ибо ежели что, то на нём часто спотыкается патчер
	try
	{
		const std::filesystem::copy_options copyOptions = std::filesystem::copy_options::skip_existing;
		std::filesystem::copy( GamePath + L"\\EscapeFromTarkov.exe", GamePath + L"\\EscapeFromTarkov.exe_" + versionexe, copyOptions );
		wprintf( L"Произведён бэкап файла EscapeFromTarkov.exe" );
	}
	catch( const std::filesystem::filesystem_error& e )
	{
		MessageBoxA( NULL, e.what(), "Бэкап EscapeFromTarkov.exe", MB_OK | MB_ICONERROR );
		return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
	}

	// Найдём все файлы по маске "Assembly-CSharp*". Если больше двух то  это что то не то
	std::vector<std::wstring> ACfiles = SearchFilesInDirectory( GamePath + L"\\EscapeFromTarkov_Data\\Managed", L"Assembly-CSharp" );
	
	/*if( ACfiles.size() == 2 ) // Чистый клиент
	{
		
	}
	else*/
	if( ACfiles.size() == 3 ) // Хоть раз игра с сервером запускалась
	{
		auto it = std::find_if( ACfiles.begin(), ACfiles.end(), []( const std::wstring& file )
		{
			return std::filesystem::path( file ).filename() == L"Assembly-CSharp.dll.spt-bak";
		} );

		if( it != ACfiles.end() )
		{
			std::wstring backupFile = *it;
			try
			{
				std::filesystem::remove( GamePath + L"\\EscapeFromTarkov_Data\\Managed\\Assembly-CSharp.dll" );
				std::filesystem::rename( backupFile, GamePath + L"\\EscapeFromTarkov_Data\\Managed\\Assembly-CSharp.dll" );
			}
			catch( const std::filesystem::filesystem_error& e )
			{
				MessageBoxA( NULL, e.what(), "std::filesystem", MB_OK | MB_ICONERROR );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
		}
		else
		{
			std::wstring result;
			for( const auto& file : ACfiles )
			{
				result += std::filesystem::path( file ).filename().wstring() + L"\n";
			}
			MessageBox( NULL, ( L"Assembly-CSharp.dll.spt-bak не найден, но найдены другие файлы.\nДальнейшая работа невозможна.\nНайдены файлы:\n" + result ).c_str(), L"!!!", MB_OK | MB_ICONERROR );
			return 0;
		}
	}
	else
	if( ACfiles.size() > 3 )
	{
		std::wstring result;
		for( const auto& file : ACfiles )
		{
			result += std::filesystem::path( file ).filename().wstring() + L"\n";
		}
		MessageBox( NULL, ( L"Файлов Assembly-CSharp* больше трёх, дальнейшая работа невозможна.\nНайдены файлы:\n" + result ).c_str(), L"!!!", MB_OK | MB_ICONERROR );
		return 0;
	}
	
	TmpFolder fTmp;

	wprintf( L"Распаковываем патч\n" );
	//! Не забыть вернуть
	//ExtractZipArchive( PatchFile, fTmp.ReturnTempPath() );
	ExtractZipArchive( PatchFile, L"D:\\123\\Tmp\\" );

	// Теперь найдём все файлы во временной папке
	//std::vector<std::wstring> mPatchList = SearchAllFilesInDirectory( fTmp.ReturnTempPath() );
	std::vector<std::wstring> mPatchList = SearchAllFilesInDirectory( L"D:\\123\\Tmp" );

	if( mPatchList.size() == 0 )
	{
		wprintf( L"Странно, не найдено ни одного файла из распакованного патча. Дальнейшая работа невозможна\n" );
		return 0;
	}
	
	// В последнее время появляются какие то проблемы с Assembly-CSharp.dll
	// По этому, если задана MD5_ACS, то первым делом проверим эту библиотечку
	if( md5_check == MD5_ACS )
	{
		// Ищем
		auto it = std::find_if( mPatchList.begin(), mPatchList.end(), []( const std::wstring& file )
		{
			return std::filesystem::path( file ).filename() == L"Assembly-CSharp.dll.patch";
		} );

		// Находим
		if( it != mPatchList.end() )
		{
			// Функция для применения патча. Параметры:
			// Путь к папке с игрой
			// Список патчей
			//ApplyPatch( GamePath, fTmp.ReturnTempPath(), *it );
			//! не забыть вернуть
			try
			{
				ApplyPatch( GamePath, L"D:\\123\\Tmp\\", *it, md5_check );
			}
			catch( const std::runtime_error& e )
			{
				MessageBoxA( NULL, e.what(), "ApplyPatch:runtime_error", MB_OK | MB_ICONERROR );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( const std::bad_alloc& e )
			{
				MessageBoxA( NULL, e.what(), "ApplyPatch:bad_alloc", MB_OK | MB_ICONERROR );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
		}
	}

	for( auto sData : mPatchList )
	{
		//auto fName = std::filesystem::path( sData ).
		// В архиве с патчем могут быть и другие файлы, а не только .patch файлы.
		// Для начала нам нужно обработать .patch, а потом займёмся остальными
		if( std::filesystem::path( sData ).extension().compare( L".patch" ) == 0 ) // То бишь расширение файла равно .patch
		{
			// Вот и файл патча






			return 0;
		}
	}

	return 0;
}

// Функция патчинга файлов
// @param (std::filesystem::path) GamePath - Путь к папке с игрой
// @param (std::filesystem::path) TmpPath - Путь к временной папке
// @param (std::filesystem::path) PatchFile - Путь к файлу с патчем
// @param (DWORD) MD5Check - Проверка файлов MD5 хэш суммой. Не проверять, часть или все
bool ApplyPatch( std::wstring GamePath, std::wstring TmpPath, std::wstring fPatch, DWORD MD5Check )
{
	int RB = 0; // Байты для чтения из файла с патчем
	uint8_t b;
	int64_t start, length;
	long soFar;
	int readBufferSize = 4 * 1024 * 1024;

	// GamePath = D:\123
	// TmpPath = D:\123\Tmp\
	// fPatch = D:\123\Tmp\EscapeFromTarkov_Data\Managed\Assembly-CSharp.dll.patch
	// relativePath = EscapeFromTarkov_Data\Managed\Assembly-CSharp.dll.patch
	// OriginalFile = EscapeFromTarkov_Data\Managed\Assembly-CSharp.dll


	// Для начала получим относительный путь к файлу с патчем
	std::filesystem::path relativePath = std::filesystem::relative( fPatch, TmpPath );
	
	// Оригинальный файл в папке с игрой
	std::filesystem::path OriginalFile = relativePath;
	OriginalFile.replace_extension();

	// Так, теперь можно начать работать с файлами
	FileMapper FileBeforeFix;		// Файл который нужно патчить
	FileMapper FileAfterFix;		// Файл который получится после патча
	FileMapper PatchFile;			// Файл с патчем, .patch

	// Для начала обработаем файл с патчем. 
	// Один раз было что у него другая "шапка" была, и отсюда всё сломалось
	try
	{
		PatchFile.Init( fPatch.c_str(), false );
	}
	catch( const std::runtime_error& e )
	{
		MessageBoxA( NULL, e.what(), "PatchFile.Init", MB_OK | MB_ICONERROR );
		return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
	}

	// Проверяем идентификатор файла, наш патч или не наш
	std::string header = PatchFile.ReadString( PatchFile.DeltaFormatHeaderLength );
	if( header.compare( "FRSNCDLTA" ) != 0 )
		throw std::runtime_error( "Некорректный файл" );

	// Теперь проверим версию?
	uint32_t version = PatchFile.ReadByte();
	if( version != PatchFile.Version )
		throw std::runtime_error( "Некорректный байт" );

	// Может я и ошибаюсь, но, возможно, это размер json строки что будет далее
	uint32_t StrLength = PatchFile.ReadByte();

	// Опять такой же байт. Пока оставлю так, потом посмотрим
	version = PatchFile.ReadByte();

	// Читаем json данные
	std::string metadataStr = PatchFile.ReadString( StrLength );

	std::unordered_map<std::string, std::string> parsedJson = ParseJson( metadataStr );

	// Данные входящего файла
	PatchFile.expectedFileHashAlgorithm = parsedJson[ "expectedFileHashAlgorithm" ];	// MD5 или другой алгоритм
	PatchFile.expectedFileHash = parsedJson[ "expectedFileHash" ];						// Строка в base64

	// Данные выходящего файла
	PatchFile.baseFileHashAlgorithm = parsedJson[ "baseFileHashAlgorithm" ];			// MD5 или другой алгоритм
	PatchFile.baseFileHash = parsedJson[ "baseFileHash" ];								// Строка в base64

	//////////////////////////////////////////////////////////////////////////
	// Ну вот и пришло время входящего файла, и заодно и выходящего, чего уж мелочиться то
	try
	{
		FileBeforeFix.Init( ( GamePath + L"\\" + OriginalFile.wstring() ).c_str(), false );
	}
	catch( const std::runtime_error& e )
	{
		MessageBoxA( NULL, e.what(), "FileBeforeFix.Init", MB_OK | MB_ICONERROR );
		return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
	}
	try
	{
		FileAfterFix.Init( ( GamePath + L"\\123.tmp" ).c_str(), true );
	}
	catch( const std::runtime_error& e )
	{
		MessageBoxA( NULL, e.what(), "FileAfterFix.Init", MB_OK | MB_ICONERROR );
		return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
	}
	//////////////////////////////////////////////////////////////////////////

	if( PatchFile.expectedFileHashAlgorithm != "MD5" || PatchFile.baseFileHashAlgorithm != "MD5" )
		MD5Check = MD5_OFF;	// В json массиве задан какой то другой алгорит хэширования, значит, в дальнейшем, с MD5 работать не будем
	//? А нужно ли, в дальнейшем, патчить файл?

	// Проверим хзши, если всё хорошо то они совпадут
	if( MD5Check == MD5_FULL || MD5Check == MD5_IN || MD5Check == MD5_ACS )
	{
		//std::string expectedFileHash = base64_decode( PatchFile.expectedFileHash );
		std::string baseFileHash = base64_decode( PatchFile.baseFileHash );

		byte digest[ 16 ];

		CMd5 MD5;
		Md5_Init( &MD5 );
		Md5_Update( &MD5, ( byte * )FileBeforeFix.GetView(), FileBeforeFix.GetFileSize() );
		Md5_Final( &MD5, digest );

		std::string digesthash = binToHex( ( char * )digest, 16 );

		if( baseFileHash != digesthash )
			throw std::runtime_error( "Хэш файла " + FileBeforeFix.FileName + " и хэш в файле патча не совпадают\nДальнейшая работа невозможна" );
	}

	// Хэши проверили, ошибка не вылетела, можно патчить файл
	char *PatchDataEnd = PatchFile.GetView() + PatchFile.GetFileSize(); // Указатель на конец данных
	
	byte *TmpBuf = ( byte * )malloc( readBufferSize ); // Выделяем буфер один раз
	if( TmpBuf == NULL )
		throw std::bad_alloc();
	
	while( PatchFile.PatchData < PatchDataEnd )
	{
		b = PatchFile.ReadByte();
		if( b == PatchFile.CopyCommand )
		{
			start = ( int64_t )PatchFile.Read<int64_t>();
			length = ( int64_t )PatchFile.Read<int64_t>();

			if( length > readBufferSize )
			{
				TmpBuf = ( byte * )realloc( TmpBuf, length );
				if( TmpBuf == NULL )
				{
					throw std::bad_alloc();
				}
				readBufferSize = length;
			}

			FileBeforeFix.ReadData( TmpBuf, start, length );
			FileAfterFix.WriteData( TmpBuf, length );
		}
		else
		if( b == PatchFile.DataCommand )
		{
			length = ( int64_t )PatchFile.Read<int64_t>();
			soFar = 0;

			while( soFar < length )
			{
				RB = min( static_cast< int64_t >( length - soFar ), static_cast< int64_t >( readBufferSize ) );
				PatchFile.ReadBytes( TmpBuf, RB );
				FileAfterFix.WriteData( TmpBuf, RB );
				soFar += RB;
			}
		}
		else
			throw std::runtime_error( "Отладка процесса патчинга. Что то пошло не так\n" + PatchFile.FileName + "\n" + FileBeforeFix.FileName );
	}

	free( TmpBuf );
	TmpBuf = nullptr;

	// Проверим хзши после патчинга файла, если всё хорошо то они совпадут
	if( MD5Check == MD5_FULL || MD5Check == MD5_IN || MD5Check == MD5_ACS )
	{
		//std::string expectedFileHash = base64_decode( PatchFile.baseFileHash );
		std::string expectedFileHash = base64_decode( PatchFile.expectedFileHash );

		byte digest[ 16 ];

		CMd5 MD5;
		Md5_Init( &MD5 );
		Md5_Update( &MD5, ( byte * )FileAfterFix.GetView(), FileAfterFix.GetFileSize() );
		Md5_Final( &MD5, digest );

		std::string digesthash = binToHex( ( char * )digest, 16 );

		if( expectedFileHash != digesthash )
			throw std::runtime_error( "Хэш файла " + FileBeforeFix.FileName + " и хэш в файле патча не совпадают\nДальнейшая работа невозможна" );
	}

	/*ReadWrite ReadWriteData;

	ReadWriteData.Apply( &FileBeforeFix, &FileAfterFix, &PatchFile );*/

	return true;
}