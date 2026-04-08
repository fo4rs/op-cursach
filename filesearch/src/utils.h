#ifndef UTILS_H
#define UTILS_H

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>

#define MAX_WORD_LEN 256
#define MAX_LINE_LEN 1024
#define MAX_KEYWORDS 50
#define MAX_EXTENSIONS 20
#define MAX_EXT_LEN 20

// Разбор строки с разделителями (запятая)
int parseCommaSeparated(const char* input, char output[][MAX_WORD_LEN], int maxItems);

// Удаление пробелов в начале и конце строки
void trimWhitespace(char* str);

// Сравнение строк без учёта регистра
int compareNoCase(const char* str1, const char* str2);

// Получение расширения файла
void getFileExtension(const char* filename, char* extension, int maxLen);

// Проверка, является ли позиция границей слова в UTF-8 строке
// Возвращает 1 если это граница (начало нового слова или конец строки)
int isWordBoundaryUTF8(const char* str, int pos);

// Копирование широких строк в UTF-8
void WideToMultiByteUTF8(const wchar_t* wide, char* multi, int maxLen);

// Копирование UTF-8 в широкие строки
void MultiByteToWideUTF8(const char* multi, wchar_t* wide, int maxLen);

// Проверка границы слова в UTF-8 строке
// Возвращает 1 если это граница (начало нового слова или конец строки)
int isWordBoundaryUTF8(const char* str, int pos);

#endif // UTILS_H

