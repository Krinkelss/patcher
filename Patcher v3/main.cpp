#include <windows.h>
#include <exception>
#include "SelectDialog.h"
#include <filesystem>
#include "FileMapper.h"
#include "ReadWriteFile.h"
#include "Utils.h"
#include "TmpFolder.h"

#pragma comment(lib, "version.lib" )
#pragma warning(disable : 4996)


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
	std::vector<std::wstring> ACfiles = SearchFilesInDirectory( L"G:\\Games\\EscapeFromTarkov\\EscapeFromTarkov_Data\\Managed", L"Assembly-CSharp" );
	
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



	wprintf( L"Патч подходит для этой версии игры\n" );

	TmpFolder fTmp;

	wprintf( L"Распаковываем патч\n" );
	ExtractZipArchive( PatchFile, fTmp.ReturnTempPath() );

	// Теперь найдём все файлы во временной папке
	std::vector<std::wstring> mPatchList = SearchAllFilesInDirectory( fTmp.ReturnTempPath() );

	if( mPatchList.size() == 0 )
	{
		wprintf( L"СТранно, не найдено ни одного файла из распакованного патча. Дальнейшая работа невозможна\n" );
		return 0;
	}
			


	return 0;
	
	/*wchar_t FileToPatch[] = L"C:\\123\\EscapeFromTarkov.exe";
	wchar_t FinishFile[] = L"C:\\123\\EscapeFromTarkov_patch.exe";
	wchar_t Patch[] = L"C:\\123\\EscapeFromTarkov.exe.patch";*/
	

	/*try
	{
		FileMapper ToPatchFile( FileToPatch );
	}
	catch ( std::exception& e )
	{
		MessageBoxA( nullptr, e.what(), "", MB_ICONERROR );
	}

	try
	{
		FileMapper ToFinishFile( FinishFile, true );
	}
	catch( std::exception& e )
	{
		MessageBoxA( nullptr, e.what(), "", MB_ICONERROR );
	}

	try
	{
		FileMapper ToPatch( Patch );
	}
	catch( std::exception& e )
	{
		MessageBoxA( nullptr, e.what(), "", MB_ICONERROR );
	}*/
	
	
	

	//ReadWrite ReadWriteData;
	//ReadWriteData.Apply( &ToPatchFile, &ToFinishFile, &ToPatch );
		
	return 0;
}