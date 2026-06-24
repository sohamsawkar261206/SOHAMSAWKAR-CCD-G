@echo off
REM ===========================================================================
REM RouteMaster Build Script
REM Ensures the MSYS2 MinGW-w64 bin is first on PATH to avoid DLL conflicts
REM with old 32-bit MinGW, Git's MinGW, or Anaconda's MinGW.
REM ===========================================================================

set CXX=C:\msys64\mingw64\bin\g++.exe
set MYSQL_INC=C:\Program Files\MySQL\MySQL Server 8.0\include
set MYSQL_LIB=C:\Program Files\MySQL\MySQL Server 8.0\lib
set FLAGS=-std=c++17 -Wall -Wextra -Iinclude -I"%MYSQL_INC%"
set LDFLAGS=-L"%MYSQL_LIB%" -lmysql -lws2_32

REM --- Prepend MSYS2 to PATH, remove conflicting MinGW entries ---
set PATH=C:\msys64\mingw64\bin;%PATH%

if not exist build mkdir build

echo === Compiling Models.cpp ===
"%CXX%" %FLAGS% -c -o build\Models.o src\Models.cpp
if %ERRORLEVEL% NEQ 0 (echo FAILED: Models.cpp & exit /b 1)

echo === Compiling DatabaseManager.cpp ===
"%CXX%" %FLAGS% -c -o build\DatabaseManager.o src\DatabaseManager.cpp
if %ERRORLEVEL% NEQ 0 (echo FAILED: DatabaseManager.cpp & exit /b 1)

echo === Compiling RouteOptimizer.cpp ===
"%CXX%" %FLAGS% -c -o build\RouteOptimizer.o src\RouteOptimizer.cpp
if %ERRORLEVEL% NEQ 0 (echo FAILED: RouteOptimizer.cpp & exit /b 1)

echo === Compiling UI_Views.cpp ===
"%CXX%" %FLAGS% -c -o build\UI_Views.o src\UI_Views.cpp
if %ERRORLEVEL% NEQ 0 (echo FAILED: UI_Views.cpp & exit /b 1)

echo === Compiling main.cpp ===
"%CXX%" %FLAGS% -c -o build\main.o src\main.cpp
if %ERRORLEVEL% NEQ 0 (echo FAILED: main.cpp & exit /b 1)

echo === Linking ===
"%CXX%" -o build\routemaster.exe build\Models.o build\DatabaseManager.o build\RouteOptimizer.o build\UI_Views.o build\main.o %LDFLAGS%
if %ERRORLEVEL% NEQ 0 (echo FAILED: Linking & exit /b 1)

echo.
echo === BUILD SUCCESSFUL ===
copy "%MYSQL_LIB%\libmysql.dll" build\ >nul 2>&1
echo Ready to run: build\routemaster.exe
