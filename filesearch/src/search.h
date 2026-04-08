#ifndef SEARCH_H
#define SEARCH_H

#include <windows.h>
#include "utils.h"

#define MAX_PATH_LEN 260

// Структура для параметров поиска
typedef struct {
    char searchPath[MAX_PATH_LEN];
    char keywords[MAX_KEYWORDS][MAX_WORD_LEN];
    int keywordCount;
    char extensions[MAX_EXTENSIONS][MAX_EXT_LEN];
    int extensionCount;
    int recursive;
    int caseSensitive;
    int wholeWord;
} SearchParams;

// Структура для результата поиска
typedef struct SearchResult {
    char filepath[MAX_PATH_LEN];
    int lineNumber;
    char lineContent[MAX_LINE_LEN];
    char keyword[MAX_WORD_LEN];
    struct SearchResult* next;
} SearchResult;

// Структура для статистики
typedef struct {
    int totalFiles;
    int matchedFiles;
    int totalMatches;
    int processedFiles;
} SearchStats;

// Структура для передачи в поток
typedef struct {
    SearchParams params;
    HWND hwnd;
} ThreadData;

// Глобальные переменные для управления поиском
extern SearchResult* g_searchResults;
extern SearchStats g_searchStats;
extern volatile int g_searchCanceled;

// Инициализация поиска
void initSearch(void);

// Очистка результатов
void freeSearchResults(void);

// Проверка расширения файла
int matchesExtension(const char* filename, char extensions[][MAX_EXT_LEN], int extensionCount);

// Поиск в файле
void searchInFile(const char* filepath, const SearchParams* params);

// Рекурсивный обход директории
void searchInDirectory(const char* path, const SearchParams* params);

// Добавление результата поиска
void addSearchResult(const char* filepath, int lineNumber, const char* lineContent, const char* keyword);

// Основная функция поиска (вызывается из GUI)
DWORD WINAPI searchThread(LPVOID lpParam);

#endif // SEARCH_H

