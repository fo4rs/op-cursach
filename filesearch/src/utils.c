#include "utils.h"
#include <string.h>
#include <ctype.h>

// Разбор строки с разделителями (запятая)
int parseCommaSeparated(const char* input, char output[][MAX_WORD_LEN], int maxItems) {
    if (!input || !output || maxItems <= 0) {
        return 0;
    }
    
    int count = 0;
    const char* start = input;
    const char* current = input;
    
    while (*current && count < maxItems) {
        // Пропускаем пробелы
        while (*current == ' ') current++;
        
        if (*current == '\0') break;
        
        start = current;
        
        // Ищем запятую или конец строки
        while (*current && *current != ',') {
            current++;
        }
        
        // Копируем элемент
        int len = current - start;
        if (len > 0 && len < MAX_WORD_LEN) {
            strncpy(output[count], start, len);
            output[count][len] = '\0';
            trimWhitespace(output[count]);
            
            // Добавляем только непустые элементы
            if (output[count][0] != '\0') {
                count++;
            }
        }
        
        if (*current == ',') {
            current++;
        }
    }
    
    return count;
}

// Удаление пробелов в начале и конце строки
void trimWhitespace(char* str) {
    if (!str) return;
    
    char* start = str;
    char* end;
    
    // Убираем пробелы в начале
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {
        start++;
    }
    
    if (*start == '\0') {
        *str = '\0';
        return;
    }
    
    // Убираем пробелы в конце
    end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    
    end[1] = '\0';
    
    // Сдвигаем строку, если начало изменилось
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// Сравнение строк без учёта регистра
int compareNoCase(const char* str1, const char* str2) {
    if (!str1 || !str2) return 0;
    
    while (*str1 && *str2) {
        if (tolower((unsigned char)*str1) != tolower((unsigned char)*str2)) {
            return 0;
        }
        str1++;
        str2++;
    }
    
    return (*str1 == '\0' && *str2 == '\0');
}

// Получение расширения файла
void getFileExtension(const char* filename, char* extension, int maxLen) {
    if (!filename || !extension || maxLen <= 0) {
        if (extension) extension[0] = '\0';
        return;
    }
    
    const char* dot = strrchr(filename, '.');
    if (dot && dot != filename) {
        int len = strlen(dot);
        if (len < maxLen) {
            strncpy(extension, dot, len);
            extension[len] = '\0';
        } else {
            extension[0] = '\0';
        }
    } else {
        extension[0] = '\0';
    }
}

// Проверка, является ли символ границей слова
int isWordBoundary(char c) {
    return !isalnum((unsigned char)c) && c != '_';
}

// Копирование широких строк в UTF-8
void WideToMultiByteUTF8(const wchar_t* wide, char* multi, int maxLen) {
    if (!wide || !multi || maxLen <= 0) {
        if (multi) multi[0] = '\0';
        return;
    }
    
    int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, multi, maxLen, NULL, NULL);
    if (len == 0 || len >= maxLen) {
        multi[0] = '\0';
    }
}

// Копирование UTF-8 в широкие строки (с автоматическим определением кодировки)
void MultiByteToWideUTF8(const char* multi, wchar_t* wide, int maxLen) {
    if (!multi || !wide || maxLen <= 0) {
        if (wide) wide[0] = L'\0';
        return;
    }
    
    if (multi[0] == '\0') {
        wide[0] = L'\0';
        return;
    }
    
    int len = 0;
    
    // Сначала пробуем UTF-8
    len = MultiByteToWideChar(CP_UTF8, 0, multi, -1, wide, maxLen);
    
    // Проверяем, есть ли невалидные символы (проверяем первый символ результата)
    if (len > 0 && len < maxLen) {
        // Проверяем, не содержит ли результат заменяющие символы (обычно это признак ошибки)
        BOOL hasInvalid = FALSE;
        for (int i = 0; i < len && i < 10; i++) {
            if (wide[i] == 0xFFFD || wide[i] == L'?') {
                // Может быть признаком ошибки, но не всегда
                break;
            }
        }
        
        // Если результат выглядит нормально, используем его
        if (!hasInvalid && wide[0] != 0) {
            return;
        }
    }
    
    // Если UTF-8 не сработал, пробуем Windows-1251 (стандартная кодировка для русских текстов)
    SetLastError(0);
    len = MultiByteToWideChar(1251, 0, multi, -1, wide, maxLen);
    if (len > 0 && len < maxLen) {
        return;
    }
    
    // Если и это не сработало, пробуем текущую кодовую страницу системы
    SetLastError(0);
    len = MultiByteToWideChar(CP_ACP, 0, multi, -1, wide, maxLen);
    if (len > 0 && len < maxLen) {
        return;
    }
    
    // Если ничего не сработало, устанавливаем пустую строку
    wide[0] = L'\0';
}

