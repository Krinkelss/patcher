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
			if( args[ i ] == "-h" || args[ i ] == "-help" )
			{
				PrintHelp();
				return true;
			}

			if( args[ i ] == "-hash0" )
				md5_check = MD5_OFF;

			if( args[ i ] == "-hash1" )
				md5_check = MD5_ALL;

			if( args[ i ] == "-v" )
				print_console = PRINT_CONSOLE_MAX;

			if( args[ i ] == "-nocheck" )
				check_update = UPDATE_OFF;
		}	

		return true;
	}

	void PrintHelp()
	{
		// Дизайн консоли: Skymmer
		wprintf( L"Как пользоваться: \"Patcher.exe\" <опции>\n" );
		wprintf( L"\n" );
		wprintf( L"Опции:\n" );
		wprintf( L" -h, -help	Вывод справки\n" );
		wprintf( L" -hash{0|1}	Проверять хэши файлов после обработки.\n" );
		wprintf( L"		0 - Не проверять файлы, 1 - Проверять все файлы\n" );
		wprintf( L"		По умолчанию включена проверка только Assembly-CSharp.dll\n" );
		wprintf( L" -v		Вывод в консоль расширенной информации\n" );
		wprintf( L" -nocheck	Отключить автоматическую проверку новых версий\n" );
		wprintf( L"\n" );
		wprintf( L"Пример: Patcher.exe -hash1 -v -nocheck\n" );

	}

public:
	uint32_t md5_check = MD5_ACS;					// По умолчанию будет проверка только Assembly-CSharp.dll
	uint32_t print_console = PRINT_CONSOLE_MIN;		// Вывод информации в консоль. По умолчанию минимальная
	uint32_t check_update = UPDATE_ON;				// Проверка наличия обновления 
};