#ifndef GUI_H
#define GUI_H

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include "search.h"

// Идентификаторы элементов управления
#define IDC_PATH_EDIT           1001
#define IDC_BROWSE_BTN          1002
#define IDC_KEYWORDS_EDIT       1003
#define IDC_EXTENSIONS_EDIT     1004
#define IDC_RECURSIVE_CHECK     1005
#define IDC_CASE_CHECK          1006
#define IDC_WHOLEWORD_CHECK     1007
#define IDC_SEARCH_BTN          1008
#define IDC_CLEAR_BTN           1009
#define IDC_RESULTS_LIST        1010
#define IDC_PROGRESS_BAR        1011
#define IDC_EXPORT_BTN          1012
#define IDC_ABOUT_BTN           1013
#define IDC_STATS_TEXT          1014

// Размеры окна
#define WINDOW_WIDTH  700
#define WINDOW_HEIGHT 600
#define MIN_WIDTH     600
#define MIN_HEIGHT    500

// Создание главного окна
HWND CreateMainWindow(HINSTANCE hInstance);

// Создание элементов управления
void CreateControls(HWND hwnd);

// Обновление прогресс-бара
void UpdateProgressBar(HWND hwnd, int percent);

// Отображение результатов поиска
void DisplayResults(HWND hwnd);

// Очистка результатов
void ClearResults(HWND hwnd);

// Экспорт результатов в файл
void ExportResults(HWND hwnd);

// Показать диалог "О программе"
void ShowAboutDialog(HWND hwnd);

// Обработка нажатия кнопки поиска
void OnSearchButtonClick(HWND hwnd);

// Обработка нажатия кнопки очистки
void OnClearButtonClick(HWND hwnd);

// Получение параметров поиска из элементов управления
int GetSearchParams(HWND hwnd, SearchParams* params);

// Очистка потока поиска
void CleanupSearchThread(void);

#endif // GUI_H

