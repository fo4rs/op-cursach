# Инструкция по установке MinGW-w64

## Способ 1: Автоматическая установка (рекомендуется)

1. **Установите WinGet** (если ещё не установлен):
   - Откройте Microsoft Store
   - Найдите "App Installer" и установите/обновите его
   - WinGet входит в App Installer

2. **Запустите скрипт установки**:
   ```powershell
   # Откройте PowerShell от имени администратора
   # Выполните:
   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
   .\install_mingw.ps1
   ```

## Способ 2: Ручная установка через установщик

### Вариант A: Прямая установка MinGW-w64

1. **Скачайте установщик**:
   - Перейдите на: https://sourceforge.net/projects/mingw-w64/files/
   - Или прямую ссылку: https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/

2. **Установите**:
   - Распакуйте архив в `C:\mingw64`
   - Или используйте установщик (если доступен)

3. **Добавьте в PATH**:
   - Откройте "Переменные среды":
     - Win+R → `sysdm.cpl` → вкладка "Дополнительно" → "Переменные среды"
     - Или: Панель управления → Система → Дополнительные параметры системы → Переменные среды
   - В разделе "Системные переменные" найдите `Path` → "Изменить"
   - Добавьте путь: `C:\mingw64\bin` (или путь, куда вы установили)
   - Нажмите OK

4. **Проверьте установку**:
   - Откройте новую командную строку/PowerShell
   - Выполните: `gcc --version`
   - Должна отобразиться версия GCC

### Вариант B: Установка через MSYS2 (рекомендуется для разработчиков)

1. **Скачайте MSYS2**:
   - Сайт: https://www.msys2.org/
   - Прямая ссылка: https://github.com/msys2/msys2-installer/releases

2. **Установите MSYS2**:
   - Запустите установщик
   - Установите в `C:\msys64` (по умолчанию)

3. **Установите MinGW-w64 через MSYS2**:
   - Откройте MSYS2 MSYS (зелёное окно)
   - Выполните команды:
     ```bash
     pacman -Syu
     pacman -S mingw-w64-x86_64-gcc
     pacman -S mingw-w64-x86_64-binutils
     ```

4. **Добавьте в PATH**:
   - Добавьте путь: `C:\msys64\mingw64\bin`
   - (Инструкции по добавлению в PATH см. выше)

## Способ 3: Через Chocolatey (если установлен)

```powershell
choco install mingw -y
```

## Проверка установки

После установки и добавления в PATH:

1. **Откройте новую командную строку/PowerShell**
   (ВАЖНО: перезапустите, чтобы обновился PATH)

2. **Проверьте GCC**:
   ```bash
   gcc --version
   ```

3. **Проверьте windres** (для компиляции ресурсов):
   ```bash
   windres --version
   ```

## Решение проблем

### GCC не найден после установки

1. **Проверьте PATH**:
   ```powershell
   $env:Path -split ';' | Select-String -Pattern "mingw"
   ```

2. **Найдите папку bin вручную**:
   - Обычно находится в:
     - `C:\mingw64\bin`
     - `C:\msys64\mingw64\bin`
     - `C:\Program Files\mingw-w64\*\bin`

3. **Добавьте путь в PATH** (см. инструкцию выше)

4. **Перезапустите терминал**

### Ошибка "gcc: command not found"

- Убедитесь, что путь к `bin` добавлен в системную переменную PATH
- Перезапустите все открытые терминалы
- Проверьте, что файл `gcc.exe` существует в папке `bin`

### Ошибки при компиляции

- Убедитесь, что установлена 64-битная версия: `x86_64` или `x86_64-w64-mingw32`
- Проверьте, что установлены все необходимые библиотеки

## Быстрая проверка

Выполните в PowerShell:

```powershell
# Проверка GCC
if (Get-Command gcc -ErrorAction SilentlyContinue) {
    Write-Host "✓ GCC установлен" -ForegroundColor Green
    gcc --version
} else {
    Write-Host "✗ GCC не найден в PATH" -ForegroundColor Red
}

# Проверка windres
if (Get-Command windres -ErrorAction SilentlyContinue) {
    Write-Host "✓ windres установлен" -ForegroundColor Green
} else {
    Write-Host "⚠ windres не найден" -ForegroundColor Yellow
}
```

## После установки

После успешной установки MinGW-w64 вы сможете:

1. Запустить `compile.bat` для компиляции проекта
2. Или компилировать вручную через командную строку

---

**Примечание**: После изменения PATH обязательно перезапустите терминал!

