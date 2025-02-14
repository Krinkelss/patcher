#include <windows.h>
#include <vector>
#include <filesystem>
#include "Utils.h"
#include "miniz/miniz.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>

// �������� ������ �����
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

// ��������� ������ ������ �� ����� ����� � ������
std::wstring GetLastFiveDigits( const std::wstring& version )
{
	size_t pos = version.find_last_of( L'.' );
	if( pos == std::wstring::npos || pos + 1 + 5 > version.length() )
	{
		return L"";
	}
	return version.substr( pos + 1 ).substr( 0, 5 );
}

// �������� "������" ������ �� ����� �����
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

std::string wstring_to_string( const std::wstring& wstr )
{
	int size_needed = WideCharToMultiByte( CP_UTF8, 0, wstr.c_str(), ( int )wstr.size(), NULL, 0, NULL, NULL );
	if( size_needed == 0 )
		throw std::runtime_error( "������ ����������� Unicode � ANSI" );
	
	std::string str( size_needed, 0 );
	if( WideCharToMultiByte( CP_UTF8, 0, wstr.c_str(), ( int )wstr.size(), &str[ 0 ], size_needed, NULL, NULL ) == 0 )
		throw std::runtime_error( "������ ����������� Unicode � ANSI" );

	return str;
}

std::wstring AnsiToUnicode( const std::string& ansi )
{
	// ���������� ����� �������������� ������ � UTF-16
	int length = MultiByteToWideChar( CP_ACP, 0, ansi.c_str(), -1, nullptr, 0 );
	if( length == 0 )
	{
		throw std::runtime_error( "Failed to convert ANSI to Unicode" );
	}

	// �������� ������ ��� �������������� ������
	std::wstring unicode( length, 0 );

	// ��������� ��������������
	if( MultiByteToWideChar( CP_ACP, 0, ansi.c_str(), -1, &unicode[ 0 ], length ) == 0 )
	{
		throw std::runtime_error( "Failed to convert ANSI to Unicode" );
	}

	// ������� ����������� ������� ������
	unicode.resize( length - 1 );

	return unicode;
}

bool ExtractZipArchive( std::wstring zipFilePath, std::wstring extractToPath )
{
	mz_zip_archive zip_archive;
	memset( &zip_archive, 0, sizeof( zip_archive ) );

	std::string zipFilePathA = wstring_to_string( zipFilePath );
	std::string extractToPathA = wstring_to_string( extractToPath );

	// ������������� ������
	if( !mz_zip_reader_init_file( &zip_archive, zipFilePathA.c_str(), 0 ) )
	{
		printf( "�� ������� ������� ZIP ����: %s\n", zipFilePathA.c_str() );
		return false;
	}

	// ��������� ���������� ������ � ������
	int file_count = ( int )mz_zip_reader_get_num_files( &zip_archive );

	std::string extracted_file_path;
	std::string full_output_path;
	std::string Rep;

	int currentfile = 0;

	// ���������� ������� �����
	for( int i = 0; i < file_count; ++i )
	{
		mz_zip_archive_file_stat file_stat;
		if( !mz_zip_reader_file_stat( &zip_archive, i, &file_stat ) )
		{
			wprintf( L"�� ������� �������� ���������� � ����� �%d\n", i );
			return false;
		}

		Rep = file_stat.m_filename;
		std::replace( Rep.begin(), Rep.end(), '/', '\\' );

		// �������� ���� ��� ������������ �����
		extracted_file_path = std::string( extractToPathA ) + Rep.c_str();
		
		// ������� ����������� ����������
		std::string directory = extracted_file_path.substr( 0, extracted_file_path.find_last_of( "\\" ) );
		std::filesystem::create_directories( directory );

		// ������� ������ ���� � ��������� �����
		/*full_output_path = extractToPathA + std::string( file_stat.m_filename, file_stat.m_filename + strlen( file_stat.m_filename ) );

		// ������� ����������� ����������
		std::string directory = full_output_path.substr( 0, full_output_path.find_last_of( "\\" ) );
		std::filesystem::create_directory( directory );*/

		// ���������� �����
		if( !mz_zip_reader_extract_to_file( &zip_archive, i, extracted_file_path.c_str(), 0 ) )
		{
			printf(  "�� ������� ������� ����: %s\n", Rep.c_str() );
			return false;
		}

		wprintf( L"\r����������� ������ %d[%d%%]", currentfile, currentfile++ * 100 / file_count );
	}

	wprintf( L"\r����������� ������ %d[100%%]\n", currentfile );

	// ���������� ������ � �������
	mz_zip_reader_end( &zip_archive );

	return true;
}

// ����� ������
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

// ����� ������ �� ����� � �������� �����(��� ��������)
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

// ������� json ������ �� ������
//! �� ����������� ����������
std::unordered_map<std::string, std::string> ParseJson2( const std::string& jsonString )
{
	std::unordered_map<std::string, std::string> result;
	size_t pos = 0;
	while( pos < jsonString.size() )
	{
		// ����� ����
		size_t keyStart = jsonString.find( '"', pos );
		if( keyStart == std::string::npos ) break;
		size_t keyEnd = jsonString.find( '"', keyStart + 1 );
		if( keyEnd == std::string::npos ) break;
		std::string key = jsonString.substr( keyStart + 1, keyEnd - keyStart - 1 );

		// ����� ��������
		size_t valueStart = jsonString.find( '"', keyEnd + 1 );
		if( valueStart == std::string::npos ) break;
		size_t valueEnd = jsonString.find( '"', valueStart + 1 );
		if( valueEnd == std::string::npos ) break;
		std::string value = jsonString.substr( valueStart + 1, valueEnd - valueStart - 1 );

		// �������� ����-�������� � ���������
		result[ key ] = value;

		// �������� �������
		pos = valueEnd + 1;
	}
	return result;
}

std::unordered_map<std::string, std::string> ParseJson( const std::string& jsonString )
{
	std::unordered_map<std::string, std::string> result;
	auto j = nlohmann::json::parse( jsonString );

	for( auto&[ key, value ] : j.items() )
	{
		result[ key ] = value;
	}

	return result;
}

/*#include <nlohmann/json.hpp>
#include <curl/curl.h>



size_t WriteCallback( void* contents, size_t size, size_t nmemb, void* userp )
{
	( ( std::string* )userp )->append( ( char* )contents, size * nmemb );
	return size * nmemb;
}

std::string GetLatestRelease( const std::string& repo )
{
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	curl = curl_easy_init();
	if( curl )
	{
		std::string url = "https://api.github.com/repos/" + repo + "/releases/latest";
		curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteCallback );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &readBuffer );
		curl_easy_setopt( curl, CURLOPT_USERAGENT, "libcurl-agent/1.0" );
		res = curl_easy_perform( curl );
		curl_easy_cleanup( curl );

		if( res != CURLE_OK )
		{
			throw std::runtime_error( "Failed to fetch release info: " + std::string( curl_easy_strerror( res ) ) );
		}
	}

	return readBuffer;
}*/

std::wstring GetAppVersion()
{
	// �������� ���� � ������������ �����
	wchar_t filePath[ MAX_PATH ];
	GetModuleFileName( NULL, filePath, MAX_PATH );

	// �������� ������ ������ ����������
	DWORD handle;
	DWORD size = GetFileVersionInfoSize( filePath, &handle );
	if( size == 0 )
	{
		return L"�� ������� �������� ������ ���������� � ������.";
	}

	// �������� ����� ��� �������� ���������� � ������
	std::vector<char> versionInfo( size );
	if( !GetFileVersionInfo( filePath, handle, size, versionInfo.data() ) )
	{
		return L"�� ������� �������� ���������� � ������.";
	}

	// ��������� ���������� � ������
	VS_FIXEDFILEINFO* fileInfo;
	UINT len;
	if( !VerQueryValueA( versionInfo.data(), "\\", reinterpret_cast< LPVOID* >( &fileInfo ), &len ) )
	{
		return L"�� ������� ������� ���������� � ������.";
	}

	// ����������� ������ � ������
	std::wstring version = std::to_wstring( ( fileInfo->dwFileVersionMS >> 16 ) & 0xffff ) + L"." +
		std::to_wstring( ( fileInfo->dwFileVersionMS >> 0 ) & 0xffff ) + L".";
	/*std::wstring version = std::to_wstring( ( fileInfo->dwFileVersionMS >> 16 ) & 0xffff ) + L"." +
		std::to_wstring( ( fileInfo->dwFileVersionMS >> 0 ) & 0xffff ) + L"." +
		std::to_wstring( ( fileInfo->dwFileVersionLS >> 16 ) & 0xffff ) + L"." +
		std::to_wstring( ( fileInfo->dwFileVersionLS >> 0 ) & 0xffff );*/

	return version;
}

//////////////////////////////////////////////////////////////////////////
// ��������� ������
std::vector<int> splitVersion( const std::string& version )
{
	std::vector<int> parts;
	size_t start = 0;
	size_t end = version.find( '.' );
	while( end != std::string::npos )
	{
		parts.push_back( std::stoi( version.substr( start, end - start ) ) );
		start = end + 1;
		end = version.find( '.', start );
	}
	parts.push_back( std::stoi( version.substr( start ) ) );
	return parts;
}

// @param (std::string) CurrentVersion - ������� ������
// @param (std::string) NewVersion - ����� ������
// @return (bool) true - true �� ����� ������ ������ ��� �������
bool isVersionGreater( const std::string& CurrentVersion, const std::string& NewVersion )
{
	std::vector<int> v1 = splitVersion( CurrentVersion );
	std::vector<int> v2 = splitVersion( NewVersion );

	size_t maxLength = max( v1.size(), v2.size() );
	v1.resize( maxLength, 0 );
	v2.resize( maxLength, 0 );

	for( size_t i = 0; i < maxLength; ++i )
	{
		if( v1[ i ] > v2[ i ] )
		{
			return false;
		}
		else if( v1[ i ] < v2[ i ] )
		{
			return true;
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////

size_t WriteCallback( void* contents, size_t size, size_t nmemb, void* userp )
{
	( ( std::string* )userp )->append( ( char* )contents, size * nmemb );
	return size * nmemb;
}

bool GetLatestRelease( std::string Ver )
{
	CURL* curl;
	CURLcode res;
	std::string releaseInfo;

	// ��������� �������� ����� ������
	curl = curl_easy_init();
	if( curl )
	{
		std::string url = "https://api.github.com/repos/Krinkelss/patcher-v3/releases/latest";
		curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteCallback );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &releaseInfo );
		curl_easy_setopt( curl, CURLOPT_USERAGENT, "libcurl-agent/1.0" );
		res = curl_easy_perform( curl );

		if( res != CURLE_OK )
		{
			throw std::runtime_error( "������ ��������� ����������: " + std::string( curl_easy_strerror( res ) ) );
		}
		else
		{
			long http_code = 0;
			curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &http_code );
			if( http_code == 404 )
			{
				//std::cerr << "Error 404: Not Found" << std::endl;
				//std::cerr << "Response: " << readBuffer << std::endl;

				throw std::runtime_error( "������ 404, ������ �� �������" );
			}
		}
		curl_easy_cleanup( curl );
	}

	// ��������. ������ ������� ������
	nlohmann::json jsonData = nlohmann::json::parse( releaseInfo );
	
	/*if( !jsonData[ key_version ].empty() )
	{
		throw std::runtime_error( "���� " + key_version + " �� ������" );
	}*/

	// ������ ���������� ������
	std::string NewVer = jsonData[ "tag_name" ];

	// ������ ������� ������ ������ � ������ ������� ���������
	if( isVersionGreater( Ver, NewVer ) == true )
	{ // ����� �����������
		// �������� ������ ��� ���������� ������	
		std::string key_url = "browser_download_url";
		// ���������, ���������� �� ���� � ������� assets
		if( jsonData.contains( "assets" ) && !jsonData[ "assets" ].empty() )
		{
			// �������� ������ ������� ������� assets
			nlohmann::json asset = jsonData[ "assets" ][ 0 ];

			// ��������� ������� ����� browser_download_url
			if( asset.contains( key_url ) == false )
			{
				throw std::runtime_error( "���� " + key_url + " �� ������" );
			}


		}
	}
}