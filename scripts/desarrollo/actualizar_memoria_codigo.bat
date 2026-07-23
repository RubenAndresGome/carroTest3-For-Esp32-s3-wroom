@echo off
setlocal
for %%I in ("%~dp0..\..") do set "REPO_ROOT=%%~fI"
set "MEMORY_EXE="
for /f "delims=" %%I in ('where codebase-memory-mcp.exe 2^>nul') do if not defined MEMORY_EXE set "MEMORY_EXE=%%I"
if not defined MEMORY_EXE set "MEMORY_EXE=%LOCALAPPDATA%\Programs\codebase-memory-mcp\codebase-memory-mcp.exe"
if not exist "%MEMORY_EXE%" (
  echo No se encontro codebase-memory-mcp. La indexacion es opcional.
  exit /b 1
)
pushd "%REPO_ROOT%"
echo Actualizando grafo de dependencias...
"%MEMORY_EXE%" cli index_repository "{\"repo_path\": \".\"}"
set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%
