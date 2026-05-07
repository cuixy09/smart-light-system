@echo off
title smart-light-system

echo ============================
echo      SMART LIGHT SYSTEM
echo ============================
echo.

echo Compiling...
g++ main.cpp -o smart-light.exe -O2 -std=c++17 -I"D:\Python314\include" -I. -L"D:\Python314\libs" -lpython314 -lws2_32
if %errorlevel% neq 0 (
    echo.
    echo Compile Failed.
    echo %errorlevel%
    pause
    exit /b 1
)

echo Success!
echo Running...

start /wait smart-light.exe