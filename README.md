# TFTD Workshop
**Benjamin et GPT-6 édition — 2.12.7**

A Windows map editor and procedural composition workshop for X-COM: Terror from the Deep / OpenXcom Extended. Work in progress; the first public release uses the existing 2.12 version line.

[Français](docs/fr/GUIDE.md) · [English](docs/en/GUIDE.md) · [Español](docs/es/GUIDE.md) · [Deutsch](docs/de/GUIDE.md)

## Download and start
Download the Windows x64 ZIP from Releases, extract it to a writable folder and run TFTD_Workshop_V2.12.7_Rendu_Mixte.exe.
This is a portable Win32 program; no Python is needed to run it. It uses Windows system libraries including GDI+.
You supply your own TFTD/OXCE resources and optional graphics packs. No commercial game data or texture packs are bundled.

Configure Resources: TFTD original root, OXCE standard resources (standard/xcom2), and user/mods. Configure a personal HD PNG folder under Render → Universal templates folder.
Select the interface language under Language. Open Tutorial → Complete guide for the offline manual.

## What it does
- Browse MAP/MCD/PCK/TAB resources; inspect and compose scenes across Z levels.
- Place, move, duplicate and capture assemblies; edit plan and route data.
- Generate varied procedural compositions with terrain relief, terraces, GEO geometry and plateau decoration.
- Preview Legacy sprites, remastered PNGs, universal PNG templates and REAL HD terrain.
- Use REAL HD normal/debug together with either PNG provider for the remaining pieces.
- Save GEO compositions in JMW4 projects. Grid visibility also controls ship reservation overlays.
- Provide a complete four-language interface and an eight-chapter offline tutorial.

## Mixed rendering in 2.12.7
Choose **Render → REAL HD texture** or **REAL HD debug**, then **Render → PNG alongside REAL HD → PNG Remastered / Universal templates**.
SAND/DEBRIS and GEO_TERRAIN use REAL HD geometry and SAND materials. Other pieces use the selected PNG provider, then Legacy if the PNG is absent.
The PNG choice is remembered. A manual PNG provider does not redirect the REAL HD material paths.

Normal materials:
user/mods/TFTD_REAL_HD_TEXTURES/Resources/TFTD_HD/RealHD/Datasets/SAND/Materials/TOP_BASE.png
user/mods/TFTD_REAL_HD_TEXTURES/Resources/TFTD_HD/RealHD/Datasets/SAND/Materials/VERTICAL_BASE.png
Debug uses the corresponding TFTD_REAL_HD_DEBUG mod.
Missing SAND/DEBRIS materials show a magenta checkerboard; missing PNGs fall back to Legacy.
GEO geometry uses these SAND maps too; without its material it retains its geometry preview.

## Current limits
This Workshop preview does not implement the game's full shaders, lighting, water or effects.
The GEO layer has no direct scene editing/picking or plan representation yet. Export of a scene containing GEO to MAP/OXCE is blocked.
Generic scenes can use the export wizard where supported; exporting does not guarantee playability. Verify output in OXCE.
The generator is one evolving generator. The rejected 2.13 prototype is not included.
The active PNG frame follows MCD Frame[0]; the preview is not a full animation player.

## Documentation and development
See [installation](docs/INSTALLATION.md), [building/testing](docs/BUILDING.md), [changelog](CHANGELOG.md), [contribution](CONTRIBUTING.md) and [credits](CREDITS.md).
Four complete manuals are included as Markdown and offline HTML, and embedded in the program.
893 translation entries per language; editable UTF-8 catalogues and a standard-library Python regeneration tool.

## Validation
This release was compiled with warnings treated as errors. Local checks: 108 regression, 40 rendering/mixed GEO, 75 procedural, 64 language/tutorial and 4 preference reloads passed.
The rendering checks use locally supplied resources. Commercial fixtures are not distributed.
Visual preview inspected; user acceptance and in-game validation of 2.12.7 remain pending.

Contact: **colmoutarde57700@gmail.com**
**Thanks to GPT-6 Sol**

## License status
No explicit license is granted in this first publication, by the project owner’s choice. Public visibility is not a blanket permission to reuse or redistribute the code. Contact the project owner about permissions.
