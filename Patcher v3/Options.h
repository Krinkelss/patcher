#pragma once
#include <windows.h>
#include <vector>
#include <filesystem>

constexpr uint32_t MD5_OFF = 0; // ����
constexpr uint32_t MD5_ALL = 1; // �������� ���� ������
constexpr uint32_t MD5_ACS = 2; // ��������� ������ Assembly-CSharp.dll

constexpr uint32_t PRINT_CONSOLE_MIN = 0;	// �������� �������� ���������� � �������
constexpr uint32_t PRINT_CONSOLE_MAX = 1;	// �������� ��� ���������� � �������


class Options
{
public:
	Options() = default;
	~Options() = default;

	bool get_arguments( int argc, char* argv[] )
	{
		if( argc < 1 && argc % 2 != 0 )
		{
			wprintf( L"������������ ����� ���������� ��������� ������\n" );
			return false;
		}

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
				if( args[ i + 1 ] == "md5_acs" )
					md5_check = MD5_ACS;
				else
				{
					wprintf( L"����������� ��������: [%s]\n", AnsiToUnicode( args[ i + 1 ] ).c_str() );					
					return false;
				}
				++i;
			}

			if( args[ i ] == "-p" )
			{
				if( args[ i + 1 ] == "print_min" )
					print_console = PRINT_CONSOLE_MIN;
				else
				if( args[ i + 1 ] == "print_max" )
					print_console = PRINT_CONSOLE_MAX;
				else
				{
					wprintf( L"����������� ��������: [%s]\n", AnsiToUnicode( args[ i + 1 ] ).c_str() );
					return false;
				}
				++i;
			}
		}

		return true;
	}

public:
	uint32_t md5_check = MD5_ACS;					// �� ��������� ����� �������� ������ Assembly-CSharp.dll
	uint32_t print_console = PRINT_CONSOLE_MIN;		// ����� ���������� � �������. �� ��������� �����������
};