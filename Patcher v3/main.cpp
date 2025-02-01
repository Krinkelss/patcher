#include "windows.h"
#include "FileMapper.h"
#include "ReadWriteFile.h"

int main( void )
{
	wchar_t FileToPatch[] = L"C:\\123\\EscapeFromTarkov.exe";
	wchar_t FinishFile[] = L"C:\\123\\EscapeFromTarkov_patch.exe";
	wchar_t Patch[] = L"C:\\123\\EscapeFromTarkov.exe.patch";

	FileMapper ToPatchFile( FileToPatch );
	FileMapper ToFinishFile( FinishFile, true );
	FileMapper ToPatch( Patch );

	//mFile.Apply();

	ReadWrite ReadWriteData;
	ReadWriteData.Apply( &ToPatchFile, &ToFinishFile, &ToPatch );
		
	return 0;
}