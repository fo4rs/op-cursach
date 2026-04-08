# Скрипт установки MinGW-w64 через WinGet
# Запустите от имени администратора: PowerShell -ExecutionPolicy Bypass -File install_mingw.ps1

Write-Host "======================================" -ForegroundColor Cyan
Write-Host "Установка MinGW-w64" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

# Проверяем наличие WinGet
$wingetExists = Get-Command winget -ErrorAction SilentlyContinue

if (-not $wingetExists) {
    Write-Host "ОШИБКА: WinGet не найден!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Установите WinGet одним из способов:" -ForegroundColor Yellow
    Write-Host "1. Обновите Windows до версии 10 1809 или выше"
    Write-Host "2. Установите вручную с Microsoft Store: App Installer"
    Write-Host ""
    Write-Host "Или используйте ручную установку:" -ForegroundColor Yellow
    Write-Host "1. Скачайте с: https://www.mingw-w64.org/downloads/"
    Write-Host "2. Или используйте MSYS2: https://www.msys2.org/"
    Write-Host ""
    pause
    exit 1
}

Write-Host "[1/3] Проверка наличия MinGW-w64..." -ForegroundColor Yellow
$mingwInstalled = winget list -e "MinGW-w64" 2>$null

if ($mingwInstalled -match "MinGW-w64") {
    Write-Host "MinGW-w64 уже установлен!" -ForegroundColor Green
    Write-Host ""
} else {
    Write-Host "[2/3] Установка MinGW-w64..." -ForegroundColor Yellow
    Write-Host "Это может занять несколько минут..." -ForegroundColor Gray
    Write-Host ""
    
    # Пробуем установить через winget
    winget install -e --id "mingw-w64" --accept-source-agreements --accept-package-agreements
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "✓ MinGW-w64 успешно установлен!" -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "✗ Ошибка установки через WinGet" -ForegroundColor Red
        Write-Host ""
        Write-Host "Попробуйте установить вручную:" -ForegroundColor Yellow
        Write-Host "1. Скачайте установщик с https://sourceforge.net/projects/mingw-w64/files/"
        Write-Host "2. Или используйте MSYS2: https://www.msys2.org/"
        Write-Host ""
        pause
        exit 1
    }
}

Write-Host "[3/3] Проверка установки..." -ForegroundColor Yellow

# Обновляем PATH в текущей сессии
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

# Ищем gcc в возможных местах
$possiblePaths = @(
    "C:\mingw64\bin",
    "C:\msys64\mingw64\bin",
    "C:\Program Files\mingw-w64\*\bin",
    "$env:USERPROFILE\mingw64\bin"
)

$gccFound = $false
foreach ($path in $possiblePaths) {
    $resolvedPath = Resolve-Path $path -ErrorAction SilentlyContinue
    if ($resolvedPath) {
        $gccPath = Join-Path $resolvedPath[0] "gcc.exe"
        if (Test-Path $gccPath) {
            Write-Host "✓ GCC найден: $gccPath" -ForegroundColor Green
            Write-Host ""
            Write-Host "ВАЖНО: Добавьте в PATH путь к MinGW bin:" -ForegroundColor Yellow
            Write-Host $resolvedPath[0] -ForegroundColor Cyan
            Write-Host ""
            $gccFound = $true
            break
        }
    }
}

if (-not $gccFound) {
    Write-Host "⚠ GCC не найден в стандартных местах" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Необходимо добавить путь к MinGW\bin в PATH вручную:" -ForegroundColor Yellow
    Write-Host "1. Откройте 'Переменные среды' (sysdm.cpl -> Дополнительно -> Переменные среды)"
    Write-Host "2. Добавьте путь к папке bin MinGW-w64 в переменную Path"
    Write-Host "   Обычно это: C:\mingw64\bin или C:\msys64\mingw64\bin"
    Write-Host "3. Перезапустите командную строку/PowerShell"
    Write-Host ""
}

Write-Host "======================================" -ForegroundColor Cyan
Write-Host "Установка завершена!" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "После добавления в PATH выполните:" -ForegroundColor Yellow
Write-Host "gcc --version" -ForegroundColor Cyan
Write-Host ""
pause

