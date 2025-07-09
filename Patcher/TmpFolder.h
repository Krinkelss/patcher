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

