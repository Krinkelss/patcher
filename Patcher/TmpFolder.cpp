/*
* This is an open source non-commercial project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
*/
#include <windows.h>
#include <filesystem>
#include "TmpFolder.h"

TmpFolder::TmpFolder()
{
	wchar_t ShortPath[ _MAX_PATH ];
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

	lstrcpy( mTempFolder, Tmp2 );
}

TmpFolder::~TmpFolder()
{
	DeleteFolder();
}

wchar_t *TmpFolder::ReturnTempPath( void )
{
	return mTempFolder;
}

void TmpFolder::DeleteFolder( void )
{
	std::filesystem::remove_all( mTempFolder );
}