#include "SelectDialog.h"
#include <stdexcept>

const COMDLG_FILTERSPEC c_rgSaveTypes[] =
{
	{L"Файл патча (*.patch)",		L"*.patch"}
};

SelectDialog::SelectDialog()
{
	HRESULT hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
	if( FAILED( hr ) )
		return;
}

SelectDialog::~SelectDialog()
{
	CoUninitialize();
}

HRESULT SelectDialog::PickFolder( const wchar_t *pszTitle )
{
	IFileOpenDialog *pfd;
	HRESULT hr = CoCreateInstance( CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &pfd ) );
	if( SUCCEEDED( hr ) )
	{
		DWORD dwCookie;
		hr = pfd->Advise( this, &dwCookie );
		if( SUCCEEDED( hr ) )
		{
			DWORD dwOptions;
			if( SUCCEEDED( pfd->GetOptions( &dwOptions ) ) )
			{
				// Устанавливаем опцию выбора папок
				hr = pfd->SetOptions( dwOptions | FOS_PICKFOLDERS );
			}

			if( pszTitle )
			{
				pfd->SetTitle( pszTitle );
			}

			IShellItem *pCurFolder = NULL;
			wchar_t ExEpath[ _MAX_PATH ];
			GetExePath( ExEpath );

			hr = SHCreateItemFromParsingName( ExEpath, NULL, IID_PPV_ARGS( &pCurFolder ) );
			if( SUCCEEDED( hr ) )
			{
				pfd->SetFolder( pCurFolder );
				pCurFolder->Release();
			}

			// the items selected are passed back via OnFileOk()
			// so we don't process the results of the dialog
			hr = pfd->Show( nullptr );

			pfd->Unadvise( dwCookie );
		}
		pfd->Release();
	}
	return hr;
}

HRESULT SelectDialog::PickFile( const wchar_t *pszTitle )
{
	IFileOpenDialog *pfd;
	HRESULT hr = CoCreateInstance( CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &pfd ) );
	if( SUCCEEDED( hr ) )
	{
		DWORD dwCookie;
		hr = pfd->Advise( this, &dwCookie );
		if( SUCCEEDED( hr ) )
		{
			DWORD dwOptions;
			if( SUCCEEDED( pfd->GetOptions( &dwOptions ) ) )
			{
				// Убираем опцию FOS_ALLOWMULTISELECT для выбора только одного файла
				hr = pfd->SetOptions( dwOptions | FOS_ALLNONSTORAGEITEMS );
			}

			// Задаём типы выбираемых файлов
			hr = pfd->SetFileTypes( ARRAYSIZE( c_rgSaveTypes ), c_rgSaveTypes );
			if( SUCCEEDED( hr ) )
			{
				// Первый элемент в списке
				hr = pfd->SetFileTypeIndex( 1 );
				if( SUCCEEDED( hr ) )
				{
					hr = pfd->SetDefaultExtension( L"*.patch" );
				}
			}

			/*IFileDialogCustomize* pfdCustomize = nullptr;
			hr = pfd->QueryInterface( IID_PPV_ARGS( &pfdCustomize ) );
			if( SUCCEEDED( hr ) )
			{
				pfdCustomize->AddCheckButton( FB_CHECKBOX_ID, L"Копировать файлы в папку?", FALSE );
				pfdCustomize->MakeProminent( FB_CHECKBOX_ID );
				pfdCustomize->Release();
			}*/

			if( pszTitle )
			{
				pfd->SetTitle( pszTitle );
			}

			IShellItem *pCurFolder = nullptr;
			wchar_t ExEpath[ _MAX_PATH ];
			GetExePath( ExEpath );

			hr = SHCreateItemFromParsingName( ExEpath, NULL, IID_PPV_ARGS( &pCurFolder ) );
			if( SUCCEEDED( hr ) )
			{
				pfd->SetFolder( pCurFolder );
				pCurFolder->Release();
			}

			// the items selected are passed back via OnFileOk()
			// so we don't process the results of the dialog
			hr = pfd->Show( nullptr );

			pfd->Unadvise( dwCookie );
		}
		pfd->Release();
	}
	return hr;
}

std::wstring SelectDialog::GetResult()
{
	return GetShortPath( Result );
}

std::wstring SelectDialog::GetShortPath( const std::wstring& longPath )
{
	DWORD bufferSize = GetShortPathNameW( longPath.c_str(), NULL, 0 );
	if( bufferSize == 0 )
	{
		throw std::runtime_error( "Ошибка при получении короткого имени файла" );
	}

	std::wstring shortPath( bufferSize, 0 );
	GetShortPathNameW( longPath.c_str(), &shortPath[ 0 ], bufferSize );

	return shortPath;
}

void SelectDialog::GetExePath( wchar_t *mPath )
{
	wchar_t path[ _MAX_PATH ];
	GetModuleFileName( GetModuleHandle( NULL ), path, _MAX_PATH );
	*wcsrchr( path, '\\' ) = '\0';
	wsprintf( mPath, L"%s", path );
}

//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SelectDialog::QueryInterface( REFIID riid, void **ppv )
{
	static const QITAB qit[] = {
		QITABENT( SelectDialog, IFileDialogEvents ),
		QITABENT( SelectDialog, IFileDialogControlEvents ),
		{ 0 },
	};
	return QISearch( this, qit, riid, ppv );
}

IFACEMETHODIMP_( ULONG ) SelectDialog::AddRef() { return 3; }
IFACEMETHODIMP_( ULONG ) SelectDialog::Release() { return 2; }

//! //////////////////////////////////////////////////////////////////////////

IFACEMETHODIMP SelectDialog::OnFileOk( IFileDialog *pfd )
{
	/*IShellItemArray *psia;
	HRESULT hr = GetSelectionFromSite( pfd, FOS_PICKFOLDERS & _options, &psia );
	if( SUCCEEDED( hr ) )
	{
		_DoAdd( psia );
		psia->Release();
	}*/

	/*IFileOpenDialog *pfod;
	HRESULT hr = pfd->QueryInterface( IID_PPV_ARGS( &pfod ) );
	if( SUCCEEDED( hr ) )
	{
		IShellItemArray *psia;
		hr = pfod->GetSelectedItems( &psia );
		if( SUCCEEDED( hr ) )
		{
			CreaterFileList( psia );
			psia->Release();
		}
		pfod->Release();
	}*/

	IFileOpenDialog *pfod;
	HRESULT hr = pfd->QueryInterface( IID_PPV_ARGS( &pfod ) );
	if( SUCCEEDED( hr ) )
	{
		IShellItem *psi;
		hr = pfod->GetResult( &psi );
		if( SUCCEEDED( hr ) )
		{
			PWSTR pszFolderPath;
			hr = psi->GetDisplayName( SIGDN_FILESYSPATH, &pszFolderPath );
			if( SUCCEEDED( hr ) )
			{
				// Сохраняем путь к выбранной папке
				Result = pszFolderPath;
				CoTaskMemFree( pszFolderPath );
			}
			psi->Release();
		}
		pfod->Release();
	}

	return S_OK; // S_FALSE keeps the dialog up, return S_OK to allows it to dismiss
}

IFACEMETHODIMP SelectDialog::OnFolderChanging( IFileDialog * /*pfd*/, IShellItem * /*psi*/ )
{
	return E_NOTIMPL;
}

IFACEMETHODIMP SelectDialog::OnFolderChange( IFileDialog * /*pfd*/ )
{
	// this event happens after a navigation is complete
	// if selecting folders is supported update the
	// text of the Open/Add button here based on the selection
	return E_NOTIMPL;
}

IFACEMETHODIMP SelectDialog::OnSelectionChange( IFileDialog *pfd )
{
	// Update the text of the Open/Add button here based on the selection
	IShellItem *psi;
	HRESULT hr = pfd->GetCurrentSelection( &psi );
	if( SUCCEEDED( hr ) )
	{
		SFGAOF attr;
		hr = psi->GetAttributes( SFGAO_FOLDER | SFGAO_STREAM, &attr );
		if( SUCCEEDED( hr ) && ( SFGAO_FOLDER == attr ) )
		{
			pfd->SetOkButtonLabel( L"Открыть" );
		}
		else
		{
			pfd->SetOkButtonLabel( L"Добавить" );
		}
		psi->Release();
	}
	return S_OK;
}

IFACEMETHODIMP SelectDialog::OnShareViolation( IFileDialog * /*pfd*/, IShellItem * /*psi*/, FDE_SHAREVIOLATION_RESPONSE * /*pResponse*/ ) { return E_NOTIMPL; }
IFACEMETHODIMP SelectDialog::OnTypeChange( IFileDialog * /*pfd*/ ) { return E_NOTIMPL; }
IFACEMETHODIMP SelectDialog::OnOverwrite( IFileDialog * /*pfd*/, IShellItem * /*psi*/, FDE_OVERWRITE_RESPONSE * /*pResponse*/ ) { return E_NOTIMPL; }
IFACEMETHODIMP SelectDialog::OnItemSelected( IFileDialogCustomize * /*pfdc*/, DWORD /*dwIDCtl*/, DWORD /*dwIDItem*/ ) { return E_NOTIMPL; }

IFACEMETHODIMP SelectDialog::OnCheckButtonToggled( IFileDialogCustomize *pfdc, DWORD dwIDCtl, BOOL bChecked )
{
	/*BOOL *IsChecked = nullptr;
	pfdc->GetCheckButtonState( FB_CHECKBOX_ID, IsChecked );
	rCopyFile = bChecked;*/

	return S_OK;
}

IFACEMETHODIMP SelectDialog::OnButtonClicked( IFileDialogCustomize *pfdc, DWORD dwIDCtl )
{
	/*switch( dwIDCtl )
	{
		case c_idDone:
			IFileDialog *pfd;
			if( SUCCEEDED( pfdc->QueryInterface( &pfd ) ) )
			{
				pfd->Close( S_OK );
				pfd->Release();
			}
			break;

		default:
			break;
	}*/

	return S_OK;
}

//	IFACEMETHODIMP OnCheckButtonToggled( IFileDialogCustomize * /*pfdc*/, DWORD /*dwIDCtl*/, BOOL /*bChecked*/ ) { return E_NOTIMPL; }
IFACEMETHODIMP SelectDialog::OnControlActivating( IFileDialogCustomize * /*pfdc*/, DWORD /*dwIDCtl*/ ) { return E_NOTIMPL; }