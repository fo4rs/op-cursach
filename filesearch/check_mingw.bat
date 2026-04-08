@echo off
chcp 65001 >nul
echo ======================================
echo Проверка установки MinGW-w64
echo ======================================
echo.

REM Проверка GCC
where gcc >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] GCC найден:
    gcc --version
    echo.
) else (
    echo [ОШИБКА] GCC не найден в PATH!
    echo.
)

REM Проверка windres
where windres >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] windres найден:
    windres --version
    echo.
) else (
    echo [ПРЕДУПРЕЖДЕНИЕ] windres не найден в PATH
    echo (может потребоваться для компиляции ресурсов)
    echo.
)

REM Поиск возможных путей установки
echo Поиск MinGW в стандартных местах:
echo.

if exist "C:\mingw64\bin\gcc.exe" (
    echo [НАЙДЕНО] C:\mingw64\bin\gcc.exe
    echo   Добавьте в PATH: C:\mingw64\bin
    echo.
)

if exist "C:\msys64\mingw64\bin\gcc.exe" (
    echo [НАЙДЕНО] C:\msys64\mingw64\bin\gcc.exe
    echo   Добавьте в PATH: C:\msys64\mingw64\bin
    echo.
)

if exist "%USERPROFILE%\mingw64\bin\gcc.exe" (
    echo [НАЙДЕНО] %USERPROFILE%\mingw64\bin\gcc.exe
    echo   Добавьте в PATH: %USERPROFILE%\mingw64\bin
    echo.
)

echo ======================================
if %ERRORLEVEL% EQU 0 (
    echo Статус: MinGW-w64 готов к использованию!
) else (
    echo Статус: MinGW-w64 не установлен или не найден в PATH
    echo.
    echo Инструкции по установке см. в файле: install_mingw.md
)
echo ======================================
echo.
pause

