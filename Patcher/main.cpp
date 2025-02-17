#include <windows.h>
#include <exception>
#include "SelectDialog.h"
#include <filesystem>
#include "FileMapper.h"
#include "Utils.h"
#include "TmpFolder.h"
#include "base64_decode.h"
#include "md5.h"
#include "Options.h"
#include "Update.h"

#pragma comment(lib, "version.lib" )
#pragma warning(disable : 4996)

//#define MD5_OFF		0	// Выкл
//#define MD5_FULL		1	// Проверка всех файлов
//#define MD5_IN		2	// Проверка только входящих файлов
//#define MD5_ACS		3	// Проверить только Assembly-CSharp.dll

//constexpr uint32_t MD5_OFF	= 0; // Выкл
//constexpr uint32_t MD5_ALL	= 1; // Проверка всех файлов
//constexpr uint32_t MD5_ACS	= 2; // Проверить только Assembly-CSharp.dll

constexpr uint32_t TWOGIGA = ( unsigned )2 * 1024 * 1024 * 1024;	// 2 гигабайта

uint32_t FilesProcessed = 1;	// Сколько файлов обработано
uint32_t AllFiles = 0;			// Всего файлов для обработки

uint32_t md5_check;				// Проверка эш файлов
uint32_t print_console;			// Вывод информации в консоль

bool ApplyPatch( std::wstring GamePath, std::wstring TmpPath, std::wstring fPatch, char *memBuf, DWORD MD5Check );
void FreeAllMem( char * memBuf );


int main( int argc, char* argv[] )
{	
	setlocale( LC_ALL, "Russian" );
	
	std::wstring version = GetAppVersion();

	wprintf( L"Patcher [%s]: Copyright (c) 2025 Krinkels [krinkels.org]\r\n", version.c_str() );
//////////////////////////////////////////////////////////////////////////
	
	Options Opt;
	Opt.get_arguments( argc, argv );

	if( Opt.check_update == UPDATE_ON )
	{
		try
		{
			if( CheckRelease( "1.0" ) == true )
				return 0;	// Обновились, можно выходить
		}
		catch( std::runtime_error& e )
		{
			MessageBoxA( nullptr, e.what(), "CheckRelease", MB_ICONERROR );
		}
	}
					
	md5_check = Opt.md5_check;
	print_console = Opt.print_console;

	//GetLatestRelease( "123" );
	
	//Выберем папку с игрой	
	SelectDialog sDialog;
	HRESULT hr = sDialog.PickFolder( L"Выберите папку с игрой" );
	if( FAILED( hr ) )
	{
		wprintf( L"Папка не выбрана, выходим\n" );
		return 0;
	}
	std::wstring GamePath = sDialog.GetResult();	// Путь к папке с игрой

	// Теперь файл с патчем
	hr = sDialog.PickFile( L"Выберите файл с патчем" );
	if( FAILED( hr ) )
	{
		wprintf( L"Папка не выбрана, выходим\n" );
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
			MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Не могу удалить файл 123.tmp", MB_OK | MB_ICONERROR );
			return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
		}
		catch( const std::error_code& e )
		{
			MessageBox( NULL, AnsiToUnicode( e.message() ).c_str(), L"Не могу удалить файл 123.tmp", MB_OK | MB_ICONERROR );
			return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
		}
	}

	//////////////////////////////////////////////////////////////
	// Проверим версию файла
	std::wstring versionexe = GetFileVersion( GamePath + L"\\EscapeFromTarkov.exe" );

	wprintf( L"Обнаружена версии Таркова: %s\n", versionexe.c_str() );

	std::wstring PatchFrom = ExtractVersion( std::filesystem::path( PatchFile ).filename().wstring() );

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
		wprintf( L"Произведён бэкап файла EscapeFromTarkov.exe\n" );
	}
	catch( const std::filesystem::filesystem_error& e )
	{
		MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Бэкап EscapeFromTarkov.exe", MB_OK | MB_ICONERROR );
		return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
	}
	catch( const std::error_code& e )
	{
		MessageBox( NULL, AnsiToUnicode( e.message() ).c_str(), L"Бэкап EscapeFromTarkov.exe", MB_OK | MB_ICONERROR );
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
				MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"std::filesystem", MB_OK | MB_ICONERROR );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( const std::error_code& e )
			{
				MessageBox( NULL, AnsiToUnicode( e.message() ).c_str(), L"std::filesystem", MB_OK | MB_ICONERROR );
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
	if( ExtractZipArchive( PatchFile, fTmp.ReturnTempPath() ) == false )
		return 0;

	// Теперь найдём все файлы во временной папке
	std::vector<std::wstring> mPatchList = SearchAllFilesInDirectory( fTmp.ReturnTempPath() );

	if( mPatchList.size() == 0 )
	{
		wprintf( L"Странно, не найдено ни одного файла из распакованного патча. Дальнейшая работа невозможна\n" );
		return 0;
	}
	
	// Нужно выделить буфер для работы с файлами
	// Максимальный размер файла таркова 1.5 гигабайт, выделим 2 гигабайта, а то мало ли что
	char *memBuf = ( char * )VirtualAlloc( NULL, TWOGIGA, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	if( memBuf == NULL )
	{
		wprintf( L"Ошибка выделения памяти через VirtualAlloc. Errorcode: %d\n", GetLastError() );
		return 0;
	}

	AllFiles = mPatchList.size();

	wprintf( L"Будет обработано файлов: %d\n", AllFiles );

	// В последнее время появляются какие то проблемы с Assembly-CSharp.dll
	// По этому, если задана MD5_ACS, то первым делом проверим эту библиотечку
	if( md5_check == MD5_ACS || md5_check == MD5_ALL )
	{		
		// Ищем
		auto it = std::find_if( mPatchList.begin(), mPatchList.end(), []( const std::wstring& file )
		{
			return std::filesystem::path( file ).filename() == L"Assembly-CSharp.dll.patch";
		} );

		// Находим
		if( it != mPatchList.end() )
		{			
			try
			{
				ApplyPatch( GamePath, fTmp.ReturnTempPath(), *it, memBuf, md5_check );
			}
			catch( const std::runtime_error& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + *it ).c_str(), L"ApplyPatch:runtime_error", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( const std::bad_alloc& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + *it ).c_str(), L"ApplyPatch:bad_alloc", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}

			// Теперь нужно скопировать файл в папку
			// Получим относительный путь к файлу с патчем
			std::wstring relativePath = std::filesystem::relative( *it, fTmp.ReturnTempPath() ).replace_extension();

			try
			{
				const std::filesystem::copy_options copyOptions = std::filesystem::copy_options::overwrite_existing;
				std::filesystem::copy( GamePath + L"\\123.tmp", GamePath + L"\\" + relativePath, copyOptions );
			}
			catch( const std::filesystem::filesystem_error& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + *it ).c_str(), L"Копирование файла", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( std::error_code& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.message() ) + L"\n" + *it ).c_str(), L"Копирование файла", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}

			if( std::filesystem::remove( GamePath + L"\\123.tmp" ) == false )
			{
				MessageBox( nullptr, L"Не могу удалить файл 123.tmp", L"Внимание", MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0;
			}

			// Ошибки не случилось, можно работать с файлом дальше
			// Удалим значение из вектора
			mPatchList.erase( it );
		}		
	}
		
	for( auto sData : mPatchList )
	{
		if( std::filesystem::path( sData ).extension().compare( L".patch" ) == 0 ) // То бишь расширение файла равно .patch
		{// Вот и файл патча

			// Путь к папке с игрой
			// Список патчей
			try
			{
				ApplyPatch( GamePath, fTmp.ReturnTempPath(), sData, memBuf, md5_check );
			}
			catch( const std::runtime_error& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + sData ).c_str(), L"ApplyPatch:runtime_error", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( const std::bad_alloc& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + sData ).c_str(), L"ApplyPatch:bad_alloc", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}

			// Теперь нужно скопировать файл в папку
			// Получим относительный путь к файлу с патчем
			std::wstring relativePath = std::filesystem::relative( sData, fTmp.ReturnTempPath() ).replace_extension();

			try
			{
				const std::filesystem::copy_options copyOptions = std::filesystem::copy_options::overwrite_existing;
				std::filesystem::copy( GamePath + L"\\123.tmp", GamePath + L"\\" + relativePath, copyOptions );
			}
			catch( const std::filesystem::filesystem_error& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + sData ).c_str(), L"Копирование файла", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( std::error_code& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.message() ) + L"\n" + sData ).c_str(), L"Копирование файла", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}

			if( std::filesystem::remove( GamePath + L"\\123.tmp" ) == false )
			{
				MessageBox( nullptr, L"Не могу удалить файл 123.tmp", L"Внимание", MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0;
			}
		}
		else
		{// Просто файл, который нужно скопировать с заменой

			// Получим относительный путь к файлу с патчем
			std::wstring relativePath = std::filesystem::relative( sData, fTmp.ReturnTempPath() );

			try
			{
				std::filesystem::path FilePath = GamePath + L"\\" + relativePath;
				std::filesystem::create_directories( FilePath.parent_path() );
				std::filesystem::copy( sData, FilePath, std::filesystem::copy_options::overwrite_existing );
			}
			catch( const std::filesystem::filesystem_error& e )
			{
				MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Копирование файла", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( std::error_code& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.message() ) + L"\n" + sData ).c_str(), L"Копирование файла", MB_OK | MB_ICONERROR );
				FreeAllMem( memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}

			wprintf( L"Скопирован файл [%d/%d]: %s\n", FilesProcessed++, AllFiles, relativePath.c_str() );
		}
	}

	FreeAllMem( memBuf );

	std::filesystem::remove( GamePath + L"\\123.tmp" );

	return 0;
}

void FreeAllMem( char * memBuf )
{
	VirtualFree( memBuf, 0, MEM_RELEASE );
	memBuf = nullptr;
}

// Функция патчинга файлов
// @param (std::filesystem::path) GamePath - Путь к папке с игрой
// @param (std::filesystem::path) TmpPath - Путь к временной папке
// @param (std::filesystem::path) sData - Путь к файлу с патчем
// @param (char *) memBuf - Общий буфер для работы с файлами
// @param (DWORD) MD5Check - Проверка файлов MD5 хэш суммой. Не проверять, часть или все
bool ApplyPatch( std::wstring GamePath, std::wstring TmpPath, std::wstring fPatch, char *memBuf, DWORD MD5Check )
{
	long start;
	int64_t length;
	uint32_t b;
	size_t WriteData = 0, readData = 0;
	uint32_t progressPercentage = -1;
	
	// Получим относительный путь к файлу с патчем
	std::filesystem::path relativePath = std::filesystem::relative( fPatch, TmpPath );

	// Оригинальный файл в папке с игрой
	std::filesystem::path OriginalFile = relativePath;
	OriginalFile.replace_extension();

	// Так, теперь можно начать работать с файлами
	FileMapper PatchFile;			// Файл с патчем, .patch

	// Обработаем файл с патчем. 
	// Один раз было что у него другая "шапка" была, и отсюда всё сломалось
	try
	{
		PatchFile.Init( fPatch.c_str(), false );
	}
	catch( const std::runtime_error& e )
	{
		MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + fPatch ).c_str(), L"PatchFile.Init", MB_OK | MB_ICONERROR );
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

	// Данные патченного файла
	PatchFile.expectedFileHashAlgorithm = parsedJson[ "expectedFileHashAlgorithm" ];	// MD5 или другой алгоритм
	PatchFile.expectedFileHash = parsedJson[ "expectedFileHash" ];						// Строка в base64

	// Данные оригинального файла
//	PatchFile.baseFileHashAlgorithm = parsedJson[ "baseFileHashAlgorithm" ];			// MD5 или другой алгоритм
//	PatchFile.baseFileHash = parsedJson[ "baseFileHash" ];								// Строка в base64

	if( PatchFile.expectedFileHashAlgorithm != "MD5" )
		MD5Check = MD5_OFF;	// В json массиве задан какой то другой алгорит хэширования, значит, в дальнейшем, с MD5 работать не будем

	FILE *FileBeforeFix;	// Файл который нужно патчить
	FILE *FileAfterFix;		// Файл который получится после патча
	errno_t err;

	err = fopen_s( &FileBeforeFix, wstring_to_string( GamePath + L"\\" + OriginalFile.wstring() ).c_str(), "rb" );
	if( err !=0 )
		throw std::runtime_error( "Ошибка при открытия оригинального файла" );
	
	err = fopen_s( &FileAfterFix, wstring_to_string( GamePath + L"\\123.tmp" ).c_str(), "wb" );
	if( err != 0 )
		throw std::runtime_error( "Ошибка при открытия патченного файла" );

	char *PatchDataEnd = PatchFile.GetView() + PatchFile.GetFileSize(); // Указатель на конец данных

	CMd5 md5Context;

	if( MD5Check == MD5_ALL || MD5Check == MD5_ACS )
		Md5_Init( &md5Context );
	
	while( PatchFile.PatchData < PatchDataEnd )
	{
		b = PatchFile.ReadByte();

		if( b == PatchFile.CopyCommand )
		{
			start = ( int64_t )PatchFile.Read<int64_t>();
			length = ( int64_t )PatchFile.Read<int64_t>();

			// Устанавливаем смещение в оригинальном файле
			// Чтоб от туда прочитать данные размером "length"
			if( fseek( FileBeforeFix, start, SEEK_SET ) != 0 )
			{
				fclose( FileAfterFix );
				fclose( FileBeforeFix );
				throw std::runtime_error( "Ошибка при установке смещения в оригинальном файле" );
			}

			// Читаем данные
			readData = fread( memBuf, 1, length, FileBeforeFix );
			if( ferror( FileBeforeFix ) )
			{
				fclose( FileAfterFix );
				fclose( FileBeforeFix );
				throw std::runtime_error( "Ошибка при чтении оригинального файла" );
			}

			if( MD5Check == MD5_ALL || MD5Check == MD5_ACS )
				Md5_Update( &md5Context, ( unsigned char * )memBuf, readData );

			// Пишем в файл
			WriteData = fwrite( memBuf, 1, length, FileAfterFix );
			if( ferror( FileAfterFix ) )
			{
				fclose( FileAfterFix );
				fclose( FileBeforeFix );
				throw std::runtime_error( "Ошибка при записи в патченный файл" );
			}

			if( WriteData != length )
			{
				fclose( FileAfterFix );
				fclose( FileBeforeFix );
				throw std::runtime_error( "CopyCommand: Ошибка при записи в патченный файл, WriteData != length" );
			}
		}
		else
		if( b == PatchFile.DataCommand )
		{
			length = ( int64_t )PatchFile.Read<int64_t>();
			
			PatchFile.ReadBytes( memBuf, length );

			if( MD5Check == MD5_ALL || MD5Check == MD5_ACS )
				Md5_Update( &md5Context, ( unsigned char * )memBuf, length );

			// Пишем в файл
			WriteData = fwrite( memBuf, 1, length, FileAfterFix );
			if( ferror( FileAfterFix ) )
			{
				fclose( FileAfterFix );
				fclose( FileBeforeFix );
				throw std::runtime_error( "Ошибка при записи в патченный файл" );
			}

			if( WriteData != length )
			{
				fclose( FileAfterFix );
				fclose( FileBeforeFix );
				throw std::runtime_error( "DataCommand: Ошибка при записи в патченный файл, WriteData != length" );
			}
		}
		else
		{
			fclose( FileAfterFix );
			fclose( FileBeforeFix );
			throw std::runtime_error( "Неизвестный байт" );
		}

		uint32_t percent = PatchFile.GetPosition() * 100 / PatchFile.GetFileSize();


		if( progressPercentage != percent && print_console == PRINT_CONSOLE_MAX/* && percent > 0 && percent % 10 == 0*/ )
		{
			progressPercentage = percent;
			wprintf( L"\r%d%%", percent );
		}
	}

	if( MD5Check == MD5_ALL || MD5Check == MD5_ACS )
	{
		unsigned char md5Digest[ 16 ]; // MD5-хэш состоит из 16 байт
		Md5_Final( &md5Context, md5Digest );

		std::string expectedFileHash = base64_decode( PatchFile.expectedFileHash );
		std::string digesthash = binToHex( ( char * )md5Digest, 16 );

		if( expectedFileHash != digesthash )
		{
			fclose( FileAfterFix );
			fclose( FileBeforeFix );
			throw std::runtime_error( "Хэш файла " + std::filesystem::path( fPatch ).filename().string() + " и хэш в файле патча не совпадают\nДальнейшая работа невозможна" );
		}

		if( print_console == PRINT_CONSOLE_MAX )
		{
			wprintf( L"\r100%%\n" );

			// Выводим MD5-хэш в виде шестнадцатеричной строки
			wprintf( L"MD5-хэш файла: " );
			for( int i = 0; i < 16; i++ )
			{
				printf( "%02x", md5Digest[ i ] );
			}
			wprintf( L"\nХэш файла и хэш из патча совпадают\n" );
		}
	}

	fclose( FileAfterFix );
	fclose( FileBeforeFix );

	wprintf( L"Обработан файл [%d/%d]: %s\n", FilesProcessed++, AllFiles, OriginalFile.wstring().c_str() );

	return true;
}