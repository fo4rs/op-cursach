#define __USE_MINGW_ANSI_STDIO 1
#include "gui.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <commdlg.h>
#include <shlobj.h>

// Определение strdup для совместимости с C99
#ifndef _WIN32
char* strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}
#else
// В Windows strdup может быть недоступна в некоторых версиях MinGW
char* strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}
#endif

static HANDLE g_searchThreadHandle = NULL;
static SearchParams g_currentParams;

// Создание главного окна
HWND CreateMainWindow(HINSTANCE hInstance) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FileSearchToolWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassW(&wc);
    
    HWND hwnd = CreateWindowExW(
        0,
        L"FileSearchToolWindow",
        L"Поиск текста в файлах",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    
    return hwnd;
}

// Создание элементов управления
void CreateControls(HWND hwnd) {
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    
    // Путь для поиска
    CreateWindowW(L"STATIC", L"Путь для поиска:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 10, 200, 20, hwnd, NULL, hInstance, NULL);
    
    CreateWindowW(L"EDIT", L"C:\\",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        10, 30, 550, 25, hwnd, (HMENU)IDC_PATH_EDIT, hInstance, NULL);
    
    CreateWindowW(L"BUTTON", L"Обзор...",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        570, 30, 100, 25, hwnd, (HMENU)IDC_BROWSE_BTN, hInstance, NULL);
    
    // Ключевые слова
    CreateWindowW(L"STATIC", L"Ключевые слова (через запятую):",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 70, 300, 20, hwnd, NULL, hInstance, NULL);
    
    CreateWindowW(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        10, 90, 660, 25, hwnd, (HMENU)IDC_KEYWORDS_EDIT, hInstance, NULL);
    
    // Расширения файлов
    CreateWindowW(L"STATIC", L"Расширения файлов (через запятую):",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 130, 300, 20, hwnd, NULL, hInstance, NULL);
    
    CreateWindowW(L"EDIT", L".txt, .c, .h, .cpp",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        10, 150, 660, 25, hwnd, (HMENU)IDC_EXTENSIONS_EDIT, hInstance, NULL);
    
    // Чекбоксы
    CreateWindowW(L"BUTTON", L"Рекурсивный поиск (включая подпапки)",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        10, 190, 300, 25, hwnd, (HMENU)IDC_RECURSIVE_CHECK, hInstance, NULL);
    SendMessage(GetDlgItem(hwnd, IDC_RECURSIVE_CHECK), BM_SETCHECK, BST_CHECKED, 0);
    
    CreateWindowW(L"BUTTON", L"Учитывать регистр",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        10, 215, 200, 25, hwnd, (HMENU)IDC_CASE_CHECK, hInstance, NULL);
    SendMessage(GetDlgItem(hwnd, IDC_CASE_CHECK), BM_SETCHECK, BST_CHECKED, 0);
    
    CreateWindowW(L"BUTTON", L"Искать только целые слова",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        10, 240, 250, 25, hwnd, (HMENU)IDC_WHOLEWORD_CHECK, hInstance, NULL);
    
    // Кнопки управления
    CreateWindowW(L"BUTTON", L"ПОИСК",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
        550, 190, 120, 30, hwnd, (HMENU)IDC_SEARCH_BTN, hInstance, NULL);
    
    CreateWindowW(L"BUTTON", L"ОЧИСТИТЬ",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        550, 225, 120, 30, hwnd, (HMENU)IDC_CLEAR_BTN, hInstance, NULL);
    
    // Текст статистики
    CreateWindowW(L"STATIC", L"Результаты поиска:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 275, 200, 20, hwnd, NULL, hInstance, NULL);
    
    CreateWindowW(L"STATIC", L"Найдено: 0 вхождений в 0 файлах",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 295, 400, 20, hwnd, (HMENU)IDC_STATS_TEXT, hInstance, NULL);
    
    // Список результатов
    CreateWindowW(L"LISTBOX", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_USETABSTOPS,
        10, 320, 660, 200, hwnd, (HMENU)IDC_RESULTS_LIST, hInstance, NULL);
    
    // Прогресс-бар
    CreateWindowExW(0, L"msctls_progress32", L"",
        WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
        10, 530, 660, 20, hwnd, (HMENU)IDC_PROGRESS_BAR, hInstance, NULL);
    SendMessage(GetDlgItem(hwnd, IDC_PROGRESS_BAR), PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    
    // Кнопки внизу
    CreateWindowW(L"BUTTON", L"Экспорт в .txt",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        10, 560, 150, 30, hwnd, (HMENU)IDC_EXPORT_BTN, hInstance, NULL);
    
    CreateWindowW(L"BUTTON", L"О программе",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        530, 560, 140, 30, hwnd, (HMENU)IDC_ABOUT_BTN, hInstance, NULL);
}

// Обновление прогресс-бара
void UpdateProgressBar(HWND hwnd, int percent) {
    HWND hProgress = GetDlgItem(hwnd, IDC_PROGRESS_BAR);
    if (hProgress) {
        SendMessage(hProgress, PBM_SETPOS, percent, 0);
    }
}

// Отображение результатов поиска
void DisplayResults(HWND hwnd) {
    HWND hList = GetDlgItem(hwnd, IDC_RESULTS_LIST);
    HWND hStats = GetDlgItem(hwnd, IDC_STATS_TEXT);
    
    if (!hList || !hStats) return;
    
    // Очищаем список
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    
    // Формируем строку статистики
    wchar_t statsText[256];
    _snwprintf(statsText, 256, L"Найдено: %d вхождений в %d файлах",
               g_searchStats.totalMatches, g_searchStats.matchedFiles);
    statsText[255] = L'\0';
    SetWindowTextW(hStats, statsText);
    
    // Проверяем, есть ли результаты
    if (!g_searchResults) {
        return;
    }
    
    // Отображаем результаты
    // Инвертируем список для правильного отображения (он был добавлен в обратном порядке)
    SearchResult* reversed = NULL;
    SearchResult* current = g_searchResults;
    
    // Инвертируем список
    while (current) {
        SearchResult* next = current->next;
        current->next = reversed;
        reversed = current;
        current = next;
    }
    
    // Теперь проходим по инвертированному списку
    current = reversed;
    char* currentFile = NULL;
    int fileIndex = 0;
    
    while (current) {
        if (!currentFile || strcmp(currentFile, current->filepath) != 0) {
            // Новый файл
            if (currentFile) {
                free(currentFile);
            }
            currentFile = strdup(current->filepath);
            fileIndex++;
            
            // Извлекаем только имя файла из пути
            const char* fileName = strrchr(current->filepath, '\\');
            if (!fileName) {
                fileName = strrchr(current->filepath, '/');
            }
            if (!fileName) {
                fileName = current->filepath;
            } else {
                fileName++; // Пропускаем разделитель
            }
            
            // Добавляем заголовок файла с полным путем
            wchar_t fileHeader[MAX_PATH_LEN + 100];
            char utf8Path[MAX_PATH_LEN];
            strncpy(utf8Path, current->filepath, MAX_PATH_LEN - 1);
            utf8Path[MAX_PATH_LEN - 1] = '\0';
            
            wchar_t widePath[MAX_PATH_LEN];
            MultiByteToWideUTF8(utf8Path, widePath, MAX_PATH_LEN);
            
            // Конвертируем имя файла
            wchar_t wideFileName[MAX_PATH_LEN];
            char utf8FileName[MAX_PATH_LEN];
            strncpy(utf8FileName, fileName, MAX_PATH_LEN - 1);
            utf8FileName[MAX_PATH_LEN - 1] = '\0';
            MultiByteToWideUTF8(utf8FileName, wideFileName, MAX_PATH_LEN);
            
            // Проверяем, что конвертация прошла успешно
            if (wideFileName[0] == L'\0' && fileName[0] != '\0') {
                // Если конвертация не удалась, пробуем напрямую
                int i;
                for (i = 0; i < MAX_PATH_LEN - 1 && fileName[i] != '\0'; i++) {
                    wideFileName[i] = (wchar_t)(unsigned char)fileName[i];
                }
                wideFileName[i] = L'\0';
            }
            if (widePath[0] == L'\0' && utf8Path[0] != '\0') {
                int i;
                for (i = 0; i < MAX_PATH_LEN - 1 && utf8Path[i] != '\0'; i++) {
                    widePath[i] = (wchar_t)(unsigned char)utf8Path[i];
                }
                widePath[i] = L'\0';
            }
            
            // Добавляем разделитель
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"=======================================");
            
            // Добавляем номер и имя файла (используем простую конкатенацию)
            wchar_t fileNum[20];
            _snwprintf(fileNum, 20, L"%d", fileIndex);
            fileNum[19] = L'\0';
            wcscpy(fileHeader, L"Файл ");
            wcscat(fileHeader, fileNum);
            wcscat(fileHeader, L": ");
            wcscat(fileHeader, wideFileName);
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)fileHeader);
            
            // Добавляем полный путь
            wchar_t pathLine[MAX_PATH_LEN + 50];
            wcscpy(pathLine, L"Путь: ");
            wcscat(pathLine, widePath);
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)pathLine);
        }
        
        // Добавляем строку результата
        wchar_t resultLine[MAX_LINE_LEN + 100];
        wchar_t wideContent[MAX_LINE_LEN];
        wchar_t lineNumStr[20];
        
        MultiByteToWideUTF8(current->lineContent, wideContent, MAX_LINE_LEN);
        
        // Проверяем конвертацию содержимого
        if (wideContent[0] == L'\0' && current->lineContent[0] != '\0') {
            // Если конвертация не удалась, пробуем напрямую
            int i;
            for (i = 0; i < MAX_LINE_LEN - 1 && current->lineContent[i] != '\0'; i++) {
                wideContent[i] = (wchar_t)(unsigned char)current->lineContent[i];
            }
            wideContent[i] = L'\0';
        }
        
        // Форматируем номер строки
        _snwprintf(lineNumStr, 20, L"%d", current->lineNumber);
        lineNumStr[19] = L'\0';
        
        // Формируем строку результата через конкатенацию
        wcscpy(resultLine, L"  Строка ");
        wcscat(resultLine, lineNumStr);
        wcscat(resultLine, L": ");
        wcscat(resultLine, wideContent);
        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)resultLine);
        
        current = current->next;
    }
    
    if (currentFile) {
        free(currentFile);
    }
    
    // Восстанавливаем исходный порядок для корректного освобождения памяти
    current = reversed;
    reversed = NULL;
    while (current) {
        SearchResult* next = current->next;
        current->next = reversed;
        reversed = current;
        current = next;
    }
}

// Очистка результатов
void ClearResults(HWND hwnd) {
    HWND hList = GetDlgItem(hwnd, IDC_RESULTS_LIST);
    HWND hStats = GetDlgItem(hwnd, IDC_STATS_TEXT);
    HWND hProgress = GetDlgItem(hwnd, IDC_PROGRESS_BAR);
    
    if (hList) SendMessage(hList, LB_RESETCONTENT, 0, 0);
    if (hStats) SetWindowTextW(hStats, L"Найдено: 0 вхождений в 0 файлах");
    if (hProgress) SendMessage(hProgress, PBM_SETPOS, 0, 0);
    
    freeSearchResults();
}

// Получение параметров поиска из элементов управления
int GetSearchParams(HWND hwnd, SearchParams* params) {
    if (!params) return 0;
    
    memset(params, 0, sizeof(SearchParams));
    
    // Получаем путь
    HWND hPath = GetDlgItem(hwnd, IDC_PATH_EDIT);
    if (hPath) {
        wchar_t widePath[MAX_PATH_LEN];
        GetWindowTextW(hPath, widePath, MAX_PATH_LEN);
        WideToMultiByteUTF8(widePath, params->searchPath, MAX_PATH_LEN);
    }
    
    if (params->searchPath[0] == '\0') {
        MessageBoxW(hwnd, L"Укажите путь для поиска!", L"Ошибка", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    // Получаем ключевые слова
    HWND hKeywords = GetDlgItem(hwnd, IDC_KEYWORDS_EDIT);
    if (hKeywords) {
        wchar_t wideKeywords[2048];
        GetWindowTextW(hKeywords, wideKeywords, 2048);
        char utf8Keywords[2048];
        WideToMultiByteUTF8(wideKeywords, utf8Keywords, 2048);
        params->keywordCount = parseCommaSeparated(utf8Keywords, params->keywords, MAX_KEYWORDS);
    }
    
    if (params->keywordCount == 0) {
        MessageBoxW(hwnd, L"Укажите ключевые слова для поиска!", L"Ошибка", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    // Получаем расширения
    HWND hExtensions = GetDlgItem(hwnd, IDC_EXTENSIONS_EDIT);
    if (hExtensions) {
        wchar_t wideExtensions[512];
        GetWindowTextW(hExtensions, wideExtensions, 512);
        char utf8Extensions[512];
        WideToMultiByteUTF8(wideExtensions, utf8Extensions, 512);
        // Используем временный массив с правильным размером для parseCommaSeparated
        char tempExtensions[MAX_EXTENSIONS][MAX_WORD_LEN];
        int count = parseCommaSeparated(utf8Extensions, tempExtensions, MAX_EXTENSIONS);
        params->extensionCount = 0;
        for (int i = 0; i < count && i < MAX_EXTENSIONS; i++) {
            strncpy(params->extensions[i], tempExtensions[i], MAX_EXT_LEN - 1);
            params->extensions[i][MAX_EXT_LEN - 1] = '\0';
            params->extensionCount++;
        }
    }
    
    // Получаем настройки чекбоксов
    params->recursive = (SendMessage(GetDlgItem(hwnd, IDC_RECURSIVE_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED);
    params->caseSensitive = (SendMessage(GetDlgItem(hwnd, IDC_CASE_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED);
    params->wholeWord = (SendMessage(GetDlgItem(hwnd, IDC_WHOLEWORD_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    return 1;
}

// Обработка нажатия кнопки поиска
void OnSearchButtonClick(HWND hwnd) {
    // Проверяем, не выполняется ли уже поиск
    if (g_searchThreadHandle != NULL) {
        DWORD exitCode;
        if (GetExitCodeThread(g_searchThreadHandle, &exitCode) && exitCode == STILL_ACTIVE) {
            MessageBoxW(hwnd, L"Поиск уже выполняется!", L"Информация", MB_OK | MB_ICONINFORMATION);
            return;
        }
        CloseHandle(g_searchThreadHandle);
        g_searchThreadHandle = NULL;
    }
    
    // Получаем параметры поиска
    if (!GetSearchParams(hwnd, &g_currentParams)) {
        return;
    }
    
    // Очищаем предыдущие результаты
    ClearResults(hwnd);
    
    // Создаём структуру для передачи в поток (используем ThreadData из search.h)
    ThreadData* data = (ThreadData*)malloc(sizeof(ThreadData));
    if (!data) {
        MessageBoxW(hwnd, L"Недостаточно памяти!", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }
    
    data->params = g_currentParams;
    data->hwnd = hwnd;
    
    // Создаём поток для поиска
    DWORD threadId;
    g_searchThreadHandle = CreateThread(NULL, 0, searchThread, data, 0, &threadId);
    
    if (!g_searchThreadHandle) {
        free(data);
        MessageBoxW(hwnd, L"Не удалось запустить поток поиска!", L"Ошибка", MB_OK | MB_ICONERROR);
        EnableWindow(GetDlgItem(hwnd, IDC_SEARCH_BTN), TRUE);
        return;
    }
    
    // Обновляем интерфейс
    EnableWindow(GetDlgItem(hwnd, IDC_SEARCH_BTN), FALSE);
    UpdateProgressBar(hwnd, 0);
    // Показываем прогресс-бар при начале поиска
    HWND hProgress = GetDlgItem(hwnd, IDC_PROGRESS_BAR);
    if (hProgress) {
        ShowWindow(hProgress, SW_SHOW);
    }
}

// Функция для очистки потока поиска (вызывается из main.c при закрытии)
void CleanupSearchThread(void) {
    if (g_searchThreadHandle != NULL) {
        g_searchCanceled = 1;
        WaitForSingleObject(g_searchThreadHandle, 5000);
        CloseHandle(g_searchThreadHandle);
        g_searchThreadHandle = NULL;
    }
}

// Обработка нажатия кнопки очистки
void OnClearButtonClick(HWND hwnd) {
    // Останавливаем поиск, если он выполняется
    if (g_searchThreadHandle != NULL) {
        g_searchCanceled = 1;
        WaitForSingleObject(g_searchThreadHandle, 5000);
        CloseHandle(g_searchThreadHandle);
        g_searchThreadHandle = NULL;
    }
    
    // Очищаем поля ввода
    SetWindowTextW(GetDlgItem(hwnd, IDC_PATH_EDIT), L"C:\\");
    SetWindowTextW(GetDlgItem(hwnd, IDC_KEYWORDS_EDIT), L"");
    SetWindowTextW(GetDlgItem(hwnd, IDC_EXTENSIONS_EDIT), L".txt, .c, .h, .cpp");
    
    // Сбрасываем чекбоксы
    SendMessage(GetDlgItem(hwnd, IDC_RECURSIVE_CHECK), BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(GetDlgItem(hwnd, IDC_CASE_CHECK), BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(GetDlgItem(hwnd, IDC_WHOLEWORD_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);
    
    // Очищаем результаты
    ClearResults(hwnd);
    
    // Разрешаем кнопку поиска
    EnableWindow(GetDlgItem(hwnd, IDC_SEARCH_BTN), TRUE);
}

// Экспорт результатов в файл
void ExportResults(HWND hwnd) {
    if (g_searchResults == NULL) {
        MessageBoxW(hwnd, L"Нет результатов для экспорта!", L"Информация", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    OPENFILENAMEW ofn = {0};
    wchar_t filename[MAX_PATH] = L"search_results.txt";
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Текстовые файлы\0*.txt\0Все файлы\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    
    if (!GetSaveFileNameW(&ofn)) {
        return; // Пользователь отменил
    }
    
    // Конвертируем имя файла в UTF-8
    char utf8Filename[MAX_PATH];
    WideToMultiByteUTF8(filename, utf8Filename, MAX_PATH);
    
    FILE* file = fopen(utf8Filename, "w");
    if (!file) {
        MessageBoxW(hwnd, L"Не удалось создать файл!", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Записываем заголовок
    fprintf(file, "Результаты поиска\n");
    fprintf(file, "==================\n\n");
    fprintf(file, "Путь: %s\n", g_currentParams.searchPath);
    
    fprintf(file, "Ключевые слова: ");
    for (int i = 0; i < g_currentParams.keywordCount; i++) {
        fprintf(file, "%s", g_currentParams.keywords[i]);
        if (i < g_currentParams.keywordCount - 1) fprintf(file, ", ");
    }
    fprintf(file, "\n");
    
    fprintf(file, "Расширения: ");
    for (int i = 0; i < g_currentParams.extensionCount; i++) {
        fprintf(file, "%s", g_currentParams.extensions[i]);
        if (i < g_currentParams.extensionCount - 1) fprintf(file, ", ");
    }
    fprintf(file, "\n\n");
    
    fprintf(file, "Всего найдено: %d вхождений в %d файлах\n\n", 
            g_searchStats.totalMatches, g_searchStats.matchedFiles);
    
    // Записываем результаты
    SearchResult* current = g_searchResults;
    char* currentFile = NULL;
    
    while (current) {
        if (!currentFile || strcmp(currentFile, current->filepath) != 0) {
            if (currentFile) free(currentFile);
            currentFile = strdup(current->filepath);
            fprintf(file, "\n=====================================\n");
            fprintf(file, "Файл: %s\n", current->filepath);
            fprintf(file, "=====================================\n");
        }
        
        fprintf(file, "Строка %d: %s\n", current->lineNumber, current->lineContent);
        current = current->next;
    }
    
    if (currentFile) free(currentFile);
    
    fclose(file);
    
    wchar_t msg[256];
    _snwprintf(msg, 256, L"Результаты успешно сохранены в файл:\n%s", filename);
    msg[255] = L'\0';
    MessageBoxW(hwnd, msg, L"Экспорт завершён", MB_OK | MB_ICONINFORMATION);
}

// Показать диалог "О программе"
void ShowAboutDialog(HWND hwnd) {
    MessageBoxW(hwnd,
        L"File Search Tool v1.0\n\n"
        L"Программа для поиска текста в файлах\n\n"
        L"Технологии:\n"
        L"- C (C99)\n"
        L"- Win32 API\n"
        L"- MinGW GCC\n\n"
        L"2025",
        L"О программе",
        MB_OK | MB_ICONINFORMATION);
}

