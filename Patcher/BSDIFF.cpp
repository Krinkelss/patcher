#include <windows.h>
#include <filesystem>
#include "bzlib.h"
#include "Utils.h"

typedef struct
{
	uint8_t   *data;
	size_t  size;
} Buffer;

void free_buffer( Buffer *buffer )
{
	free( buffer->data );

	buffer->data = NULL;
	buffer->size = 0;
}

static int decompress_bzip2( const uint8_t *compressed, size_t compressed_size, Buffer *output )
{
	size_t max_bz2_size = ( size_t )UINT_MAX;

	if( compressed_size > max_bz2_size )
	{
		fprintf( stderr, "decompress_bzip2: BSDIFF4 compressed block is too large\n" );
		return 0;
	}

	if( compressed_size > ( SIZE_MAX - 1024 ) / 2 )
	{
		fprintf( stderr, "decompress_bzip2: Size overflow\n" );
		return 0;
	}

	size_t output_size = compressed_size * 2 + 1024;

	if( output_size > max_bz2_size )
		output_size = max_bz2_size;

	if( output_size == 0 )
		output_size = 1;

	uint8_t *data = ( uint8_t * )malloc( output_size );

	if( data == NULL )
	{
		fprintf( stderr, "decompress_bzip2: Out of memory\n" );
		return 0;
	}

	for( ;;)
	{
		unsigned int source_len;
		unsigned int dest_len;
		int result;

		source_len = ( unsigned int )compressed_size;
		dest_len = ( unsigned int )output_size;

		result = BZ2_bzBuffToBuffDecompress( ( char * )data, &dest_len, ( char * )compressed, source_len, 0, 0 );

		if( result == BZ_OK )
		{
			/*
				Распаковка завершена.
			*/
			output->data = data;
			output->size = ( size_t )dest_len;

			return 1;
		}

		if( result != BZ_OUTBUFF_FULL )
		{
			free( data );
			fprintf( stderr, "decompress_bzip2: Invalid bzip2 data in BSDIFF4 patch\n" );

			return 0;
		}

		/*
			Буфер недостаточен.

			Увеличиваем его в два раза.
		*/
		if( output_size >= max_bz2_size / 2 )
		{
			free( data );
			fprintf( stderr, "decompress_bzip2:BSDIFF4 decompressed block is too large\n" );

			return 0;
		}

		output_size *= 2;

		{
			uint8_t *new_data;

			new_data = ( uint8_t * )realloc( data, output_size );

			if( new_data == NULL )
			{
				free( data );
				fprintf( stderr, "decompress_bzip2: realloc, Out of memory\n" );

				return 0;
			}

			data = new_data;
		}
	}


}

static int64_t read_int64( const uint8_t *p, int *ok )
{
	uint64_t value;

	value =
		( ( uint64_t )p[ 0 ] )
		| ( ( uint64_t )p[ 1 ] << 8 )
		| ( ( uint64_t )p[ 2 ] << 16 )
		| ( ( uint64_t )p[ 3 ] << 24 )
		| ( ( uint64_t )p[ 4 ] << 32 )
		| ( ( uint64_t )p[ 5 ] << 40 )
		| ( ( uint64_t )p[ 6 ] << 48 )
		| ( ( uint64_t )( p[ 7 ] & 0x7f ) << 56 );

	*ok = 1;

	if( p[ 7 ] & 0x80 )
	{
		if( value == ( ( uint64_t )1 << 63 ) )
			return INT64_MIN;

		return -( int64_t )value;
	}

	if( value > ( uint64_t )INT64_MAX )
	{
		*ok = 0;
		return 0;
	}

	return ( int64_t )value;
}

static int range_fits( uint64_t offset, uint64_t size, size_t buffer_size )
{
	uint64_t u_offset;
	uint64_t u_size;
	uint64_t u_buffer;

	if( offset < 0 || size < 0 )
		return 0;

	u_offset = ( uint64_t )offset;
	u_size = ( uint64_t )size;
	u_buffer = ( uint64_t )buffer_size;

	if( u_offset > u_buffer )
		return 0;

	return u_size <= u_buffer - u_offset;
}

static int apply_patch( const Buffer *old_data, const Buffer *patch_data, Buffer *new_data )
{
	int ok;

	uint64_t control_compressed_size;
	uint64_t diff_compressed_size;
	uint64_t new_size;

	size_t control_offset;
	size_t control_size;

	size_t diff_offset;
	size_t diff_size;

	size_t extra_offset;
	size_t extra_size;

	Buffer control_block;
	Buffer diff_block;
	Buffer extra_block;

	uint64_t old_pos;
	uint64_t new_pos;

	size_t control_pos;
	size_t diff_pos;
	size_t extra_pos;

	const uint8_t magic[] = "BSDIFF40";

	control_block.data = NULL;
	control_block.size = 0;

	diff_block.data = NULL;
	diff_block.size = 0;

	extra_block.data = NULL;
	extra_block.size = 0;

	new_data->data = NULL;
	new_data->size = 0;

	/* --------------------------------------------------------------------- */
	/* 1. Минимальный размер header                                          */
	/* --------------------------------------------------------------------- */
	if( patch_data->size < 32 )
	{
		fprintf( stderr, "apply_patch: BSDIFF4 patch is too small\n" );
		return 0;
	}

	//! Переделать
	/* --------------------------------------------------------------------- */
	/* 2. Magic                                                              */
	/* --------------------------------------------------------------------- */
	if( memcmp( patch_data->data, magic, 8 ) != 0 )
	{
		fprintf( stderr, "apply_patch: Not a BSDIFF40 patch\n" );
		return 0;
	}

	/* --------------------------------------------------------------------- */
	/* 3. Header                                                             */
	/* --------------------------------------------------------------------- */
	control_compressed_size = read_int64( patch_data->data + 8, &ok );

	if( ok == 0 )
	{
		fprintf( stderr, "apply_patch: Invalid BSDIFF4 control block size\n" );
		return 0;
	}

	diff_compressed_size = read_int64( patch_data->data + 16, &ok );

	if( ok == 0 )
	{
		fprintf( stderr, "apply_patch: Invalid BSDIFF4 diff block size\n" );
		return 0;
	}

	new_size = read_int64( patch_data->data + 24, &ok );

	if( ok == 0 )
	{
		fprintf( stderr, "apply_patch: Invalid BSDIFF4 resulting file size\n" );
		return 0;
	}

	/* --------------------------------------------------------------------- */
	/* 4. Проверки размеров                                                  */
	/* --------------------------------------------------------------------- */

	if( control_compressed_size < 0 )
	{
		fprintf( stderr, "apply_patch: Invalid BSDIFF4 control block size\n" );
		return 0;
	}

	if( diff_compressed_size < 0 )
	{
		fprintf( stderr, "apply_patch: Invalid BSDIFF4 diff block size\n" );
		return 0;
	}

	if( new_size < 0 )
	{
		fprintf( stderr, "apply_patch: Invalid BSDIFF4 new file size\n" );
		return 0;
	}

	if( ( uint64_t )new_size > ( uint64_t )SIZE_MAX )
	{
		fprintf( stderr, "apply_patch: BSDIFF4 resulting file is too large\n" );
		return 0;
	}

	/* --------------------------------------------------------------------- */
	/* 5. Расположение compressed blocks                                     */
	/* --------------------------------------------------------------------- */

	control_offset = 32;
	control_size = ( size_t )control_compressed_size;

	if( control_size > patch_data->size - control_offset )
	{
		fprintf( stderr, "apply_patch: BSDIFF4 control block exceeds patch size\n" );
		return 0;
	}

	diff_offset = control_offset + control_size;
	diff_size = ( size_t )diff_compressed_size;

	if( diff_size > patch_data->size - diff_offset )
	{
		fprintf( stderr, "apply_patch: BSDIFF4 diff block exceeds patch size\n" );
		return 0;
	}

	extra_offset = diff_offset + diff_size;
	extra_size = patch_data->size - extra_offset;

	/* --------------------------------------------------------------------- */
	/* 6. Распаковка                                                          */
	/* --------------------------------------------------------------------- */

	if( !decompress_bzip2( patch_data->data + control_offset, control_size, &control_block ) )
	{
		return 0;
	}

	if( !decompress_bzip2( patch_data->data + diff_offset, diff_size, &diff_block ) )
	{
		free_buffer( &control_block );
		return 0;
	}

	if( !decompress_bzip2( patch_data->data + extra_offset, extra_size, &extra_block ) )
	{
		free_buffer( &control_block );
		free_buffer( &diff_block );
		return 0;
	}

	/* --------------------------------------------------------------------- */
	/* 7. Выделяем результат                                                  */
	/* --------------------------------------------------------------------- */

	new_data->size = ( size_t )new_size;

	if( new_data->size > 0 )
	{
		new_data->data = ( uint8_t * )malloc( new_data->size );

		if( new_data->data == NULL )
		{
			fprintf( stderr, "Out of memory\n" );

			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			new_data->size = 0;

			return 0;
		}
	}

	/* --------------------------------------------------------------------- */
	/* 8. Начальные позиции                                                   */
	/* --------------------------------------------------------------------- */

	old_pos = 0;
	new_pos = 0;

	control_pos = 0;
	diff_pos = 0;
	extra_pos = 0;

	/* --------------------------------------------------------------------- */
	/* 9. Обработка control block                                             */
	/* --------------------------------------------------------------------- */

	uint64_t x;
	uint64_t y;
	uint64_t z;

	while( new_pos < new_size )
	{
		x = 0;
		y = 0;
		z = 0;

		/*
			Каждая control-команда = 24 байта.
		*/
		if( control_pos > control_block.size || control_block.size - control_pos < 24 )
		{
			fprintf( stderr, "Truncated BSDIFF4 control block\n" );

			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}

		x = read_int64( control_block.data + control_pos, &ok );

		if( !ok )
		{
			fprintf( stderr, "Invalid BSDIFF4 control value\n" );
			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}

		y = read_int64( control_block.data + control_pos + 8, &ok );

		if( !ok )
		{
			fprintf( stderr, "Invalid BSDIFF4 control value\n" );
			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}

		z = read_int64( control_block.data + control_pos + 16, &ok );

		if( !ok )
		{
			fprintf( stderr, "Invalid BSDIFF4 control value\n" );
			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}

		control_pos += 24;


		/* ----------------------------------------------------------------- */
		/* x/y не могут быть отрицательными                                  */
		/* ----------------------------------------------------------------- */

		if( x < 0 )
		{
			fprintf( stderr, "Invalid BSDIFF4 control: negative diff size\n" );

			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}

		if( y < 0 )
		{
			fprintf( stderr, "Invalid BSDIFF4 control: negative extra size\n" );

			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}


		/* ----------------------------------------------------------------- */
		/* Проверяем x относительно output                                    */
		/* ----------------------------------------------------------------- */

		if( x > new_size - new_pos )
		{
			fprintf( stderr, "BSDIFF4 diff data exceeds output size\n" );

			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}


		/* ----------------------------------------------------------------- */
		/* Проверяем diff block                                                */
		/* ----------------------------------------------------------------- */

		if( !range_fits( ( uint64_t )diff_pos, x, diff_block.size ) )
		{
			fprintf( stderr, "BSDIFF4 diff block is truncated\n" );

			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}


		/* ----------------------------------------------------------------- */
		/* Применяем diff                                                      */
		/* ----------------------------------------------------------------- */

		{
			for( uint64_t i = 0; i < x; ++i )
			{
				uint64_t old_index;
				size_t output_index;
				uint8_t diff_byte;

				old_index = old_pos + i;

				output_index = ( size_t )( new_pos + i );

				diff_byte =
					diff_block.data[ diff_pos + ( size_t )i ];

				if( old_index >= 0 && old_index < ( uint64_t )old_data->size )
				{
					uint8_t old_byte;

					old_byte = old_data->data[ ( size_t )old_index ];

					/*
						Сложение Byte происходит modulo 256.
					*/
					new_data->data[ output_index ] = ( uint8_t )( diff_byte + old_byte );
				}
				else
				{
					new_data->data[ output_index ] = diff_byte;
				}
			}
		}


		diff_pos += ( size_t )x;
		new_pos += x;


		/* ----------------------------------------------------------------- */
		/* old_pos += x                                                       */
		/* ----------------------------------------------------------------- */

		if( x > 0 && old_pos > INT64_MAX - x )
		{
			fprintf( stderr, "BSDIFF4 old position overflow\n" );

			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}

		old_pos += x;


		/* ----------------------------------------------------------------- */
		/* Extra block                                                        */
		/* ----------------------------------------------------------------- */

		if( y > new_size - new_pos )
		{
			fprintf( stderr, "BSDIFF4 extra data exceeds output size\n" );

			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}

		if( !range_fits( ( uint64_t )extra_pos, y, extra_block.size ) )
		{
			fprintf( stderr, "BSDIFF4 extra block is truncated\n" );

			free_buffer( &control_block );
			free_buffer( &diff_block );
			free_buffer( &extra_block );

			free_buffer( new_data );

			return 0;
		}

		if( y > 0 )
		{
			memcpy( new_data->data + ( size_t )new_pos, extra_block.data + extra_pos, ( size_t )y );
		}

		extra_pos += ( size_t )y;
		new_pos += y;


		/* ----------------------------------------------------------------- */
		/* z — изменение позиции в old                                         */
		/* ----------------------------------------------------------------- */

		if( z > 0 )
		{
			if( old_pos > INT64_MAX - z )
			{
				fprintf( stderr, "BSDIFF4 old position overflow\n" );

				free_buffer( &control_block );
				free_buffer( &diff_block );
				free_buffer( &extra_block );

				free_buffer( new_data );

				return 0;
			}
		}
		else
			if( z < 0 )
			{
				if( old_pos < INT64_MIN - z )
				{
					fprintf( stderr, "BSDIFF4 old position overflow\n" );

					free_buffer( &control_block );
					free_buffer( &diff_block );
					free_buffer( &extra_block );

					free_buffer( new_data );

					return 0;
				}
			}

		old_pos += z;
	}

	/* --------------------------------------------------------------------- */
	/* 10. Финальные проверки                                                 */
	/* --------------------------------------------------------------------- */

	if( new_pos != new_size )
	{
		fprintf( stderr, "BSDIFF4 patch did not produce expected output size\n" );

		free_buffer( &control_block );
		free_buffer( &diff_block );
		free_buffer( &extra_block );

		free_buffer( new_data );

		return 0;
	}

	if( diff_pos != diff_block.size )
	{
		fprintf( stderr, "BSDIFF4 diff block has unused data\n" );

		free_buffer( &control_block );
		free_buffer( &diff_block );
		free_buffer( &extra_block );

		free_buffer( new_data );

		return 0;
	}

	if( extra_pos != extra_block.size )
	{
		fprintf( stderr, "BSDIFF4 extra block has unused data\n" );

		free_buffer( &control_block );
		free_buffer( &diff_block );
		free_buffer( &extra_block );

		free_buffer( new_data );

		return 0;
	}

	if( control_pos != control_block.size )
	{
		fprintf( stderr, "BSDIFF4 control block has unused data\n" );

		free_buffer( &control_block );
		free_buffer( &diff_block );
		free_buffer( &extra_block );

		free_buffer( new_data );

		return 0;
	}

	/* --------------------------------------------------------------------- */
	/* Успех                                                                 */
	/* --------------------------------------------------------------------- */

	free_buffer( &control_block );
	free_buffer( &diff_block );
	free_buffer( &extra_block );

	return 1;
}

/*
	Получение последней ошибки WinAPI в удобном виде.
*/
static void print_win_error( const char *operation, const char *path )
{
	DWORD error_code;
	char message[ 512 ];
	DWORD length;

	error_code = GetLastError();

	length = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error_code, 0, message, sizeof( message ), NULL );

	if( length > 0 )
	{
		/*
			Убираем CR/LF в конце сообщения.
		*/
		while( length > 0 && ( message[ length - 1 ] == '\r' || message[ length - 1 ] == '\n' ) )
		{
			message[ length - 1 ] = '\0';
			--length;
		}

		fprintf( stderr, "%s: %s: %s (error %lu)\n", operation, path, message, ( unsigned long )error_code );
	}
	else
	{
		fprintf( stderr, "%s: %s (error %lu)\n", operation, path, ( unsigned long )error_code );
	}
}

static int read_file( const char *path, Buffer *buffer )
{
	HANDLE file;
	LARGE_INTEGER file_size_li;
	uint64_t file_size;
	size_t size;
	size_t total_read;

	buffer->data = NULL;
	buffer->size = 0;


	/* --------------------------------------------------------------------- */
	/* Открываем файл                                                         */
	/* --------------------------------------------------------------------- */

	file = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if( file == INVALID_HANDLE_VALUE )
	{
		print_win_error( "Cannot open file", path );		
		return -1;
	}


	/* --------------------------------------------------------------------- */
	/* Получаем размер                                                        */
	/* --------------------------------------------------------------------- */

	if( !GetFileSizeEx( file, &file_size_li ) )
	{
		print_win_error( "Cannot determine file size", path );

		CloseHandle( file );

		return 0;
	}

	if( file_size_li.QuadPart < 0 )
	{
		fprintf( stderr, "Invalid file size: %s\n", path );

		CloseHandle( file );

		return 0;
	}

	file_size = ( uint64_t )file_size_li.QuadPart;


	/*
		Проверяем, помещается ли размер в size_t.
	*/
	if( file_size > ( uint64_t )SIZE_MAX )
	{
		fprintf( stderr, "File is too large: %s\n", path );

		CloseHandle( file );

		return 0;
	}

	size = ( size_t )file_size;


	/* --------------------------------------------------------------------- */
	/* Выделяем память                                                        */
	/* --------------------------------------------------------------------- */

	if( size > 0 )
	{
		buffer->data = ( uint8_t * )malloc( size );

		if( buffer->data == NULL )
		{
			fprintf( stderr, "Out of memory while reading: %s\n", path );

			CloseHandle( file );

			return 0;
		}
	}


	/* --------------------------------------------------------------------- */
	/* Читаем файл                                                            */
	/* --------------------------------------------------------------------- */

	total_read = 0;

	while( total_read < size )
	{
		size_t remaining;
		DWORD chunk_size;
		DWORD bytes_read;

		remaining = size - total_read;

		/*
			ReadFile принимает DWORD размер.
			Поэтому большие файлы читаем частями <= 4 GiB.
		*/
		if( remaining > ( size_t )0xFFFFFFFFUL )
			chunk_size = 0xFFFFFFFFUL;
		else
			chunk_size = ( DWORD )remaining;

		bytes_read = 0;

		if( !ReadFile(
			file,
			buffer->data + total_read,
			chunk_size,
			&bytes_read,
			NULL ) )
		{
			print_win_error( "Cannot read file", path );

			free_buffer( buffer );
			CloseHandle( file );

			return 0;
		}

		/*
			Защита от неожиданного нулевого чтения.
		*/
		if( bytes_read == 0 )
		{
			fprintf( stderr, "Unexpected end of file: %s\n", path );

			free_buffer( buffer );
			CloseHandle( file );

			return 0;
		}

		total_read += ( size_t )bytes_read;
	}


	/* --------------------------------------------------------------------- */
	/* Закрываем файл                                                         */
	/* --------------------------------------------------------------------- */

	if( !CloseHandle( file ) )
	{
		print_win_error( "Cannot close file", path );

		free_buffer( buffer );

		return 0;
	}


	buffer->size = size;

	return 1;
}

static int write_file( const char *path, const Buffer *buffer )
{
	HANDLE file;
	size_t total_written;

	/*
		CREATE_ALWAYS:
		- создаёт новый файл;
		- если файл уже существует — перезаписывает его.
	*/
	file = CreateFileA( path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	if( file == INVALID_HANDLE_VALUE )
	{
		print_win_error( "Cannot create file",
						 path
		);

		return 0;
	}


	/* --------------------------------------------------------------------- */
	/* Записываем данные                                                      */
	/* --------------------------------------------------------------------- */

	total_written = 0;

	while( total_written < buffer->size )
	{
		size_t remaining;
		DWORD chunk_size;
		DWORD bytes_written;

		remaining =
			buffer->size - total_written;

		/*
			WriteFile также использует DWORD.
			Большие файлы пишем частями.
		*/
		if( remaining > ( size_t )0xFFFFFFFFUL )
			chunk_size = 0xFFFFFFFFUL;
		else
			chunk_size = ( DWORD )remaining;

		bytes_written = 0;

		if( !WriteFile( file, buffer->data + total_written, chunk_size, &bytes_written, NULL ) )
		{
			print_win_error( "Cannot write file", path );

			CloseHandle( file );

			return 0;
		}

		/*
			Защита от зацикливания при неожиданной ситуации.
		*/
		if( bytes_written == 0 )
		{
			fprintf( stderr, "WriteFile wrote zero bytes: %s\n", path );

			CloseHandle( file );

			return 0;
		}

		total_written += ( size_t )bytes_written;
	}


	/* --------------------------------------------------------------------- */
	/* Закрываем файл                                                         */
	/* --------------------------------------------------------------------- */

	if( !CloseHandle( file ) )
	{
		print_win_error( "Cannot close output file", path );

		return 0;
	}

	return 1;
}

// Функция патчинга файлов BSDIFF40
// @param (std::filesystem::path) GamePath - Путь к папке с игрой
// @param (std::filesystem::path) TmpPath - Путь к временной папке
// @param (std::filesystem::path) fPatch - Путь к файлу с патчем
bool ApplyPatch_BSD( std::wstring GamePath, std::wstring TmpPath, std::wstring fPatch, uint32_t *FilesProcessed, uint32_t *AllFiles )
{
	Buffer old_data;
	Buffer patch_data;
	Buffer new_data;

	// Получим относительный путь к файлу с патчем
	std::filesystem::path relativePath = std::filesystem::relative( fPatch, TmpPath );

	// Оригинальный файл в папке с игрой
	std::filesystem::path OriginalFile = relativePath;
	OriginalFile.replace_extension();

	wprintf( L"----------------------------------------------------\n" );
	wprintf( L"Обработка файла [%d/%d]: %s\n", *FilesProcessed, *AllFiles, OriginalFile.wstring().c_str() );

	*FilesProcessed = *FilesProcessed + 1;

	/* --------------------------------------------------------------------- */
	/* Читаем старый файл                                                     */
	/* --------------------------------------------------------------------- */

	if( read_file( wstring_to_string( GamePath + L"\\" + OriginalFile.wstring() ).c_str(), &old_data ) == 0 )
	{
		return 1;
	}

	/*int ResultReadFike = read_file( wstring_to_string( GamePath + L"\\" + OriginalFile.wstring() ).c_str(), &old_data );

	if( ResultReadFike == -1 )
	{
		std::string TmpMsg( "Файл\n" + wstring_to_string( GamePath ) + "\nне найден. Пропустить его обработку?\nВнимание! После пропуска работоспособность игры не гарантируется" );
		
		if( MessageBoxA( NULL, "Ошибка открытия файла", TmpMsg.c_str(), MB_YESNO ) == IDYES )
		{
			HANDLE hFile = CreateFileA( wstring_to_string( GamePath + L"\\123.tmp" ).c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

			if( hFile != INVALID_HANDLE_VALUE )
			{
				CloseHandle( hFile );
			}
		}
		return 0;
	}*/


	/* --------------------------------------------------------------------- */
	/* Читаем patch                                                           */
	/* --------------------------------------------------------------------- */

	if( !read_file( wstring_to_string( fPatch ).c_str(), &patch_data ) )
	{
		free_buffer( &old_data );

		return 1;
	}

	/* --------------------------------------------------------------------- */
	/* Применяем patch                                                        */
	/* --------------------------------------------------------------------- */
	if( apply_patch( &old_data, &patch_data, &new_data ) == 0 )
	{
		free_buffer( &old_data );
		free_buffer( &patch_data );

		return 1;
	}

	/* --------------------------------------------------------------------- */
	/* Записываем результат                                                   */
	/* --------------------------------------------------------------------- */

	if( !write_file( wstring_to_string( GamePath + L"\\123.tmp" ).c_str(), &new_data ) )
	{
		free_buffer( &old_data );
		free_buffer( &patch_data );
		free_buffer( &new_data );

		return 1;
	}

	free_buffer( &old_data );
	free_buffer( &patch_data );
	free_buffer( &new_data );
}