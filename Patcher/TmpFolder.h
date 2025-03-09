/*
* This is an open source non-commercial project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
*/
#pragma once
#include <windows.h>

class TmpFolder
{
public:
	TmpFolder() = default;
	~TmpFolder();
	void init();
	std::wstring ReturnTempPath( void );

private:
	//wchar_t mTempFolder[ _MAX_PATH ] = { 0 };
	std::wstring mTempFolder;
};

