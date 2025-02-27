/*
* This is an open source non-commercial project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
*/
#include <windows.h>
#include <filesystem>
#include "Utils.h"
#include "TmpFolder.h"

TmpFolder::~TmpFolder()
{
	DeleteFolder();
}

void TmpFolder::Init( void )
{
	try
	{
		std::filesystem::path tempPath = std::filesystem::temp_directory_path();
		DWORD pid = GetCurrentProcessId() ^ 0xa1234568;
		std::wstring tempFolder = tempPath / ( L"btl_temp_" + std::to_wstring( pid ) );

		std::filesystem::create_directory( tempFolder );
		mTempFolder = tempFolder;
	}
	catch( const std::filesystem::filesystem_error& e )
	{
		MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Ошибка создания временной папки [filesystem_error]", MB_OK | MB_ICONERROR );
		throw;
	}
	catch( const std::error_code& e )
	{
		MessageBox( NULL, AnsiToUnicode( e.message() ).c_str(), L"Ошибка создания временной папки [error_code]", MB_OK | MB_ICONERROR );
		throw;
	}
}

const std::wstring TmpFolder::ReturnTempPath( void )
{
	return mTempFolder;
}

void TmpFolder::DeleteFolder( void )
{
	//std::filesystem::remove_all( mTempFolder );
	try
	{
		std::filesystem::remove_all( mTempFolder );
	}
	catch( const std::filesystem::filesystem_error& e )
	{
		MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Ошибка удаления временной папки [ filesystem_error ]", MB_OK | MB_ICONERROR );
	}
	catch( const std::error_code& e )
	{
		MessageBox( NULL, AnsiToUnicode( e.message() ).c_str(), L"Ошибка удаления временной папки [ error_code ]", MB_OK | MB_ICONERROR );
	}
}