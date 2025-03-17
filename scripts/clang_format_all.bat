@echo off
setlocal enabledelayedexpansion

where clang-format>nul

if %errorlevel% neq 0 (
    echo Cannot find clang-format, exiting
    exit /B 1
) else (
    echo Found clang-format
)

set ROOT_DIR=%~dp0..
set DIRS=%ROOT_DIR%\src,%ROOT_DIR%\include\stdromano,%ROOT_DIR%\tests

call :Loop "%DIRS%"

exit /B 0

:Loop
set DIRS=%1
set DIRS=%DIRS:"=%
for /f "tokens=1* delims=," %%a in ("%DIRS%") do (
    if "%%a" neq "" call :ProcessDirectory %%a
    if "%%b" neq "" call :Loop %%b
)

exit /B 0

:ProcessDirectory
if exist "%~1" (
    for /R "%~1" %%f in (*) do (
        set FILEPATH=%%f

        if "!FILEPATH:~-4!" equ ".cpp" (
            echo Formatting "%%f"
            clang-format -i -style=file "%%f"
        )

        if "!FILEPATH:~-3!" equ ".cc" (
            echo Formatting "%%f"
            clang-format -i -style=file "%%f"
        )

        if "!FILEPATH:~-2!" equ ".c" (
            echo Formatting "%%f"
            clang-format -i -style=file "%%f"
        )

        if "!FILEPATH:~-2!" equ ".h" (
            echo Formatting "%%f"
            clang-format -i -style=file "%%f"
        )

        if "!FILEPATH:~-4!" equ ".hpp" (
            echo Formatting "%%f"
            clang-format -i -style=file "%%f"
        )
    )
)

exit /B 0