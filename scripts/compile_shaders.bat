@echo off
setlocal enabledelayedexpansion

set SHADER_DIR=src/shader
set OUTPUT_DIR=src/shader/spv


if not defined VULKAN_SDK (
    echo VULKAN_SDK environment variable is not set.
    exit /b 1
)
set GLSLANG_VALIDATOR=%VULKAN_SDK%\Bin\glslangValidator.exe

if not exist "%GLSLANG_VALIDATOR%" (
    echo glslangValidator not found at %GLSLANG_VALIDATOR%
    exit /b 1
)

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

for /r "%SHADER_DIR%" %%f in (*.vert *.frag *.comp) do (

    set FILE_PATH=%%f
    set FILE_NAME=%%~nxf

    "%GLSLANG_VALIDATOR%" -V "!FILE_PATH!" -o "%OUTPUT_DIR%\!FILE_NAME!.spv"

    if errorlevel 1 (
        echo Failed to compile !FILE_PATH!
        exit /b 1
    ) else (
        echo Compiled !FILE_PATH! to %OUTPUT_DIR%\!FILE_NAME!.spv
    )
)

echo All shaders compiled successfully.