#include "ReadWriteFile.h"
#include "md5.h"

class WindowsException
{
public:
	WindowsException( const std::wstring& message, DWORD errorCode ) : message( message ), errorCode( errorCode )
	{}

	std::wstring getMessage() const
	{
		return message;
	}

	DWORD getErrorCode() const
	{
		return errorCode;
	}

private:
	std::wstring message;
	DWORD errorCode;
};

ReadWrite::ReadWrite()
{}

ReadWrite::~ReadWrite()
{}

void ReadWrite::Apply( FileMapper *ToPatchFile, FileMapper *ToFinishFile, FileMapper *ToPatch )
{
	uint8_t b;
	int64_t start, length;
	long soFar = 0;
	int readBufferSize = 4 * 1024 * 1024;
	byte *TmpBuf;

	//cursor = static_cast< char* >( ToPatch.GetView() );
	fileSize = ToPatch->GetFileSize();
	PatchData = ( char * )ToPatch->GetView();

	// Для начала проверим заголовок файла, а вдруг это не он
	std::string header = ReadString( 9 );
	if( header.compare( "FRSNCDLTA" ) != 0 )
		throw WindowsException( L"Некорректный файл", 0 );

	// Теперь проверим версию?
	uint8_t version = ReadByte();
	if( version != 0x01 )
		throw WindowsException( L"Некорректный байт", 0 );

	// Может я и ошибаюсь, но, возможно, это размер json строки что будет далее
	uint8_t StrLength = ReadByte();

	// Опять такой же байт. Пока оставлю так, потом посмотрим
	version = ReadByte();
	if( version != 0x01 )
		throw WindowsException( L"Некорректный байт 2", 0 );

	// Читаем json данные
	std::string metadataStr = ReadString( StrLength );

	auto hash = ConstexprHashes::md5( ToPatch->GetView(), fileSize );

	for( size_t i = 0; i < 16; i++ )
	{
		printf( "%02x", hash[ i ] & 0xff );
	}

	/*std::unordered_map<std::string, std::string> parsedJson = ParseJson( metadataStr );

	std::string HashAlgorithm = parsedJson[ "hashAlgorithm" ];
	std::string ExpectedFileHash = parsedJson[ "expectedFileHash" ];

	auto expectedHash = base64_decode( ExpectedFileHash );*/

//	char* ToPatchFileData = static_cast< char* >( ToPatchFile.GetView() );
//	char* ToFinishFileData = static_cast< char* >( ToFinishFile.GetView() );
//	std::vector<uint8_t> writeData;
//	std::vector<uint8_t> bytes;
	int RB = 0;	// Байты для чтения из файла с патчем

	//! Переделать выделение памяти
	//! Переделать while( *PatchData != 0 )
	while( *PatchData != 0 )
	{
		b = ReadByte();
		if( b == CopyCommand )
		{
			start = ( int64_t )Read<int64_t>();
			length = ( int64_t )Read<int64_t>();

			// Читаем данные из оригинального файла
			TmpBuf = ( byte * )malloc( length );
			ToPatchFile->ReadData( TmpBuf, start, length );
			ToFinishFile->WriteData( TmpBuf, length );
			free( TmpBuf );
			TmpBuf = NULL;
		}
		else
		if( b == DataCommand )
		{
			length = ( int64_t )Read<int64_t>();
			soFar = 0;
			while( soFar < length )
			{
				RB = min( static_cast< int64_t >( length - soFar ), static_cast< int64_t >( readBufferSize ) );
				TmpBuf = ( byte * )malloc( RB );
				ReadBytes( TmpBuf, RB );
				ToFinishFile->WriteData( TmpBuf, length );
				free( TmpBuf );
				TmpBuf = NULL;
				soFar += RB;
			}			
		}
	}
}

template <typename T>
T ReadWrite::Read()
{
	T value = *reinterpret_cast< T* >( PatchData );
	PatchData += sizeof( T );
	return value;
}

uint8_t ReadWrite::ReadByte()
{
	uint8_t value = *reinterpret_cast< uint8_t* >( PatchData );
	PatchData += sizeof( uint8_t );
	return value;
}

std::string ReadWrite::ReadString( size_t length )
{
	std::string bytes( PatchData, length );
	PatchData += length;
	return bytes;
}

void ReadWrite::ReadBytes( byte *Data, size_t length )
{
	if( PatchData + length > PatchData + fileSize )
		throw WindowsException( L"Смещение больше чем файл патча", GetLastError() );

	memcpy( Data, PatchData, length );
	PatchData += length;
}

//////////////////////////////////////////////////////////////////////////

std::unordered_map<std::string, std::string> ReadWrite::ParseJson( const std::string& jsonString )
{
	std::unordered_map<std::string, std::string> result;
	size_t pos = 0;
	while( pos < jsonString.size() )
	{
		// Найти ключ
		size_t keyStart = jsonString.find( '"', pos );
		if( keyStart == std::string::npos ) break;
		size_t keyEnd = jsonString.find( '"', keyStart + 1 );
		if( keyEnd == std::string::npos ) break;
		std::string key = jsonString.substr( keyStart + 1, keyEnd - keyStart - 1 );

		// Найти значение
		size_t valueStart = jsonString.find( '"', keyEnd + 1 );
		if( valueStart == std::string::npos ) break;
		size_t valueEnd = jsonString.find( '"', valueStart + 1 );
		if( valueEnd == std::string::npos ) break;
		std::string value = jsonString.substr( valueStart + 1, valueEnd - valueStart - 1 );

		// Добавить ключ-значение в результат
		result[ key ] = value;

		// Обновить позицию
		pos = valueEnd + 1;
	}
	return result;
}