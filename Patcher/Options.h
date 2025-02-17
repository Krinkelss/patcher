#pragma once
#include <windows.h>
#include <vector>
#include <filesystem>

constexpr uint32_t MD5_OFF = 0;				// Выкл
constexpr uint32_t MD5_ALL = 1;				// Проверка всех файлов
constexpr uint32_t MD5_ACS = 2;				// Проверить только Assembly-CSharp.dll

constexpr uint32_t PRINT_CONSOLE_MIN = 0;	// Выводить основную информацию в консоль
constexpr uint32_t PRINT_CONSOLE_MAX = 1;	// Выводить всю информацию в консоль

constexpr uint32_t UPDATE_OFF = 0;			// Выключить проверку обновления
constexpr uint32_t UPDATE_ON = 1;			// Включить проверку обновления


class Options
{
public:
	Options() = default;
	~Options() = default;

	bool get_arguments( int argc, char* argv[] )
	{
		if( argc < 1 && argc % 2 != 0 )
		{
			wprintf( L"Некорректное число аргументов командной строки\n" );
			return false;
		}
		if( argc == 2 && ( lstrcmpiA( argv[ 1 ], "-h" ) == 0 || lstrcmpiA( argv[ 1 ], "-help" ) == 0 ) )
			PrintHelp();
				
		std::vector<std::string> args = std::vector<std::string>();
		for( int i = 1; i < argc; i++ )
			args.emplace_back( argv[ i ] );

		return parse_arguments( args );
	}


private:
	bool parse_arguments( const std::vector<std::string>& args )
	{
		for( int i = 0; i < args.size(); ++i )
		{
			if( args[ i ] == "-md5" )
			{
				if( args[ i + 1 ] == "md5_off" )
					md5_check = MD5_OFF;
				else
				if( args[ i + 1 ] == "md5_all" )
					md5_check = MD5_ALL;
				else
				{
					wprintf( L"Неизвестный параметр: [%s]\n", AnsiToUnicode( args[ i + 1 ] ).c_str() );					
					return false;
				}
				++i;
			}

			if( args[ i ] == "-p" )
			{
				if( args[ i + 1 ] == "print_max" )
					print_console = PRINT_CONSOLE_MAX;
				else
				{
					wprintf( L"Неизвестный параметр: [%s]\n", AnsiToUnicode( args[ i + 1 ] ).c_str() );
					return false;
				}
				++i;
			}

			if( args[ i ] == "-update" )
			{
				if( args[ i + 1 ] == "off" )
					check_update = UPDATE_OFF;
				else
				{
					wprintf( L"Неизвестный параметр: [%s]\n", AnsiToUnicode( args[ i + 1 ] ).c_str() );
					return false;
				}
				++i;
			}			
		}

		return true;
	}

	void PrintHelp()
	{
		wprintf( L"Как пользоваться: \"Patcher.exe\" <опции>\n" );
		wprintf( L"\n" );
		wprintf( L"Опции:\n" );
		wprintf( L"-h, -help Печать справки\n" );
		wprintf( L"\n" );
		wprintf( L"-md5 Проверять хэши файлов после обработки. По умолчанию включена проверка только Assembly-CSharp.dll\n" );
		wprintf( L"	md5_off Не проверять хэши\n" );
		wprintf( L"	md5_all Проверять все файлы\n" );
		wprintf( L"\n" );
		wprintf( L"-p Вывод в консоль\n" );
		wprintf( L"	print_max - Вывод в консоль расширенной информации\n" );
		wprintf( L"\n" );
		wprintf( L"-u Отключить автоматическую проверку новых версий\n" );		
		wprintf( L"	off - Выключить автоматическое обновление\n" );
		wprintf( L"\n" );
		wprintf( L"Пример: Patcher.exe -md5 md5_all -p print_max -u off\n" );

	}

public:
	uint32_t md5_check = MD5_ACS;					// По умолчанию будет проверка только Assembly-CSharp.dll
	uint32_t print_console = PRINT_CONSOLE_MIN;		// Вывод информации в консоль. По умолчанию минимальная
	uint32_t check_update = UPDATE_ON;				// Проверка наличия обновления 
};