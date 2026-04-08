#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include "gui.h"
#include "search.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// Глобальные переменные
static HWND g_hwndMain = NULL;
static HINSTANCE g_hInstance = NULL;

// Обработчик сообщений окна
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateControls(hwnd);
            g_hwndMain = hwnd;
            return 0;
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            
            switch (wmId) {
                case IDC_SEARCH_BTN:
                    OnSearchButtonClick(hwnd);
                    break;
                
                case IDC_CLEAR_BTN:
                    OnClearButtonClick(hwnd);
                    break;
                
                case IDC_EXPORT_BTN:
                    ExportResults(hwnd);
                    break;
                
                case IDC_ABOUT_BTN:
                    ShowAboutDialog(hwnd);
                    break;
                
                case IDC_BROWSE_BTN: {
                    // Диалог выбора папки
                    BROWSEINFOW bi = {0};
                    wchar_t path[MAX_PATH];
                    
                    bi.hwndOwner = hwnd;
                    bi.pszDisplayName = path;
                    bi.lpszTitle = L"Выберите папку для поиска:";
                    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
                    
                    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
                    if (pidl) {
                        if (SHGetPathFromIDListW(pidl, path)) {
                            SetWindowTextW(GetDlgItem(hwnd, IDC_PATH_EDIT), path);
                        }
                        CoTaskMemFree(pidl);
                    }
                    break;
                }
                
                default:
                    return DefWindowProcW(hwnd, msg, wParam, lParam);
            }
            return 0;
        }
        
        case WM_SIZE: {
            // Изменение размера окна - обновляем размеры элементов управления
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            
            HWND hList = GetDlgItem(hwnd, IDC_RESULTS_LIST);
            HWND hProgress = GetDlgItem(hwnd, IDC_PROGRESS_BAR);
            HWND hPathEdit = GetDlgItem(hwnd, IDC_PATH_EDIT);
            HWND hBrowseBtn = GetDlgItem(hwnd, IDC_BROWSE_BTN);
            HWND hKeywordsEdit = GetDlgItem(hwnd, IDC_KEYWORDS_EDIT);
            HWND hExtensionsEdit = GetDlgItem(hwnd, IDC_EXTENSIONS_EDIT);
            
            if (hList) {
                SetWindowPos(hList, NULL, 10, 320, width - 30, height - 260, SWP_NOZORDER);
            }
            if (hProgress) {
                SetWindowPos(hProgress, NULL, 10, height - 70, width - 30, 20, SWP_NOZORDER);
            }
            if (hPathEdit) {
                SetWindowPos(hPathEdit, NULL, 10, 30, width - 130, 25, SWP_NOZORDER);
            }
            if (hBrowseBtn) {
                SetWindowPos(hBrowseBtn, NULL, width - 110, 30, 100, 25, SWP_NOZORDER);
            }
            if (hKeywordsEdit) {
                SetWindowPos(hKeywordsEdit, NULL, 10, 90, width - 30, 25, SWP_NOZORDER);
            }
            if (hExtensionsEdit) {
                SetWindowPos(hExtensionsEdit, NULL, 10, 150, width - 30, 25, SWP_NOZORDER);
            }
            
            return 0;
        }
        
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 600;
            mmi->ptMinTrackSize.y = 500;
            return 0;
        }
        
        case WM_USER + 1:
            // Сообщение об окончании поиска
            DisplayResults(hwnd);
            EnableWindow(GetDlgItem(hwnd, IDC_SEARCH_BTN), TRUE);
            // Сбрасываем прогресс-бар в 0 и скрываем его
            UpdateProgressBar(hwnd, 0);
            HWND hProgress = GetDlgItem(hwnd, IDC_PROGRESS_BAR);
            if (hProgress) {
                ShowWindow(hProgress, SW_HIDE);
            }
            return 0;
        
        case WM_DESTROY:
            // Останавливаем поиск при закрытии
            CleanupSearchThread();
            freeSearchResults();
            PostQuitMessage(0);
            return 0;
        
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// Точка входа приложения
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;
    
    // Инициализируем Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Инициализируем COM для SHBrowseForFolder
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    // Создаём главное окно
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FileSearchToolWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Не удалось зарегистрировать класс окна!", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    HWND hwnd = CreateWindowExW(
        0,
        L"FileSearchToolWindow",
        L"Поиск текста в файлах",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwnd) {
        MessageBoxW(NULL, L"Не удалось создать окно!", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Цикл сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    CoUninitialize();
    
    return (int)msg.wParam;
}

