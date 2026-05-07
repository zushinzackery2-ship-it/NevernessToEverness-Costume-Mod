@echo off
setlocal
set SCRIPT_DIR=%~dp0.
set SDK=%SCRIPT_DIR%\..\[x64]Rei-Universal-Dumper\Rei-SdkDump\5.6.1.0-HTGame\CppSDK
set OUT=%SCRIPT_DIR%\bin
set OBJ=%SCRIPT_DIR%\obj

call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 exit /b 1

if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OBJ%" mkdir "%OBJ%"
set PCH=%OBJ%\cfsetter.pch
if exist "%PCH%" del /F /Q "%PCH%"
if exist "%OBJ%\Pch.obj" del /F /Q "%OBJ%\Pch.obj"

echo === PCH ===
cl /nologo /std:c++20 /EHsc /O2 /MT /W3 /DNDEBUG /I"%SCRIPT_DIR%" /I"%SDK%" /I"%SDK%\SDK" /YcPch.h /Fp"%PCH%" /Fo"%OBJ%\Pch.obj" /c "%SCRIPT_DIR%\Pch.cpp"
if errorlevel 1 exit /b 1

echo === COMPILE ===
cl /nologo /std:c++20 /EHsc /O2 /MT /W3 /DNDEBUG /I"%SCRIPT_DIR%" /I"%SDK%" /I"%SDK%\SDK" /YuPch.h /FI"Pch.h" /Fp"%PCH%" /Fo"%OBJ%/" /c "%SCRIPT_DIR%\CurrentFashionSetter.cpp" "%SCRIPT_DIR%\AppearanceData.cpp" "%SCRIPT_DIR%\FashionSetter.cpp" "%SCRIPT_DIR%\Log.cpp" "%SCRIPT_DIR%\PostRenderRunner.cpp" "%SCRIPT_DIR%\SdkGlue.cpp" "%SCRIPT_DIR%\SdkRuntimeHelpers.cpp" "%SCRIPT_DIR%\AntiFadeMod\CameraPatchInspect.cpp" "%SCRIPT_DIR%\AntiFadeMod\CameraRuntimeCalls.cpp" "%SCRIPT_DIR%\AntiFadeMod\CameraSourcePatch.cpp" "%SCRIPT_DIR%\AntiFadeMod\SdkHelpers.cpp" "%SDK%\SDK\CoreUObject_functions.cpp"
if errorlevel 1 exit /b 1

echo === LINK ===
set DLL=%OUT%\Rei-CurrentFashionSetter.dll
set PDB=%OUT%\Rei-CurrentFashionSetter.pdb
link /nologo /DLL /OUT:"%DLL%" /PDB:"%PDB%" "%OBJ%\CurrentFashionSetter.obj" "%OBJ%\AppearanceData.obj" "%OBJ%\FashionSetter.obj" "%OBJ%\Log.obj" "%OBJ%\PostRenderRunner.obj" "%OBJ%\SdkGlue.obj" "%OBJ%\SdkRuntimeHelpers.obj" "%OBJ%\CameraPatchInspect.obj" "%OBJ%\CameraRuntimeCalls.obj" "%OBJ%\CameraSourcePatch.obj" "%OBJ%\SdkHelpers.obj" "%OBJ%\CoreUObject_functions.obj" "%OBJ%\Pch.obj" kernel32.lib user32.lib
if errorlevel 1 exit /b 1

echo === SUCCESS ===
echo Output: %DLL%
exit /b 0
