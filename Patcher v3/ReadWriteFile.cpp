#include "ReadWriteFile.h"
#include "base64_decode.h"
#include "md5.h"

ReadWrite::ReadWrite()
{}

ReadWrite::~ReadWrite()
{}

void ReadWrite::Apply( FileMapper *ToPatchFile, FileMapper *ToFinishFile, FileMapper *ToPatch, DWORD MD5Check )
{
	uint8_t b;
	int64_t start, length;
	long soFar = 0;
	int readBufferSize = 4 * 1024 * 1024;
	byte *TmpBuf;


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
		throw std::runtime_error( "Смещение больше чем файл патча" );

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