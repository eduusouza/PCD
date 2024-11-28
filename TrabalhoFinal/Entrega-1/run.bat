@echo off
setlocal enabledelayedexpansion

REM Nome do executável
set EXECUTABLE=difusao_omp.exe

REM Número de execuções
set ITERATIONS=5

echo Executando o programa %EXECUTABLE% %ITERATIONS% vezes...

for /L %%i in (1,1,%ITERATIONS%) do (
    echo.
    echo Execução %%i...
    REM Executar o programa
    %EXECUTABLE%
)
pause
