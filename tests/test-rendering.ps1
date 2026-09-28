param([Parameter(Mandatory=$true)][string]$Compiler,[Parameter(Mandatory=$true)][string]$Tftd,[Parameter(Mandatory=$true)][string]$OxceXcom2,[Parameter(Mandatory=$true)][string]$Mods,[Parameter(Mandatory=$true)][string]$Universal)
$ErrorActionPreference='Stop'
$exe=Join-Path $PSScriptRoot 'rendering.exe'
& $Compiler -std=c11 -O2 -Wall -Wextra -Werror (Join-Path $PSScriptRoot 'rendering.c') -o $exe -luser32 -lgdi32 -lcomdlg32 -lshell32 -lole32 -lgdiplus -lkernel32
if($LASTEXITCODE -ne 0){throw 'Compilation echouee'}
$run=Join-Path $PSScriptRoot 'run-rendering'
New-Item -ItemType Directory -Force (Join-Path $run 'work') | Out-Null
Push-Location $run
try{& $exe $Tftd $OxceXcom2 $Mods $Universal | Tee-Object -FilePath (Join-Path $PSScriptRoot "RENDERING_RESULTATS_LOCAL.txt");if($LASTEXITCODE -ne 0){throw 'Tests echoues'}}finally{Pop-Location}
