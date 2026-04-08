#include "search.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>

// Эвристика: отбрасываем бинарные/плохо декодируемые файлы,
// чтобы не показывать в результатах "мусор".
static int isLikelyTextBuffer(const unsigned char* data, size_t len) {
    if (!data || len == 0) return 1;

    size_t controlCount = 0;
    size_t highCount = 0;
    size_t zeroCount = 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = data[i];
        if (c == 0) {
            zeroCount++;
            continue;
        }
        if (c >= 0x80) {
            highCount++;
            continue;
        }
        if (c < 0x20 && c != '\n' && c != '\r' && c != '\t' && c != '\f') {
            controlCount++;
        }
    }

    if (zeroCount > 0) return 0;
    if (controlCount > (len / 20)) return 0; // >5% управляющих символов
    if (highCount > 0 && controlCount > 0) return 0;
    return 1;
}

// Глобальные переменные
SearchResult* g_searchResults = NULL;
SearchStats g_searchStats = {0, 0, 0, 0};
volatile int g_searchCanceled = 0;
static CRITICAL_SECTION g_resultLock;
static int g_lockInitialized = 0;

// Инициализация поиска
void initSearch(void) {
    g_searchResults = NULL;
    memset(&g_searchStats, 0, sizeof(SearchStats));
    g_searchCanceled = 0;
    if (!g_lockInitialized) {
        InitializeCriticalSection(&g_resultLock);
        g_lockInitialized = 1;
    }
}

// Очистка результатов
void freeSearchResults(void) {
    if (g_lockInitialized) {
        EnterCriticalSection(&g_resultLock);
    }
    SearchResult* current = g_searchResults;
    while (current) {
        SearchResult* next = current->next;
        free(current);
        current = next;
    }
    g_searchResults = NULL;
    if (g_lockInitialized) {
        LeaveCriticalSection(&g_resultLock);
    }
}

// Проверка расширения файла
int matchesExtension(const char* filename, char extensions[][MAX_EXT_LEN], int extensionCount) {
    if (extensionCount == 0) {
        return 1; // Если расширения не указаны, принимать все файлы
    }
    
    char fileExt[MAX_EXT_LEN];
    getFileExtension(filename, fileExt, MAX_EXT_LEN);
    
    if (fileExt[0] == '\0') {
        // Файл без расширения - проверяем, есть ли пустое расширение в списке
        for (int i = 0; i < extensionCount; i++) {
            if (extensions[i][0] == '\0' || strcmp(extensions[i], ".") == 0) {
                return 1;
            }
        }
        return 0;
    }
    
    // Проверяем каждое расширение
    for (int i = 0; i < extensionCount; i++) {
        if (compareNoCase(fileExt, extensions[i])) {
            return 1;
        }
        // Также проверяем с точкой в начале, если её нет
        char extWithDot[MAX_EXT_LEN];
        if (extensions[i][0] != '.') {
            snprintf(extWithDot, MAX_EXT_LEN, ".%s", extensions[i]);
            if (compareNoCase(fileExt, extWithDot)) {
                return 1;
            }
        }
    }
    
    return 0;
}

// Добавление результата поиска
void addSearchResult(const char* filepath, int lineNumber, const char* lineContent, const char* keyword) {
    if (!filepath || !lineContent || !keyword) return;
    if (filepath[0] == '\0' || keyword[0] == '\0') return;
    
    if (g_lockInitialized) {
        EnterCriticalSection(&g_resultLock);
    }
    
    SearchResult* newResult = (SearchResult*)malloc(sizeof(SearchResult));
    if (newResult) {
        memset(newResult, 0, sizeof(SearchResult));
        strncpy(newResult->filepath, filepath, MAX_PATH_LEN - 1);
        newResult->filepath[MAX_PATH_LEN - 1] = '\0';
        newResult->lineNumber = lineNumber;
        strncpy(newResult->lineContent, lineContent, MAX_LINE_LEN - 1);
        newResult->lineContent[MAX_LINE_LEN - 1] = '\0';
        strncpy(newResult->keyword, keyword, MAX_WORD_LEN - 1);
        newResult->keyword[MAX_WORD_LEN - 1] = '\0';
        newResult->next = g_searchResults;
        g_searchResults = newResult;
        g_searchStats.totalMatches++;
    }
    
    if (g_lockInitialized) {
        LeaveCriticalSection(&g_resultLock);
    }
}

// Поиск вхождения в строке
static int findKeywordInLine(const char* line, const char* keyword, int caseSensitive, int wholeWord) {
    if (!line || !keyword || keyword[0] == '\0') {
        return 0;
    }
    
    int lineLen = strlen(line);
    int keywordLen = strlen(keyword);
    if (keywordLen == 0) return 0;
    if (keywordLen > lineLen) return 0; // Ключевое слово длиннее строки
    if (keywordLen > MAX_WORD_LEN) return 0; // Защита от переполнения
    
    const char* pos = line;
    
    while (*pos) {
        const char* found = NULL;
        
        if (caseSensitive) {
            found = strstr(pos, keyword);
        } else {
            // Поиск без учёта регистра
            const char* searchPos = pos;
            while (*searchPos) {
                int match = 1;
                // Проверяем, достаточно ли символов для совпадения
                for (int i = 0; i < keywordLen; i++) {
                    if (searchPos[i] == '\0' || 
                        tolower((unsigned char)searchPos[i]) != tolower((unsigned char)keyword[i])) {
                        match = 0;
                        break;
                    }
                }
                if (match) {
                    found = searchPos;
                    break;
                }
                searchPos++;
            }
        }
        
        if (!found) break;
        
        // Проверяем границы слова, если требуется
        if (wholeWord) {
            // Проверяем, что found находится в пределах строки
            if (found >= line && found + keywordLen <= line + lineLen) {
                if ((found == line || isWordBoundary(found[-1])) &&
                    (found[keywordLen] == '\0' || isWordBoundary(found[keywordLen]))) {
                    return 1;
                }
            }
            pos = found + 1; // Продолжаем поиск дальше
            if (pos >= line + lineLen) break; // Защита от выхода за границы
        } else {
            return 1; // Найдено совпадение
        }
    }
    
    return 0;
}

// Поиск в файле
void searchInFile(const char* filepath, const SearchParams* params) {
    if (g_searchCanceled) return;
    if (!filepath || filepath[0] == '\0') return;
    if (!params) return;
    if (params->keywordCount <= 0 || params->keywordCount > MAX_KEYWORDS) return;
    
    // Конвертируем UTF-8 путь в широкую строку для открытия файла
    wchar_t widePath[MAX_PATH_LEN];
    widePath[0] = L'\0';
    MultiByteToWideUTF8(filepath, widePath, MAX_PATH_LEN);
    
    if (widePath[0] == L'\0') {
        return; // Не удалось конвертировать путь
    }
    
    // Используем _wfopen для работы с UTF-8 путями (доступен в MinGW)
    FILE* file = NULL;
    #ifdef __MINGW32__
    file = _wfopen(widePath, L"rb");
    #else
    // Альтернатива для других компиляторов
    char mbPath[MAX_PATH_LEN * 2];
    WideCharToMultiByte(CP_ACP, 0, widePath, -1, mbPath, MAX_PATH_LEN * 2, NULL, NULL);
    file = fopen(mbPath, "rb");
    #endif
    if (!file) {
        return; // Не удалось открыть файл
    }

    // Быстрая проверка, что это действительно текстовый файл.
    unsigned char probe[1024];
    size_t probeLen = fread(probe, 1, sizeof(probe), file);
    if (probeLen > 0 && !isLikelyTextBuffer(probe, probeLen)) {
        fclose(file);
        return;
    }
    rewind(file);
    
    char line[MAX_LINE_LEN];
    int lineNumber = 0;
    int fileHasMatches = 0;
    
    // Читаем файл построчно
    while (fgets(line, MAX_LINE_LEN, file) && !g_searchCanceled) {
        lineNumber++;
        
        // Защита от слишком больших файлов
        if (lineNumber > 1000000) break; // Ограничение на количество строк
        
        // Убираем символ новой строки
        size_t len = strlen(line);
        if (len >= MAX_LINE_LEN - 1) {
            // Строка слишком длинная, пропускаем
            continue;
        }
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (len > 1 && line[len - 2] == '\r') {
            line[len - 2] = '\0';
        }
        
        // Пропускаем пустые строки после обработки
        if (line[0] == '\0') {
            continue;
        }
        
        // Проверяем каждое ключевое слово
        for (int i = 0; i < params->keywordCount && i < MAX_KEYWORDS; i++) {
            if (params->keywords[i][0] != '\0') {
                if (findKeywordInLine(line, params->keywords[i], 
                                     params->caseSensitive, params->wholeWord)) {
                    addSearchResult(filepath, lineNumber, line, params->keywords[i]);
                    fileHasMatches = 1;
                    // Не прерываем цикл, чтобы найти все ключевые слова в строке
                }
            }
        }
    }
    
    fclose(file);
    g_searchStats.processedFiles++;
    
    // Если были найдены совпадения, увеличиваем счётчик файлов
    if (fileHasMatches) {
        g_searchStats.matchedFiles++;
    }
}

// Рекурсивный обход директории (использует Windows API для работы с UTF-8 путями)
static int g_directoryDepth = 0;
#define MAX_DIRECTORY_DEPTH 100

void searchInDirectory(const char* path, const SearchParams* params) {
    if (g_searchCanceled) return;
    if (!path || path[0] == '\0') return;
    if (!params) return;
    
    // Защита от слишком глубокой рекурсии
    if (g_directoryDepth >= MAX_DIRECTORY_DEPTH) {
        return;
    }
    g_directoryDepth++;
    
    // Конвертируем UTF-8 путь в широкую строку
    wchar_t widePath[MAX_PATH_LEN];
    MultiByteToWideUTF8(path, widePath, MAX_PATH_LEN);
    if (widePath[0] == L'\0') {
        g_directoryDepth--;
        return;
    }
    
    // Формируем маску поиска
    wchar_t searchPattern[MAX_PATH_LEN + 3];
    int len = wcslen(widePath);
    if (len <= 0 || len >= MAX_PATH_LEN - 3) {
        g_directoryDepth--;
        return;
    }
    
    wcscpy(searchPattern, widePath);
    if (searchPattern[len - 1] != L'\\' && searchPattern[len - 1] != L'/') {
        if (len + 1 >= MAX_PATH_LEN - 3) {
            g_directoryDepth--;
            return;
        }
        wcscat(searchPattern, L"\\");
        len++;
    }
    if (len + 1 >= MAX_PATH_LEN - 3) {
        g_directoryDepth--;
        return;
    }
    wcscat(searchPattern, L"*");
    
    // Ищем файлы и директории
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern, &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        g_directoryDepth--;
        return; // Не удалось открыть директорию
    }
    
    do {
        if (g_searchCanceled) break;
        
        // Пропускаем текущую и родительскую директории
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }
        
        // Формируем полный путь
        wchar_t fullPathW[MAX_PATH_LEN];
        wcscpy(fullPathW, widePath);
        int pathLen = wcslen(fullPathW);
        if (pathLen > 0 && fullPathW[pathLen - 1] != L'\\' && fullPathW[pathLen - 1] != L'/') {
            if (pathLen + 1 < MAX_PATH_LEN) {
                wcscat(fullPathW, L"\\");
                pathLen++;
            }
        }
        int fileNameLen = wcslen(findData.cFileName);
        if (pathLen + fileNameLen >= MAX_PATH_LEN) {
            continue; // Путь слишком длинный
        }
        wcscat(fullPathW, findData.cFileName);
        
        // Конвертируем обратно в UTF-8
        char fullPath[MAX_PATH_LEN];
        WideToMultiByteUTF8(fullPathW, fullPath, MAX_PATH_LEN);
        if (fullPath[0] == '\0') continue;
        
        // Конвертируем имя файла для проверки расширения
        char fileName[MAX_PATH_LEN];
        WideToMultiByteUTF8(findData.cFileName, fileName, MAX_PATH_LEN);
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Это директория
            if (params->recursive) {
                searchInDirectory(fullPath, params);
            }
        } else {
            // Это обычный файл
            if (matchesExtension(fileName, (char(*)[MAX_EXT_LEN])params->extensions, 
                               params->extensionCount)) {
                searchInFile(fullPath, params);
            }
        }
    } while (FindNextFileW(hFind, &findData) != 0);
    
    FindClose(hFind);
    g_directoryDepth--;
}

// Основная функция поиска (вызывается из GUI в отдельном потоке)
DWORD WINAPI searchThread(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    if (!data) return 1;
    
    SearchParams* params = &data->params;
    HWND hwnd = data->hwnd;
    
    if (!params || !IsWindow(hwnd)) {
        free(data);
        return 1;
    }
    
    initSearch();
    g_searchCanceled = 0;
    g_directoryDepth = 0; // Сбрасываем счетчик глубины рекурсии
    
    // Проверяем, является ли путь файлом или директорией
    if (params->searchPath[0] != '\0') {
        // Конвертируем UTF-8 путь в широкую строку
        wchar_t widePath[MAX_PATH_LEN];
        MultiByteToWideUTF8(params->searchPath, widePath, MAX_PATH_LEN);
        
        if (widePath[0] != L'\0') {
            DWORD attrs = GetFileAttributesW(widePath);
            if (attrs != INVALID_FILE_ATTRIBUTES) {
                if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
                    searchInDirectory(params->searchPath, params);
                } else {
                    // Это файл
                    if (matchesExtension(params->searchPath, 
                                       (char(*)[MAX_EXT_LEN])params->extensions, 
                                       params->extensionCount)) {
                        searchInFile(params->searchPath, params);
                    }
                }
            }
        }
    }
    
    // Отправляем сообщение об окончании поиска
    if (IsWindow(hwnd)) {
        PostMessage(hwnd, WM_USER + 1, 0, 0);
    }
    
    free(data);
    
    return 0;
}

