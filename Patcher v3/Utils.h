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
bool GetLatestRelease( std::string Version );