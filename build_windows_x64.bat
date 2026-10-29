@echo off
REM Builds the Windows x64 VST3.
REM JUCE is used from %JUCE_DIR%, .\JUCE, ..\JUCE or downloaded automatically.
setlocal
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release --target MIRRORA_VST3 --parallel
if errorlevel 1 exit /b 1
echo.
echo Built: build\MIRRORA_artefacts\Release\VST3\MIRRORA.vst3
