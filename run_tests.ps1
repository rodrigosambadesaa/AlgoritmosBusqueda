$ErrorActionPreference = 'Stop'

$sources = @(
    'tests/test_suite.c',
    'dynamic_vector.c',
    'search.c',
    'quick_sort.c',
    'big_decimal.c'
)

$exeDir = 'build/tests'
$exePath = Join-Path $exeDir 'test_suite.exe'

New-Item -ItemType Directory -Force $exeDir | Out-Null

$needsBuild = -not (Test-Path $exePath)
if (-not $needsBuild) {
    $exeTime = (Get-Item $exePath).LastWriteTimeUtc
    foreach ($src in $sources) {
        if ((Get-Item $src).LastWriteTimeUtc -gt $exeTime) {
            $needsBuild = $true
            break
        }
    }
}

if ($needsBuild) {
    Write-Output 'Compilando tests...'
    gcc -O2 -std=c11 -Wall -Wextra -Werror -I. $sources -o $exePath
    if ($LASTEXITCODE -ne 0) {
        throw 'Fallo en compilacion de tests'
    }
}
else {
    Write-Output 'Binario de tests actualizado. Se omite compilacion.'
}

Write-Output 'Ejecutando tests...'
& $exePath
exit $LASTEXITCODE
