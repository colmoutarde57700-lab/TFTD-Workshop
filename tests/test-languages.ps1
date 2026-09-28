param([Parameter(Mandatory=$true)][string]$Compiler)
$ErrorActionPreference='Stop'
$exe=Join-Path $PSScriptRoot 'languages.exe'
& $Compiler -std=c11 -O2 -Wall -Wextra -Werror (Join-Path $PSScriptRoot 'languages.c') -o $exe -luser32 -lgdi32 -lcomdlg32 -lshell32 -lole32 -lgdiplus -lkernel32
if($LASTEXITCODE -ne 0){throw 'Compilation echouee'}
$run=Join-Path $PSScriptRoot 'run-languages'
New-Item -ItemType Directory -Force (Join-Path $run 'work') | Out-Null
Push-Location $run
try {
 & $exe
 if($LASTEXITCODE -ne 0){throw 'Tests echoues'}
 foreach($l in 0..3){& $exe write $l; & $exe read $l;if($LASTEXITCODE -ne 0){throw 'Preference echouee'}}
} finally {Pop-Location}
