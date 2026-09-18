#pragma once
#include <windows.h>
#include <filesystem>

// Функция патчинга файлов BSDIFF40
// @param (std::filesystem::path) GamePath - Путь к папке с игрой
// @param (std::filesystem::path) TmpPath - Путь к временной папке
// @param (std::filesystem::path) fPatch - Путь к файлу с патчем
bool ApplyPatch_BSD( std::wstring GamePath, std::wstring TmpPath, std::wstring fPatch, uint32_t *FilesProcessed, uint32_t *AllFiles );