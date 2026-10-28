@echo off
setlocal
if "%JUCE_DIR%"=="" (echo Set JUCE_DIR first & exit /b 1)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release --target MIRRORA_VST3
