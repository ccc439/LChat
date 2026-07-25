@echo off
setlocal enabledelayedexpansion

echo ============================================
echo   LChat - One-Click Package Script
echo ============================================
echo.

:: ====== CONFIG - Edit these paths to match your environment ======
set "QT_BIN_DIR=D:\Qt_Creator_18.0.1\6.11.1\mingw_64\bin"
set "PROJECT_DIR=D:\git_project\Qt\LChat"
set "BUILD_DIR=%PROJECT_DIR%\build\Desktop_Qt_6_11_1_MinGW_64_bit_Release"
set "EXE_PATH=%BUILD_DIR%\bin\LChat.exe"
set "OUTPUT_DIR=%~dp0LChat_Package"
set "FINAL_EXE_NAME=%~dp0LChat_Portable.exe"

:: ====== Locate Enigma Virtual Box ======
set "ENIGMA_EXE="

:: 1) Check PATH
for %%X in (enigmavbconsole.exe) do set "ENIGMA_EXE=%%~$PATH:X"

:: 2) Check %ProgramFiles% and %ProgramFiles(x86)% (works on any drive)
if "%ENIGMA_EXE%"=="" (
    for %%D in ("%ProgramFiles%" "%ProgramFiles(x86)%") do (
        if exist "%%~D\Enigma Virtual Box\enigmavbconsole.exe" set "ENIGMA_EXE=%%~D\Enigma Virtual Box\enigmavbconsole.exe"
    )
)

:: 3) Search common drive roots (C: through G:)
if "%ENIGMA_EXE%"=="" (
    for %%L in (C D E F G) do (
        if exist "%%L:\Enigma Virtual Box\enigmavbconsole.exe" set "ENIGMA_EXE=%%L:\Enigma Virtual Box\enigmavbconsole.exe"
    )
)

:: 4) Ask user to browse if still not found
if "%ENIGMA_EXE%"=="" (
    echo +============================================+
    echo ^|  Enigma Virtual Box not auto-detected     ^|
    echo ^|  Download: https://enigmaprotector.com     ^|
    echo +============================================+
    echo.
    echo Enter the full path to enigmavbconsole.exe
    echo   e.g. D:\Enigma Virtual Box\enigmavbconsole.exe
    echo   Or press Enter to skip single-EXE packing.
    echo.
    set /p ENIGMA_EXE="Path: "
    if "!ENIGMA_EXE!"=="" (
        echo   Skipping single-EXE support.
    ) else if not exist "!ENIGMA_EXE!" (
        echo   File not found, skipping.
        set "ENIGMA_EXE="
    ) else (
        echo   Found: !ENIGMA_EXE!
    )
    echo.
)

echo [1/5] Preparing output directory...
if exist "%OUTPUT_DIR%" (
    echo   Removing old: %OUTPUT_DIR%
    rmdir /s /q "%OUTPUT_DIR%"
)
mkdir "%OUTPUT_DIR%"
echo   Created: %OUTPUT_DIR%
echo.

echo [2/5] Copying main executable...
if not exist "%EXE_PATH%" (
    echo   [ERROR] LChat.exe not found!
    echo   Please build the project in Release mode first
    echo   Expected: %EXE_PATH%
    pause
    exit /b 1
)
copy /Y "%EXE_PATH%" "%OUTPUT_DIR%\" >nul
echo   LChat.exe copied
echo.

echo [3/5] Copying config and resources...
if exist "%PROJECT_DIR%\config.ini" (
    copy /Y "%PROJECT_DIR%\config.ini" "%OUTPUT_DIR%\" >nul
    echo   config.ini copied
) else (
    echo   [WARN] config.ini not found, skipping
)

if exist "%BUILD_DIR%\bin\static" (
    xcopy /Y /E /I "%BUILD_DIR%\bin\static" "%OUTPUT_DIR%\static\" >nul
    echo   static/ folder copied
) else (
    echo   [WARN] static/ folder not found, skipping
)
echo.

echo [4/5] Running windeployqt to collect Qt dependencies...
if not exist "%QT_BIN_DIR%\windeployqt.exe" (
    echo   [ERROR] windeployqt.exe not found!
    echo   Please edit QT_BIN_DIR in this script to your Qt install path
    pause
    exit /b 1
)
"%QT_BIN_DIR%\windeployqt.exe" "%OUTPUT_DIR%\LChat.exe"
echo.

echo [5/5] Copying MinGW runtime DLLs...
for %%F in ("libstdc++-6.dll" "libgcc_s_seh-1.dll" "libwinpthread-1.dll") do (
    if exist "%QT_BIN_DIR%\%%~F" (
        copy /Y "%QT_BIN_DIR%\%%~F" "%OUTPUT_DIR%\" >nul
        echo   %%~F copied
    ) else (
        echo   [WARN] %%~F not found, may not be needed
    )
)
echo.

echo ============================================
echo   Packaging complete!
echo   Output: %OUTPUT_DIR%
echo   Run: %OUTPUT_DIR%\LChat.exe
echo ============================================
echo.

:: Test run
choice /C YN /T 10 /D Y /M "Test run LChat.exe now? [Y,N] (10s default=Y)"
if errorlevel 2 goto skip_test
echo   Launching LChat.exe ...
start "" "%OUTPUT_DIR%\LChat.exe"

:skip_test
echo.

:: ====== Single EXE packing (Enigma Virtual Box) ======
if "%ENIGMA_EXE%"=="" goto :done

choice /C YN /T 10 /D Y /M "Pack into a single .exe file? [Y,N] (10s default=Y)"
if errorlevel 2 goto done

echo.
echo [6/6] Generating EVB project and building single EXE...

:: Generate .evb file using Python
set "EVB_FILE=%~dp0LChat_pack.evb"
python "%~dp0package_tool\generate_evb.py" "%EVB_FILE%" "%OUTPUT_DIR%\LChat.exe" "%FINAL_EXE_NAME%" "%OUTPUT_DIR%"
if errorlevel 1 (
    echo   [FAIL] Failed to generate EVB project file
    goto done
)

:: Run Enigma Virtual Box console
echo   Running Enigma Virtual Box...
"%ENIGMA_EXE%" "%EVB_FILE%"
del "%EVB_FILE%" 2>nul

if exist "%FINAL_EXE_NAME%" (
    echo.
    echo ============================================
    echo   Single EXE created!
    echo   File: %FINAL_EXE_NAME%
    echo ============================================
) else (
    echo   [FAIL] Single-EXE packing failed.
    echo   You can also manually use the GUI:
    echo     1. Open enigmavb.exe
    echo     2. Input: %OUTPUT_DIR%\LChat.exe
    echo     3. Output: %FINAL_EXE_NAME%
    echo     4. Add Folder: %OUTPUT_DIR%
    echo     5. Click Process
)

:done
echo.
echo All done. Press any key to exit...
pause >nul
