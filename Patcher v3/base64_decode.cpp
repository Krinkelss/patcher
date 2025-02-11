#include "base64_decode.h"

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static inline bool is_base64( unsigned char c )
{
	return ( isalnum( c ) || ( c == '+' ) || ( c == '/' ) );
}

std::string base64_decode( std::string const& encoded_string )
{
	size_t in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[ 4 ], char_array_3[ 3 ];
	std::string ret;

	while( in_len-- && ( encoded_string[ in_ ] != '=' ) && is_base64( encoded_string[ in_ ] ) )
	{
		char_array_4[ i++ ] = encoded_string[ in_ ]; in_++;
		if( i == 4 )
		{
			for( i = 0; i < 4; i++ )
				char_array_4[ i ] = base64_chars.find( char_array_4[ i ] );

			char_array_3[ 0 ] = ( char_array_4[ 0 ] << 2 ) + ( ( char_array_4[ 1 ] & 0x30 ) >> 4 );
			char_array_3[ 1 ] = ( ( char_array_4[ 1 ] & 0xf ) << 4 ) + ( ( char_array_4[ 2 ] & 0x3c ) >> 2 );
			char_array_3[ 2 ] = ( ( char_array_4[ 2 ] & 0x3 ) << 6 ) + char_array_4[ 3 ];

			for( i = 0; ( i < 3 ); i++ )
				ret += char_array_3[ i ];
			i = 0;
		}
	}

	if( i )
	{
		for( j = i; j < 4; j++ )
			char_array_4[ j ] = 0;

		for( j = 0; j < 4; j++ )
			char_array_4[ j ] = base64_chars.find( char_array_4[ j ] );

		char_array_3[ 0 ] = ( char_array_4[ 0 ] << 2 ) + ( ( char_array_4[ 1 ] & 0x30 ) >> 4 );
		char_array_3[ 1 ] = ( ( char_array_4[ 1 ] & 0xf ) << 4 ) + ( ( char_array_4[ 2 ] & 0x3c ) >> 2 );
		char_array_3[ 2 ] = ( ( char_array_4[ 2 ] & 0x3 ) << 6 ) + char_array_4[ 3 ];

		for( j = 0; ( j < i - 1 ); j++ ) ret += char_array_3[ j ];
	}

	return binToHex( ret.c_str(), ret.size() );
}

//Из названия понятно что выполняет функция. Для MD5 хэша
std::string binToHex( const char* data, size_t length )
{
	const char hexDigits[] = "0123456789ABCDEF";
	std::string hexString;
	hexString.reserve( length * 2 );

	for( size_t i = 0; i < length; ++i )
	{
		uint8_t byte = data[ i ];
		hexString.push_back( hexDigits[ byte >> 4 ] );
		hexString.push_back( hexDigits[ byte & 0x0F ] );
	}

	return hexString;
}

/*std::string binToHex( const std::string &data )
{
	std::string hex_output;
	const char hex_chars[] = "0123456789abcdef"; // Символы для шестнадцатеричного представления

	for( unsigned char byte : data )
	{
		// Получаем старший и младший нибблы
		char high_nibble = hex_chars[ ( byte >> 4 ) & 0x0F ];
		char low_nibble = hex_chars[ byte & 0x0F ];

		// Добавляем символы в результат
		hex_output.push_back( high_nibble );
		hex_output.push_back( low_nibble );
	}
	return hex_output;
}*/