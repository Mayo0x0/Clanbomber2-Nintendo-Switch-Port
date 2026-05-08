@echo off
REM ----------------------------------------------------------------------------
REM Clanbomber Switch port - Windows build helper
REM ----------------------------------------------------------------------------
REM Sets up the devkitPro environment and runs `make`. Intended to be invoked
REM from a normal Windows cmd prompt (no MSYS2 shell needed).
REM ----------------------------------------------------------------------------

if "%DEVKITPRO%"=="" set DEVKITPRO=C:/devkitPro
if "%DEVKITA64%"=="" set DEVKITA64=%DEVKITPRO%/devkitA64
if "%PORTLIBS_PREFIX%"=="" set PORTLIBS_PREFIX=%DEVKITPRO%/portlibs/switch

REM gcc on Windows refuses to use MSYS-style /tmp paths. Force a real Windows
REM temp directory so the compiler can write its intermediate files.
if "%TMP%"=="" set TMP=%LOCALAPPDATA%\Temp
if "%TEMP%"=="" set TEMP=%LOCALAPPDATA%\Temp
if "%TMP:~0,1%"=="/" set TMP=%LOCALAPPDATA%\Temp
if "%TEMP:~0,1%"=="/" set TEMP=%LOCALAPPDATA%\Temp

set PATH=C:\devkitPro\devkitA64\bin;C:\devkitPro\tools\bin;C:\devkitPro\msys2\usr\bin;%PATH%

C:\devkitPro\msys2\usr\bin\make.exe DEVKITPRO=/c/devkitPro DEVKITA64=/c/devkitPro/devkitA64 PORTLIBS_PREFIX=/c/devkitPro/portlibs/switch %*
