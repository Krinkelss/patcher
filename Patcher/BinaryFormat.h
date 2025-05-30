﻿/*
* This is an open source non-commercial project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
*/
#pragma once
#include<windows.h>
#include <unordered_map>
//#include "FileMapper.h"

//#define MD5_OFF		0	// Выкл
//#define MD5_FULL		1	// Проверка всех файлов
//#define MD5_IN		2	// Проверка только входящих файлов
//#define MD5_ACS		3	// Проверить только Assembly-CSharp.dll


uint32_t DeltaFormatHeaderLength = 9;
uint32_t CopyCommand = 0x60;
uint32_t DataCommand = 0x80;
uint32_t Version = 0x01;

// Данные оригинального файла
std::string baseFileHashAlgorithm;	// MD5 или другой алгоритм
std::string baseFileHash;			// Строка в base64

// Данные патченного файла
std::string expectedFileHashAlgorithm;	// MD5 или другой алгоритм
std::string expectedFileHash;			// Строка в base64