@echo off
setlocal enabledelayedexpansion

REM Nome do executável
set EXECUTABLE=difusao_omp.exe

REM Número de execuções
set ITERATIONS=5

REM Variável para armazenar o tempo total
set TOTAL_TIME=0

echo Executando o programa %EXECUTABLE% %ITERATIONS% vezes...

for /L %%i in (1,1,%ITERATIONS%) do (
    echo.
    echo Execução %%i...

    REM Obter o tempo inicial em milissegundos
    for /f %%t in ('powershell -command "Get-Date -Format \"HH:mm:ss.fff\" "') do set START_TIME=%%t

    REM Executar o programa
    %EXECUTABLE%

    REM Obter o tempo final em milissegundos
    for /f %%t in ('powershell -command "Get-Date -Format \"HH:mm:ss.fff\" "') do set END_TIME=%%t

    REM Converter tempos para milissegundos e calcular a diferença
    for /f "tokens=1,2,3 delims=:." %%h in ("%START_TIME%") do (
        set /a START_MS=%%h*3600000 + %%i*60000 + %%j*1000 + %%k
    )
    for /f "tokens=1,2,3 delims=:." %%h in ("%END_TIME%") do (
        set /a END_MS=%%h*3600000 + %%i*60000 + %%j*1000 + %%k
    )

    set /a TIME_DIFF=!END_MS! - !START_MS!
    echo Tempo da execução %%i: !TIME_DIFF! ms

    REM Adicionar ao total
    set /a TOTAL_TIME+=!TIME_DIFF!
)

REM Calculando a média
set /a AVERAGE_TIME=%TOTAL_TIME% / %ITERATIONS%

echo.
echo Tempo médio de execução: %AVERAGE_TIME% ms

pause
