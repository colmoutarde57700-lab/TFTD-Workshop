param([Parameter(Mandatory=$true)][string]$Compiler,[Parameter(Mandatory=$true)][string]$Tftd,[Parameter(Mandatory=$true)][string]$OxceXcom2,[Parameter(Mandatory=$true)][string]$MirrorMod)
$ErrorActionPreference = 'Stop'
$TestOutput = Join-Path $PSScriptRoot 'procedural.exe'
& $Compiler -std=c11 -O2 -Wall -Wextra -Werror (Join-Path $PSScriptRoot 'procedural.c') -o $TestOutput -luser32 -lgdi32 -lcomdlg32 -lshell32 -lole32 -lgdiplus -lkernel32
if ($LASTEXITCODE -ne 0) { throw 'Compilation echouee.' }
$RunRoot = Join-Path $PSScriptRoot 'run-procedural'
New-Item -ItemType Directory -Force (Join-Path $RunRoot 'work') | Out-Null
Push-Location $RunRoot
try {
 & $TestOutput $Tftd $OxceXcom2 $MirrorMod | Tee-Object -FilePath (Join-Path $PSScriptRoot 'PROCEDURAL_RESULTATS_LOCAL.txt')
 if ($LASTEXITCODE -ne 0) { throw 'Tests echoues.' }
} finally { Pop-Location }
