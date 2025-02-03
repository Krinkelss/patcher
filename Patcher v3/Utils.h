#pragma once
#include <string>

std::wstring GetFileVersion( const std::wstring& filePath );
std::wstring ExtractVersion( const std::wstring& fileName );
std::wstring GetFileName( const std::wstring& filePath );
bool ExtractZipArchive( std::wstring zipFilePath, std::wstring extractToPath );
std::vector<std::wstring> SearchAllFilesInDirectory( const std::wstring& directory );
std::vector<std::wstring> SearchFilesInDirectory( const std::wstring& directory, const std::wstring& filePattern );