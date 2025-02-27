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
	void Init( void );
	const std::wstring ReturnTempPath( void );

private:
	void DeleteFolder( void );

private:
	std::wstring mTempFolder;
};

