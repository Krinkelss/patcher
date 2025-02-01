#include "windows.h"
#include "FileMapper.h"
#include "ReadWriteFile.h"
#include "md5.h"
#include <exception>
#include "SelectDialog.h"

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
	std::wstring SelectFolder = sDialog.GetResult();

	// Теперь файл с патчем
	hr = sDialog.PickFile( L"Выберите файл с патчем" );
	if( FAILED( hr ) )
	{
		wprintf( L"12312 Папка не выбрана, выходим\n" );
		return 0;
	}
	std::wstring Selectile = sDialog.GetResult();
	
	
	
	
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