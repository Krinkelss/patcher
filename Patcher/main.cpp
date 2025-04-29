/*
* This is an open source non-commercial project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
*/
#include <windows.h>
#include <locale.h>
#include <exception>
#include "SelectDialog.h"
#include <filesystem>
#include "Utils.h"
#include "TmpFolder.h"
#include "base64_decode.h"
#include "md5.h"
#include "Options.h"
#include "Update.h"
#include "BinaryFormat.h"

#pragma comment(lib, "version.lib" )
//#pragma warning(disable : 4996)

uint32_t FilesProcessed = 1;	// Сколько файлов обработано
uint32_t AllFiles = 0;			// Всего файлов для обработки

uint32_t md5_check;				// Проверка эш файлов
uint32_t print_console;			// Вывод информации в консоль

uint32_t mem_size;				// Память под буфер для копирования файлов

bool ApplyPatch( std::wstring GamePath, std::wstring TmpPath, std::wstring fPatch, char *memBuf, uint32_t memBufSize, DWORD MD5Check );
void FreeAllMem( char **memBuf );

int main( int argc, char* argv[] )
{	
	setlocale( LC_ALL, "Russian" );
	
	std::wstring version = GetAppVersion();

	wprintf( L"Patcher [%s]: Copyright (c) 2025 Krinkels [krinkels.org]\r\n", version.c_str() );
//////////////////////////////////////////////////////////////////////////
		
	Options Opt;
	if( Opt.get_arguments( argc, argv ) == false )
		return 0;

	if( Opt.check_update == UPDATE_ON )
	{
		try
		{
			if( CheckRelease( wstring_to_string( version ) ) == true )
				return 0;	// Обновились, можно выходить	
		}
		catch( ... )
		{
			MessageBox( nullptr, L"Неизвестная ошибка curl.\nДля отключения автоматической проверки новых версий\nзапустите программу с параметром \"-nocheck\"", L"Ошибка обновления", MB_ICONERROR );
		}
	}
					
	md5_check = Opt.md5_check;
	print_console = Opt.print_console;
	mem_size = Opt.mem_size;

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
		wprintf( L"Файл не выбран, выходим\n" );
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
			MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Не могу удалить файл 123.tmp [filesystem_error]", MB_OK | MB_ICONERROR );
			return 0;
		}
		catch( const std::error_code& e )
		{
			MessageBox( NULL, AnsiToUnicode( e.message() ).c_str(), L"Не могу удалить файл 123.tmp [error_code]", MB_OK | MB_ICONERROR );
			return 0;
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
		MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Бэкап EscapeFromTarkov.exe [filesystem_error]", MB_OK | MB_ICONERROR );
		return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
	}
	catch( const std::error_code& e )
	{
		MessageBox( NULL, AnsiToUnicode( e.message() ).c_str(), L"Бэкап EscapeFromTarkov.exe [error_code]", MB_OK | MB_ICONERROR );
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
				MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Ошибка при переименовывании файла [filesystem_error]", MB_OK | MB_ICONERROR );
				return 0;
			}
			catch( const std::error_code& e )
			{
				MessageBox( NULL, AnsiToUnicode( e.message() ).c_str(), L"Ошибка при переименовывании файла [error_code]", MB_OK | MB_ICONERROR );
				return 0;
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

	try
	{
		fTmp.init();
	}
	catch( const std::filesystem::filesystem_error &e )
	{
		MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Ошибка при создании временной папки [filesystem_error]", MB_OK | MB_ICONERROR );
		return 0;
	}
	catch( const std::error_code &e )
	{
		MessageBox( NULL, AnsiToUnicode( e.message() ).c_str(), L"Ошибка при создании временной папки [error_code]", MB_OK | MB_ICONERROR );
		return 0;
	}
		
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
	char *memBuf = ( char * )VirtualAlloc( NULL, mem_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
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
				ApplyPatch( GamePath, fTmp.ReturnTempPath(), *it, memBuf, mem_size, md5_check );
			}
			catch( const std::runtime_error& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + *it ).c_str(), L"ApplyPatch:runtime_error", MB_OK | MB_ICONERROR );
				FreeAllMem( &memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( const std::bad_alloc& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + *it ).c_str(), L"ApplyPatch:bad_alloc", MB_OK | MB_ICONERROR );
				FreeAllMem( &memBuf );
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
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + *it ).c_str(), L"Копирование файла [filesystem_error]", MB_OK | MB_ICONERROR );
				FreeAllMem( &memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( std::error_code& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.message() ) + L"\n" + *it ).c_str(), L"Копирование файла [error_code]", MB_OK | MB_ICONERROR );
				FreeAllMem( &memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}

			if( std::filesystem::remove( GamePath + L"\\123.tmp" ) == false )
			{
				MessageBox( nullptr, L"Не могу удалить файл 123.tmp", L"Внимание", MB_ICONERROR );
				FreeAllMem( &memBuf );
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
				ApplyPatch( GamePath, fTmp.ReturnTempPath(), sData, memBuf, mem_size, md5_check );
			}
			catch( const std::runtime_error& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + sData ).c_str(), L"ApplyPatch:runtime_error", MB_OK | MB_ICONERROR );
				FreeAllMem( &memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( const std::bad_alloc& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + sData ).c_str(), L"ApplyPatch:bad_alloc", MB_OK | MB_ICONERROR );
				FreeAllMem( &memBuf );
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
				MessageBox( NULL, ( AnsiToUnicode( e.what() ) + L"\n" + sData ).c_str(), L"Копирование файла [filesystem_error]", MB_OK | MB_ICONERROR );
				FreeAllMem( &memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}
			catch( std::error_code& e )
			{
				MessageBox( NULL, ( AnsiToUnicode( e.message() ) + L"\n" + sData ).c_str(), L"Копирование файла [error_code]", MB_OK | MB_ICONERROR );
				FreeAllMem( &memBuf );
				return 0; // Если не можем переименовать файл, то и пропатчить не сможем, нет смысла продолжать
			}

			if( std::filesystem::remove( GamePath + L"\\123.tmp" ) == false )
			{
				MessageBox( nullptr, L"Не могу удалить файл 123.tmp", L"Внимание", MB_ICONERROR );
				FreeAllMem( &memBuf );
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
				if( MessageBox( nullptr, ( L"Ошибка копирования файла\n" + AnsiToUnicode( e.what() ) + L"\n\nПропустить?" ).c_str(), L"Копирование файла [filesystem_error]", MB_YESNO | MB_ICONERROR ) == IDYES )
				{
					continue;
				}
				else
				{
					FreeAllMem( &memBuf );
					return 0;
				}
			}
			catch( std::error_code& e )
			{
				if( MessageBox( nullptr, ( L"Ошибка копирования файла\n" + AnsiToUnicode( e.message() ) + L"\n\nПропустить?" ).c_str(), L"Копирование файла [error_code]", MB_YESNO | MB_ICONERROR ) == IDYES )
				{
					continue;
				}
				else
				{
					FreeAllMem( &memBuf );
					return 0;
				}
			}

			wprintf( L"Скопирован файл [%d/%d]: %s\n", FilesProcessed++, AllFiles, relativePath.c_str() );
		}
	}

	FreeAllMem( &memBuf );

	std::filesystem::remove( GamePath + L"\\123.tmp" );

	return 0;
}

void FreeAllMem( char **memBuf )
{
	VirtualFree( *memBuf, 0, MEM_RELEASE );
	*memBuf = nullptr;
}

// Функция патчинга файлов
// @param (std::filesystem::path) GamePath - Путь к папке с игрой
// @param (std::filesystem::path) TmpPath - Путь к временной папке
// @param (std::filesystem::path) sData - Путь к файлу с патчем
// @param (char *) memBuf - Общий буфер для работы с файлами
// @param (uint32_t *) memBufSize - Размер выделенного буфера
// @param (DWORD) MD5Check - Проверка файлов MD5 хэш суммой. Не проверять, часть или все
bool ApplyPatch( std::wstring GamePath, std::wstring TmpPath, std::wstring fPatch, char *memBuf, uint32_t memBufSize, DWORD MD5Check )
{
	uint64_t start;
	uint64_t length;
	uint32_t ReadDataLength = 0;
	uint32_t b;
	size_t WriteData = 0, readData = 0;
	uint32_t progressPercentage = 0;
	uint32_t percent = 0;
		
	// Получим относительный путь к файлу с патчем
	std::filesystem::path relativePath = std::filesystem::relative( fPatch, TmpPath );

	// Оригинальный файл в папке с игрой
	std::filesystem::path OriginalFile = relativePath;
	OriginalFile.replace_extension();

	wprintf ( L"Обработка файла [%d/%d]: %s\n", FilesProcessed++, AllFiles, OriginalFile.wstring ().c_str () );

	uint64_t filesize = std::filesystem::file_size( wstring_to_string( fPatch ).c_str() );

	// Так, теперь можно начать работать с файлами
	FILEWrapper PatchFile( wstring_to_string( fPatch ).c_str(), false );

	// Проверяем идентификатор файла, наш патч или не наш
	char header[ 10 ] = { 0 };
	PatchFile.readfile( header, 9, "Ошибка при чтении идентификатора в файле патча" );

	if( lstrcmpA( header, "FRSNCDLTA" ) != 0 )
	{
		throw std::runtime_error( "Неизвестный формат патча" );
	}

	// Теперь проверим версию
	//PatchFile.readfile( TmpBuf, 1, "Ошибка при чтении версии в файле патча" );
	uint32_t version = PatchFile.getint( "Ошибка при чтении версии в файле патча" );

	if( version != 1 ) // Версия должна быть равна 1
		throw std::runtime_error( "Некорректный байт версии" );

	// Читаем длину json строки
	uint32_t jsonLength = PatchFile.read_7bit_encoded_int();

	// По идее такого не может быть, но чем чёрт не шутит
	if( jsonLength > 256 )
		throw std::runtime_error( "Размер json строки больше чем буфер" );

	// Читаем саму строку
	char metadataStr[ 256 ];
	PatchFile.readfile( metadataStr, jsonLength, "Ошибка при чтении json строки в файле патча" );
	metadataStr[ jsonLength ] = '\0';

	std::unordered_map<std::string, std::string> parsedJson = ParseJson( metadataStr );

	// Данные патченного файла
	expectedFileHashAlgorithm = parsedJson[ "expectedFileHashAlgorithm" ];	// MD5 или другой алгоритм
	expectedFileHash = parsedJson[ "expectedFileHash" ];						// Строка в base64

	// Данные оригинального файла
//	PatchFile.baseFileHashAlgorithm = parsedJson[ "baseFileHashAlgorithm" ];			// MD5 или другой алгоритм
//	PatchFile.baseFileHash = parsedJson[ "baseFileHash" ];								// Строка в base64

	if( expectedFileHashAlgorithm != "MD5" )
		MD5Check = MD5_OFF;	// В json массиве задан какой то другой алгорит хэширования, значит, в дальнейшем, с MD5 работать не будем

	FILEWrapper FileBeforeFix( wstring_to_string( GamePath + L"\\" + OriginalFile.wstring() ).c_str(), false );
	FILEWrapper FileAfterFix( wstring_to_string( GamePath + L"\\123.tmp" ).c_str(), true );

	CMd5 md5Context;

	if( MD5Check == MD5_ALL || MD5Check == MD5_ACS )
		Md5_Init( &md5Context );

	while( ( b = PatchFile.getint( "Ошибка чтения служебного байта" ) ) > 0 )
	{
		if( b == CopyCommand )
		{
			PatchFile.readfile( &start, sizeof( start ), "Ошибка чтения смещения начала для копирования" );
			PatchFile.readfile( &length, sizeof( length ), "Ошибка чтения размера копируемого участка" );

			// Устанавливаем смещение в оригинальном файле
			// Чтоб от туда прочитать данные размером "length"
			FileBeforeFix.fseekfile( start, "Ошибка при установке смещения в оригинальном файле" );
						
			while( length != 0 )
			{
				// Выбираем минимум. При использовании малого количества памяти memBufSize будет меньше чем length. Но не всегда
				ReadDataLength = min( memBufSize, length );

				// Читаем данные
				readData = FileBeforeFix.readfile( memBuf, ReadDataLength, "Ошибка при чтении оригинального файла" );

				if( MD5Check == MD5_ALL || MD5Check == MD5_ACS )
					Md5_Update( &md5Context, ( unsigned char * )memBuf, readData );

				// Пишем в файл
				FileAfterFix.writefile( memBuf, ReadDataLength, "Ошибка при записи в патченный файл" );

				length -= ReadDataLength;
			}
		}
		else
			if( b == DataCommand )
			{
				PatchFile.readfile( &length, sizeof( length ), "Ошибка чтения размера блока для копирования" );

				while( length != 0 )
				{
					// Выбираем минимум. При использовании малого количества памяти memBufSize будет меньше чем length. Но не всегда
					ReadDataLength = min( memBufSize, length );

					PatchFile.readfile( memBuf, ReadDataLength, "Ошибка чтения блока для копирования" );

					if( MD5Check == MD5_ALL || MD5Check == MD5_ACS )
						Md5_Update( &md5Context, ( unsigned char * )memBuf, ReadDataLength );

					// Пишем в файл
					FileAfterFix.writefile( memBuf, ReadDataLength, "Ошибка при записи в патченный файл" );

					length -= ReadDataLength;
				}
			}
			else
			{
				throw std::runtime_error( "Неизвестный байт" );
			}


		percent = SetFilePointer( PatchFile.get(), NULL, NULL, FILE_CURRENT ) * 100 / filesize;


		if( progressPercentage < percent && print_console == PRINT_CONSOLE_MAX )
		{
			progressPercentage = percent;
			wprintf( L"\r%d%%", percent );
		}
	}

	if( MD5Check == MD5_ALL || MD5Check == MD5_ACS )
	{
		unsigned char md5Digest[ 16 ]; // MD5-хэш состоит из 16 байт
		Md5_Final( &md5Context, md5Digest );

		std::string expectedFH = base64_decode( expectedFileHash );
		std::string digesthash = binToHex( ( char * )md5Digest, 16 );


		if( expectedFH != digesthash )
		{
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
		
	return true;
}