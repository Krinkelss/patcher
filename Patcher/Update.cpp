#include <windows.h>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "Update.h"
#include "Utils.h"
#include "SelectDialog.h"

void downloadFile( const std::string& url, const std::wstring& outputPath );

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

size_t WriteCallback( void* contents, size_t size, size_t nmemb, void* userp )
{
	( ( std::string* )userp )->append( ( char* )contents, size * nmemb );
	return size * nmemb;
}

bool CheckRelease( std::string Ver )
{
	CURL* curl;
	CURLcode res;
	std::string releaseInfo;

	curl_global_init( CURL_GLOBAL_DEFAULT );
	curl = curl_easy_init();
	if( curl )
	{
		std::string url = "https://api.github.com/repos/Krinkelss/patcher/releases/latest";
		curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteCallback );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &releaseInfo );
		curl_easy_setopt( curl, CURLOPT_USERAGENT, "libcurl-agent/1.0" );
		curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L ); // ��������� ���������������
		//curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L ); // ��������� ���������� ����������
		curl_easy_setopt( curl, CURLOPT_TIMEOUT, 20L ); // ��������� ��������
		curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, 20L ); // ��������� �������� �� �����������	
		curl_easy_setopt( curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE ); // ���������� �������� ������ ������������
		res = curl_easy_perform( curl );

		if( res != CURLE_OK )
		{
			curl_easy_cleanup( curl );
			curl_global_cleanup();
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
				curl_easy_cleanup( curl );
				curl_global_cleanup();
				throw std::runtime_error( "������ 404, ������ �� �������" );
			}
		}
	}
	else
	{
		curl_global_cleanup();
		throw std::runtime_error( "������ ������������� CURL" );
	}

	curl_easy_cleanup( curl );
	curl_global_cleanup();

	// ��������.
	nlohmann::json jsonData = nlohmann::json::parse( releaseInfo );

	// ������ ���������� ������
	std::string NewVer = jsonData[ "tag_name" ];

	//! �� ������ ������� true
	if( isVersionGreater( Ver, NewVer ) == true )
	{// ����� �����������
		std::string Buf = "�������� ����� ������ ���������. �������?\n\n������� ������: " + Ver + "\n��������� ������: " + NewVer;

		if( MessageBox( nullptr, AnsiToUnicode( Buf ).c_str(), L"�������� ����������", MB_YESNO ) == IDYES )
		{
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

				// ������ �� ����������
				std::string Url = asset[ key_url ];

				// ��������� ������� ��� �����.
				// ������� ���� ��������� ����
				SelectDialog Dialog;

				if( FAILED( Dialog.SaveFileDialog( L"�������� ����� ��� ���������� ������" ) ) )
				{
					return false;
				}

				downloadFile( Url, Dialog.GetResult() );
			}
			return true;
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
size_t WriteDownloadCallback( void* contents, size_t size, size_t nmemb, void* userp )
{
	FILE* fp = static_cast< FILE* >( userp );
	return fwrite( contents, size, nmemb, fp );
}

int ProgressDownloadCallback( void* ptr, curl_off_t totalToDownload, curl_off_t nowDownloaded, curl_off_t, curl_off_t )
{
	if( totalToDownload > 0 )
	{
		int percentage = static_cast< int >( ( nowDownloaded * 100 ) / totalToDownload );
		//std::cout << "\rProgress: " << percentage << "%" << std::flush;
		if( percentage >= 0 && percentage <= 100 )
			wprintf( L"\r���������� �����: %d%%", percentage );
	}
	return 0;
}

void downloadFile( const std::string& url, const std::wstring& outputPath )
{
	CURL* curl;
	CURLcode res;
	FILE* fp = nullptr;
	errno_t err = _wfopen_s( &fp, outputPath.c_str(), L"wb" );
	if( err != 0 )
		throw std::runtime_error( "������ ��� �������� ����� ������" );

	curl_global_init( CURL_GLOBAL_DEFAULT );
	curl = curl_easy_init();
	if( curl )
	{
		curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
		curl_easy_setopt( curl, CURLOPT_USERAGENT, "libcurl-agent/1.0" );
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteDownloadCallback );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, fp );
		curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 0L );
		curl_easy_setopt( curl, CURLOPT_XFERINFOFUNCTION, ProgressDownloadCallback );
		curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L ); // ��������� ���������������
		//curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L ); // ��������� ���������� ����������
		curl_easy_setopt( curl, CURLOPT_TIMEOUT, 20L ); // ��������� ��������
		curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, 20L ); // ��������� �������� �� �����������	
		curl_easy_setopt( curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE ); // ���������� �������� ������ ������������
		
		wprintf( L"������� ������� ����\n" );

		res = curl_easy_perform( curl );
		if( res != CURLE_OK )
		{			
			curl_easy_cleanup( curl );
			curl_global_cleanup();
			fclose( fp );
			throw std::runtime_error( "������ curl_easy_perform(): " + std::string( curl_easy_strerror( res ) ) );
		}
	}
	else
	{
		curl_global_cleanup();
		fclose( fp );
		throw std::runtime_error( "������ ������������� CURL" );
	}

	wprintf( L"\n���� ������\n" );

	curl_easy_cleanup( curl );
	curl_global_cleanup();
	fclose( fp );
}
//////////////////////////////////////////////////////////////////////////