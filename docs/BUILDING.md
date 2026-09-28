# Build and test
Windows x64 with LLVM-MinGW (tested with llvm-mingw 20260826 UCRT x86_64).
Run in PowerShell:
./build.ps1 -Compiler C:/toolchain/bin/x86_64-w64-mingw32-clang.exe

Generated C headers are committed; Python is optional for a direct build.
After editing locales or docs/chapters.json: python tools/regenerate.py, then rebuild.
The tool checks all four catalogues and printf parameters and regenerates native and offline documentation.

Tests, using your own resource paths:
./tests/test.ps1 -Compiler <compiler> -Tftd <TFTD> -OxceXcom2 <standard/xcom2>
./tests/test-procedural.ps1 -Compiler <compiler> -Tftd <TFTD> -OxceXcom2 <standard/xcom2> -MirrorMod <JM_USO_SYMETRIE>
./tests/test-rendering.ps1 -Compiler <compiler> -Tftd <TFTD> -OxceXcom2 <standard/xcom2> -Mods <user/mods> -Universal <universal PNG root>
./tests/test-languages.ps1 -Compiler <compiler>

Regression/procedural/rendering tests require local game files and the relevant packs. Rendering tests require remastered/universal PNGs plus normal/debug REAL HD maps.
No game fixtures are bundled. Do not interpret a missing local fixture as proof of game compatibility or incompatibility.
Test output lives under tests/run*. These directories are ignored by Git.
