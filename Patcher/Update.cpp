/*
* This is an open source non-commercial project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
*/
#include <windows.h>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "Update.h"
#include "Utils.h"
#include "SelectDialog.h"

void downloadFile( const std::string& url, const std::wstring& outputPath );

// Сравнение версий
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

// @param (std::string) CurrentVersion - Текущая версия
// @param (std::string) NewVersion - Новая версия
// @return (bool) true - true то новая версия больше чем текущая
bool isVersionGreater( const std::string& currentVersion, const std::string& newVersion )
{
	std::vector<int> v1 = splitVersion( currentVersion );
	std::vector<int> v2 = splitVersion( newVersion );

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

bool OpenUrlInDefaultBrowser( const std::string& url )
{
	HINSTANCE result = ShellExecuteA( NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL );
	if( ( int )result <= 32 )
	{
		// Обработка ошибки
		MessageBox( NULL, L"Не удалось открыть ссылку в браузере.", L"Ошибка", MB_ICONERROR );
		return false;
	}
	return true;
}

bool CheckRelease( std::string currentVersion )
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
		curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L ); // Обработка перенаправлений
		//curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L ); // Включение отладочной информации
		curl_easy_setopt( curl, CURLOPT_TIMEOUT, 30L ); // Установка таймаута
		curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, 30L ); // Установка таймаута на подключение	
		curl_easy_setopt( curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE ); // Отключение проверки отзыва сертификатов
		res = curl_easy_perform( curl );

		if( res != CURLE_OK )
		{
			curl_easy_cleanup( curl );
			curl_global_cleanup();
			throw std::runtime_error( "Ошибка получения обновления: " + std::string( curl_easy_strerror( res ) ) );
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
				throw std::runtime_error( "Ошибка 404, ничего не найдено" );
			}
		}
	}
	else
	{
		curl_global_cleanup();
		throw std::runtime_error( "Ошибка инициализации CURL" );
	}

	curl_easy_cleanup( curl );
	curl_global_cleanup();

	// Получили.
	nlohmann::json jsonData = nlohmann::json::parse( releaseInfo );

	// Версия последнего релиза
	//std::string NewVer = jsonData[ "tag_name" ];
	std::string NewVer = jsonData.value( "tag_name", "" );

	//! Не забыть вернуть true
	if( isVersionGreater( currentVersion, NewVer ) == true )
	{// Можно обновляться
		std::string Buf = "Доступна новая версия программы. Скачать?\n\nТекущая версия: " + currentVersion + "\nДоступная версия: " + NewVer;

		if( MessageBox( nullptr, AnsiToUnicode( Buf ).c_str(), L"Доступно обновление", MB_YESNO ) == IDYES )
		{
			// Получаем ссылку для скачивания релиза	
			std::string key_url = "browser_download_url";
			// Проверяем, существует ли ключ в массиве assets
			if( jsonData.contains( "assets" ) && !jsonData[ "assets" ].empty() )			
			{
				// Получаем первый элемент массива assets
				nlohmann::json asset = jsonData[ "assets" ][ 0 ];

				// Проверяем наличие ключа browser_download_url
				if( asset.contains( key_url ) == false )
				{
					throw std::runtime_error( "Ключ " + key_url + " не найден" );
				}

				// Ссылка на скачивание
				std::string Url = asset[ key_url ];

				// Попробуем скачать наш архив.
				// Выберем куда сохранять файл
				/*SelectDialog Dialog;

				if( FAILED( Dialog.SaveFileDialog( L"Выберете папку для сохранения архива" ) ) )
				{
					return false;
				}
								
				try
				{
					downloadFile( Url, Dialog.GetResult() );
				}
				catch( const std::runtime_error& e )
				{
					MessageBox( NULL, AnsiToUnicode( e.what() ).c_str(), L"Ошибка при попытка скачать файл", MB_OK | MB_ICONERROR );
					return 0;
				}*/

				if( OpenUrlInDefaultBrowser( Url ) == false )
					return false;
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
			wprintf( L"\rСкачивание файла: %d%%", percentage );
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
		throw std::runtime_error( "Ошибка при создании файла архива" );

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
		curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L ); // Обработка перенаправлений
		//curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L ); // Включение отладочной информации
		curl_easy_setopt( curl, CURLOPT_TIMEOUT, 30L ); // Установка таймаута
		curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, 30L ); // Установка таймаута на подключение	
		//curl_easy_setopt( curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE ); // Отключение проверки отзыва сертификатов
		//curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
		//curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
		
		wprintf( L"Папытка скачать файл\n" );

		res = curl_easy_perform( curl );
		if( res != CURLE_OK )
		{			
			curl_easy_cleanup( curl );
			curl_global_cleanup();
			fclose( fp );
			throw std::runtime_error( "Ошибка curl_easy_perform(): " + std::string( curl_easy_strerror( res ) ) );
		}
	}
	else
	{
		curl_global_cleanup();
		fclose( fp );
		throw std::runtime_error( "Ошибка инициализации CURL" );
	}

	wprintf( L"\nФайл скачан\n" );

	curl_easy_cleanup( curl );
	curl_global_cleanup();
	fclose( fp );
}
//////////////////////////////////////////////////////////////////////////