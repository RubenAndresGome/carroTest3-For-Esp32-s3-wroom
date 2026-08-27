@echo off
setlocal
for %%I in ("%~dp0..\..") do set "REPO_ROOT=%%~fI"
set "REPO_MCP=%REPO_ROOT:\=/%"
set "MEMORY_EXE="
if exist "%USERPROFILE%\.local\bin\codebase-memory-mcp.exe" set "MEMORY_EXE=%USERPROFILE%\.local\bin\codebase-memory-mcp.exe"
for /f "delims=" %%I in ('where codebase-memory-mcp.exe 2^>nul') do if not defined MEMORY_EXE set "MEMORY_EXE=%%I"
if not defined MEMORY_EXE set "MEMORY_EXE=%LOCALAPPDATA%\Programs\codebase-memory-mcp\codebase-memory-mcp.exe"
if not exist "%MEMORY_EXE%" (
  echo No se encontro codebase-memory-mcp.
  echo Instale y registre el servidor con: codebase-memory-mcp install -y
  exit /b 1
)
pushd "%REPO_ROOT%"
echo Indexando memoria de codigo para: %REPO_ROOT%
"%MEMORY_EXE%" cli index_repository --repo-path "%REPO_MCP%" --mode full
set "RESULT=%ERRORLEVEL%"
if "%RESULT%"=="0" (
  echo Verificando que el proyecto persista en la cache local...
  "%MEMORY_EXE%" cli list_projects --include-details true
  set "RESULT=%ERRORLEVEL%"
)
popd
exit /b %RESULT%
