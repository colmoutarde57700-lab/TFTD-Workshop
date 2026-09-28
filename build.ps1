param([Parameter(Mandatory=$true)][string]$Compiler)
$ErrorActionPreference = 'Stop'
$WorkshopRoot = $PSScriptRoot
# Generated headers are included for users without Python; regenerate after catalogue edits.
& $Compiler -std=c11 -O2 -Wall -Wextra -Werror -mwindows "$WorkshopRoot/src/tftd_workshop_v2_12_1.c" -o "$WorkshopRoot/TFTD_Workshop_V2.12.7_Rendu_Mixte.exe" -luser32 -lgdi32 -lcomdlg32 -lshell32 -lole32 -lgdiplus -lkernel32
if ($LASTEXITCODE -ne 0) { throw 'Compilation du Workshop echouee.' }
