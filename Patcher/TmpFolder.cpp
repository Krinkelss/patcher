/*
* This is an open source non-commercial project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
*/
#include <windows.h>
#include <filesystem>
#include <string>
#include "TmpFolder.h"

void TmpFolder::init()
{
	/*wchar_t ShortPath[_MAX_PATH];
	wchar_t TempPath[ _MAX_PATH ];
	wchar_t Tmp2[ _MAX_PATH ] = { 0 };

	GetTempPath( _MAX_PATH, ShortPath );

	if( GetShortPathName( ShortPath, TempPath, _MAX_PATH ) == 0 )
	{
		lstrcpy( TempPath, ShortPath );
	}

	DWORD pid = GetCurrentProcessId() ^ 0xa1234568;

	wsprintf( Tmp2, L"%sbtl_temp_%x\\", TempPath, pid );
	CreateDirectory( Tmp2, 0 );

	lstrcpy( mTempFolder, Tmp2 );*/

	DWORD pid = GetCurrentProcessId() ^ 0xa1234568;

	std::wstring TempPath = std::filesystem::temp_directory_path().wstring();

	mTempFolder = TempPath + L"btl_temp_" + std::to_wstring( pid );

	std::filesystem::create_directory( mTempFolder );
}

TmpFolder::~TmpFolder()
{
	std::filesystem::remove_all( mTempFolder );
}

std::wstring TmpFolder::ReturnTempPath( void )
{
	return mTempFolder;
}