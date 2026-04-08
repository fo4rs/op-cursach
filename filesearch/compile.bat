@echo off
setlocal enabledelayedexpansion
REM chcp 65001 >nul 2>&1
echo ======================================
echo Compilation File Search Tool
echo ======================================

REM Проверка наличия MinGW
where gcc >nul 2>nul
set MINGW_FOUND=!ERRORLEVEL!
if !MINGW_FOUND! NEQ 0 (
    echo ERROR: MinGW not found in PATH!
    echo Install MinGW-w64 and add to PATH
    pause
    exit /b 1
)

REM Создаём папку build, если её нет
if not exist build mkdir build

set CFLAGS=-Wall -O2 -std=c99 -DUNICODE -D_UNICODE -finput-charset=UTF-8 -fexec-charset=UTF-8

echo.
echo [1/4] Compiling resources...
set RESOURCE_FILE=
if not exist resources\resource.rc (
    echo WARNING: File resources\resource.rc not found (can be ignored)
)

if exist resources\resource.rc (
    if exist build\resource.res del build\resource.res
    where windres >nul 2>nul
    if !ERRORLEVEL! EQU 0 (
        windres resources\resource.rc -O coff -o build\resource.res 2>nul
        if !ERRORLEVEL! EQU 0 if exist build\resource.res (
            for %%A in (build\resource.res) do set RES_SIZE=%%~zA
            if defined RES_SIZE if !RES_SIZE! GTR 0 (
                set RESOURCE_FILE=build\resource.res
                echo Resources compiled successfully
            )
        )
        if not defined RESOURCE_FILE (
            echo WARNING: Failed to compile resources (can be ignored)
            if exist build\resource.res del build\resource.res 2>nul
        )
    ) else (
        echo WARNING: windres not found, resources will be skipped
    )
)

echo [2/4] Compiling utils.c...
gcc -c src\utils.c -o build\utils.o %CFLAGS%
set COMPILE_RESULT=!ERRORLEVEL!
if !COMPILE_RESULT! NEQ 0 (
    echo ERROR compiling utils.c
    pause
    exit /b 1
)

echo [3/4] Compiling search.c...
gcc -c src\search.c -o build\search.o %CFLAGS%
set COMPILE_RESULT=!ERRORLEVEL!
if !COMPILE_RESULT! NEQ 0 (
    echo ERROR compiling search.c
    pause
    exit /b 1
)

echo [4/4] Compiling gui.c and main.c...
gcc -c src\gui.c -o build\gui.o %CFLAGS%
set COMPILE_RESULT=!ERRORLEVEL!
if !COMPILE_RESULT! NEQ 0 (
    echo ERROR compiling gui.c
    pause
    exit /b 1
)

gcc -c src\main.c -o build\main.o %CFLAGS%
set COMPILE_RESULT=!ERRORLEVEL!
if !COMPILE_RESULT! NEQ 0 (
    echo ERROR compiling main.c
    pause
    exit /b 1
)

echo.
echo [Linking] Creating executable file...
set LINK_SUCCESS=0
if defined RESOURCE_FILE (
    gcc build\main.o build\gui.o build\search.o build\utils.o build\resource.res ^
        -o build\FileSearchTool.exe ^
        -mwindows -lcomctl32 -lgdi32 -lcomdlg32 -lshlwapi -lshell32 -lole32 -static-libgcc -static-libstdc++
    set LINK_RESULT=!ERRORLEVEL!
    if !LINK_RESULT! EQU 0 set LINK_SUCCESS=1
) else (
    gcc build\main.o build\gui.o build\search.o build\utils.o ^
        -o build\FileSearchTool.exe ^
        -mwindows -lcomctl32 -lgdi32 -lcomdlg32 -lshlwapi -lshell32 -lole32 -static-libgcc -static-libstdc++
    set LINK_RESULT=!ERRORLEVEL!
    if !LINK_RESULT! EQU 0 set LINK_SUCCESS=1
)

if !LINK_SUCCESS! EQU 1 (
    echo.
    echo ======================================
    echo SUCCESS!
    echo ======================================
    echo Executable file: build\FileSearchTool.exe
    if exist build\FileSearchTool.exe (
        echo.
        echo Run program? (Y/N)
        set /p choice=
        if /i "!choice!"=="Y" (
            echo Starting program...
            cd /d "%~dp0"
            start "" "build\FileSearchTool.exe"
            REM Команда start всегда возвращает успех, поэтому не проверяем ERRORLEVEL
        )
    ) else (
        echo.
        echo WARNING: Executable file not found!
    )
    goto :end
)

echo.
echo ======================================
echo COMPILATION ERROR!
echo ======================================

:end

pause

