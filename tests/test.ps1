param([Parameter(Mandatory=$true)][string]$Compiler,[Parameter(Mandatory=$true)][string]$Tftd,[Parameter(Mandatory=$true)][string]$OxceXcom2)
$ErrorActionPreference = 'Stop'
$TestRoot = $PSScriptRoot
$TestOutput = Join-Path $TestRoot 'regression.exe'
& $Compiler -std=c11 -O2 -Wall -Wextra -Werror (Join-Path $TestRoot 'regression.c') -o $TestOutput -luser32 -lgdi32 -lcomdlg32 -lshell32 -lole32 -lgdiplus -lkernel32
if ($LASTEXITCODE -ne 0) { throw 'Compilation des tests echouee.' }
$RunRoot = Join-Path $TestRoot 'run'
New-Item -ItemType Directory -Force (Join-Path $RunRoot 'work') | Out-Null
Push-Location $RunRoot
try {
 & $TestOutput $Tftd $OxceXcom2 | Tee-Object -FilePath (Join-Path $TestRoot 'RESULTATS_LOCAL.txt')
 if ($LASTEXITCODE -ne 0) { throw 'Tests echoues.' }
} finally { Pop-Location }
