@echo off
setlocal
set "SCRIPT_DIR=%~dp0.."
for %%I in ("%SCRIPT_DIR%\..\[x64]Rei-Universal-Dumper\Rei-SdkDump\5.6.1.0-HTGame\CppSDK") do set "SDK=%%~fI"
set "OBJ=%SCRIPT_DIR%\obj"
call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 exit /b 1
if not exist "%OBJ%" mkdir "%OBJ%"
cl /nologo /std:c++20 /EHsc /O2 /MT /W3 /DNDEBUG /I"%SCRIPT_DIR%" /I"%SDK%" /I"%SDK%\SDK" /Fo"%OBJ%\layout_compile_probe.obj" /c "%SCRIPT_DIR%\tools\layout_compile_probe.cpp"
if errorlevel 1 exit /b 1
echo Layout compile probe succeeded
exit /b 0
