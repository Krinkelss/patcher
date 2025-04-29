#pragma once
#include <string>
#include <unordered_map>

std::wstring GetFileVersion( const std::wstring& filePath );
std::wstring ExtractVersion( const std::wstring& fileName );
std::string wstring_to_string( const std::wstring& wstr );
std::wstring AnsiToUnicode( const std::string& ansi );
bool ExtractZipArchive( std::wstring zipFilePath, std::wstring extractToPath );
std::vector<std::wstring> SearchAllFilesInDirectory( const std::wstring& directory );
std::vector<std::wstring> SearchFilesInDirectory( const std::wstring& directory, const std::wstring& filePattern );
std::unordered_map<std::string, std::string> ParseJson( const std::string& jsonString );
std::wstring GetAppVersion();

class FILEWrapper
{
public:
	FILEWrapper( const char *filePath, bool writeAccess )
	{
		DWORD access = GENERIC_READ;
		DWORD creationDisposition = OPEN_EXISTING;

		if( writeAccess == true )
		{
			access = GENERIC_WRITE;
			creationDisposition = CREATE_ALWAYS;
		}
		
		hFile = CreateFileA( filePath, access, 0, NULL, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile == INVALID_HANDLE_VALUE )
		{
			if( GetLastError() == ERROR_SHARING_VIOLATION )
			{
				throw std::runtime_error( "Файл \n" + std::string( filePath ) + "\nзаблокирован другим процессом" );
			}

			throw std::runtime_error( "Ошибка при открытии файла\n" + *filePath );
		}

	}

	~FILEWrapper()
	{
		if( hFile != INVALID_HANDLE_VALUE )
		{
			CloseHandle( hFile );
			hFile = nullptr;
		}
	}

public:
	DWORD readfile( void *buffer, uint32_t ReadLen, const char *ErrorText )
	{
		DWORD bytesRead;
		if( ReadFile( hFile, buffer, ReadLen, &bytesRead, NULL ) != FALSE && bytesRead > 0 )
		{
			if( ReadLen != bytesRead )
				throw std::runtime_error( ErrorText );
		}

		return bytesRead;
	}

	void writefile( void *buffer, uint32_t WriteLen, const char *ErrorText )
	{
		DWORD bytesWrite;
		if( WriteFile( hFile, buffer, WriteLen, &bytesWrite, NULL ) != FALSE && bytesWrite > 0 )
		{
			if( WriteLen != bytesWrite )
				throw std::runtime_error( ErrorText );
		}
	}

	// Функция для чтения 7-битного сжатого целого числа
	uint32_t read_7bit_encoded_int( void )
	{
		uint32_t result = 0;
		int shift = 0;
		char buffer;
		DWORD bytesRead;
		int byte;

		while( 1 )
		{
			/*int byte = fgetc( file ); // Читаем один байт
			if( byte == EOF )
			{
				throw std::runtime_error( "Неожиданный конец файла при чтенаа 7-битного закодированного числа" );
			}*/

			if( ReadFile( hFile, &buffer, 1, &bytesRead, NULL ) != FALSE && bytesRead == 1 )
			{
				byte = static_cast< unsigned char >( buffer );
				// Извлекаем 7 бит данных
				result |= ( byte & 0x7F ) << shift;

				// Проверяем старший бит (8-й бит)
				if( ( byte & 0x80 ) == 0 )
				{
					break; // Если старший бит равен 0, это последний байт числа
				}

				shift += 7;

				// Проверка на переполнение
				if( shift >= 32 )
				{
					throw std::runtime_error( "7-битное закодированное число слишком велико" );
				}
			}
			else
				throw std::runtime_error( "Ошибка чтения 7-битного закодированного числа" );
		}
			

		return result;
	}

	uint32_t getint( const char *ErrorText )
	{
		char byteRead;
		DWORD bytesRead;

		if( ReadFile( hFile, &byteRead, sizeof( byteRead ), &bytesRead, NULL ) != FALSE && bytesRead == sizeof( byteRead ) )
			return static_cast< unsigned char >( byteRead );
		else
			return 0;
	}

	void fseekfile(uint32_t offset, const char *ErrorText )
	{
		DWORD result = SetFilePointer( hFile, offset, NULL, FILE_BEGIN );
		if( result == INVALID_SET_FILE_POINTER )
		{
			if( GetLastError() != NO_ERROR )
			{
				throw std::runtime_error( ErrorText );
			}
		}
	}

	HANDLE get( void )
	{
		return hFile;
	}

private:
	HANDLE hFile;
};