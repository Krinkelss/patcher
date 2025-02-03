#include <windows.h>
#include <vector>
#include <filesystem>
#include "Utils.h"
#include "miniz/miniz.h"

// получить версию файла
std::wstring GetFileVersion( const std::wstring& filePath )
{
	DWORD handle = 0;
	DWORD size = GetFileVersionInfoSize( filePath.c_str(), &handle );
	if( size == 0 )
	{
		return L"";
	}

	std::vector<BYTE> buffer( size );
	if( !GetFileVersionInfo( filePath.c_str(), handle, size, buffer.data() ) )
	{
		return L"";
	}

	VS_FIXEDFILEINFO* fileInfo = nullptr;
	UINT len = 0;
	if( !VerQueryValue( buffer.data(), L"\\", reinterpret_cast< LPVOID* >( &fileInfo ), &len ) )
	{
		return L"";
	}

	if( fileInfo == nullptr )
	{
		return L"";
	}

	std::wstring version = std::to_wstring( HIWORD( fileInfo->dwFileVersionMS ) ) + L"." +
		std::to_wstring( LOWORD( fileInfo->dwFileVersionMS ) ) + L"." +
		std::to_wstring( HIWORD( fileInfo->dwFileVersionLS ) ) + L"." +
		std::to_wstring( LOWORD( fileInfo->dwFileVersionLS ) );

	//return version;
	return std::to_wstring( LOWORD( fileInfo->dwFileVersionLS ) );
}

std::wstring GetLastFiveDigits( const std::wstring& version )
{
	size_t pos = version.find_last_of( L'.' );
	if( pos == std::wstring::npos || pos + 1 + 5 > version.length() )
	{
		return L"";
	}
	return version.substr( pos + 1 ).substr( 0, 5 );
}

// Получить "первую" версию из имени патча
std::wstring ExtractVersion( const std::wstring& fileName )
{
	size_t start = fileName.find( L'.' );
	if( start == std::wstring::npos )
	{
		return L"";
	}
	start++; // Move past the first '.'

	size_t end = fileName.find( L'-', start );
	if( end == std::wstring::npos )
	{
		return L"";
	}

	return GetLastFiveDigits( fileName.substr( start, end - start ) );
}

//Получить имя файла
std::wstring GetFileName( const std::wstring& filePath )
{
	size_t pos = filePath.find_last_of( L"\\/" );
	if( pos == std::wstring::npos )
	{
		return filePath;
	}
	return filePath.substr( pos + 1 );
}

std::string wstring_to_string( const std::wstring& wstr )
{
	int size_needed = WideCharToMultiByte( CP_UTF8, 0, wstr.c_str(), ( int )wstr.size(), NULL, 0, NULL, NULL );
	std::string str( size_needed, 0 );
	WideCharToMultiByte( CP_UTF8, 0, wstr.c_str(), ( int )wstr.size(), &str[ 0 ], size_needed, NULL, NULL );
	return str;
}

bool ExtractZipArchive( std::wstring zipFilePath, std::wstring extractToPath )
{
	mz_zip_archive zip_archive;
	memset( &zip_archive, 0, sizeof( zip_archive ) );

	std::string zipFilePathA = wstring_to_string( zipFilePath );
	std::string extractToPathA = wstring_to_string( extractToPath );

	// Инициализация архива
	if( !mz_zip_reader_init_file( &zip_archive, zipFilePathA.c_str(), 0 ) )
	{
		printf( "Не удалось открыть ZIP файл: %s\n", zipFilePathA.c_str() );
		return false;
	}

	// Получение количества файлов в архиве
	int file_count = ( int )mz_zip_reader_get_num_files( &zip_archive );

	std::string extracted_file_path;
	std::string full_output_path;
	std::string Rep;

	int currentfile = 1;

	// Извлечение каждого файла
	for( int i = 0; i < file_count; ++i )
	{
		mz_zip_archive_file_stat file_stat;
		if( !mz_zip_reader_file_stat( &zip_archive, i, &file_stat ) )
		{
			wprintf( L"Не удалось получить информацию о файле №%d\n", i );
			return false;
		}

		Rep = file_stat.m_filename;
		std::replace( Rep.begin(), Rep.end(), '/', '\\' );

		// Создание пути для извлеченного файла
		extracted_file_path = std::string( extractToPathA ) + Rep.c_str();
		
		// Создаем необходимые директории
		std::string directory = extracted_file_path.substr( 0, extracted_file_path.find_last_of( "\\" ) );
		std::filesystem::create_directories( directory );

		// Создаем полный путь к выходному файлу
		/*full_output_path = extractToPathA + std::string( file_stat.m_filename, file_stat.m_filename + strlen( file_stat.m_filename ) );

		// Создаем необходимые директории
		std::string directory = full_output_path.substr( 0, full_output_path.find_last_of( "\\" ) );
		std::filesystem::create_directory( directory );*/

		// Извлечение файла
		if( !mz_zip_reader_extract_to_file( &zip_archive, i, extracted_file_path.c_str(), 0 ) )
		{
			printf(  "Не удалось извлечь файл: %s\n", Rep.c_str() );
			return false;
		}

		printf( "Распаковано файлов [%d/%d]: %s\n", currentfile++, file_count, Rep.c_str() );
	}

	// Завершение работы с архивом
	mz_zip_reader_end( &zip_archive );

	return true;
}

// Поиск файлов
std::vector<std::wstring> SearchAllFilesInDirectory( const std::wstring& directory )
{
	std::vector<std::wstring> files;
	for( const auto& entry : std::filesystem::recursive_directory_iterator( directory ) )
	{
		if( entry.is_regular_file() )
		{
			files.push_back( entry.path().wstring() );
		}
	}
	return files;
}

// Поиск файлов по маске в заданной папке(без подпапок)
std::vector<std::wstring> SearchFilesInDirectory( const std::wstring& directory, const std::wstring& filePattern )
{
	std::vector<std::wstring> files;
	for( const auto& entry : std::filesystem::directory_iterator( directory ) )
	{
		if( entry.is_regular_file() && entry.path().filename().wstring().find( filePattern ) == 0 )
		{
			files.push_back( entry.path().wstring() );
		}
	}
	return files;
}