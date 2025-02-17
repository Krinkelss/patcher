#pragma once
#include <shlobj.h>
#include <shlwapi.h>
#include <string>

#pragma comment(lib, "shlwapi.lib")

class SelectDialog : private IFileDialogEvents, private IFileDialogControlEvents
{
public:
	SelectDialog();
	~SelectDialog();
	HRESULT PickFolder( const wchar_t *pszTitle );
	HRESULT PickFile( const wchar_t *pszTitle );
	HRESULT SaveFileDialog( const wchar_t *pszTitle );
	std::wstring GetResult();

private:
	void GetExePath( wchar_t *mPath );

private:
	IFACEMETHODIMP QueryInterface( REFIID riid, void **ppv );
	IFACEMETHODIMP_( ULONG ) AddRef();
	IFACEMETHODIMP_( ULONG ) Release();
	IFACEMETHODIMP OnFileOk( IFileDialog *pfd );
	IFACEMETHODIMP OnFolderChanging( IFileDialog * /*pfd*/, IShellItem * /*psi*/ );
	IFACEMETHODIMP OnFolderChange( IFileDialog * /*pfd*/ );
	IFACEMETHODIMP OnSelectionChange( IFileDialog *pfd );
	IFACEMETHODIMP OnShareViolation( IFileDialog * /*pfd*/, IShellItem * /*psi*/, FDE_SHAREVIOLATION_RESPONSE * /*pResponse*/ );
	IFACEMETHODIMP OnTypeChange( IFileDialog * /*pfd*/ );
	IFACEMETHODIMP OnOverwrite( IFileDialog * /*pfd*/, IShellItem * /*psi*/, FDE_OVERWRITE_RESPONSE * /*pResponse*/ );
	IFACEMETHODIMP OnItemSelected( IFileDialogCustomize * /*pfdc*/, DWORD /*dwIDCtl*/, DWORD /*dwIDItem*/ );
	IFACEMETHODIMP OnCheckButtonToggled( IFileDialogCustomize *pfdc, DWORD dwIDCtl, BOOL bChecked );
	IFACEMETHODIMP OnButtonClicked( IFileDialogCustomize *pfdc, DWORD dwIDCtl );
	IFACEMETHODIMP OnControlActivating( IFileDialogCustomize*, DWORD );

private:
	std::wstring Result;
};