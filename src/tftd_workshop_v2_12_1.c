#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shlobj.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <wctype.h>
#include <gdiplus/gdiplus.h>

#include "workshop_i18n.h"

#define APP_TITLE tr(L"TFTD Workshop V2.12.7 - Procedural + REAL HD Preview")
#define CFG_SECTION L"TFTDWorkshop"
#define MAX_LIBRARY 1024
#define MAX_ACTIVE 64
#define MAX_PROFILES 2048
#define MAX_MAP_ENTRIES 8192
#define MAX_PROFILE_DATASETS 24
#define PATH_CAP 1024
#define TILE_W 32
#define TILE_H 40
#define MCD_SIZE 62
#define TREE_TOP 198
#define SRC_TFTD 0
#define SRC_OXCE 1
#define SRC_MANUAL 2
#define SRC_MOD 3
#define TREE_BOTTOM 66
#define GRID_COLS 4
#define GRID_CELL_H 78
#define INSPECTOR_W 340
#define SEARCH_ID 2001
#define UNDO_MAX 65536
#define PLAN_SEMANTIC_COUNT 24
#define PLAN_OBJECT_COUNT 9
#define PLAN_FLAG_LOCKED 0x01
#define HD_CACHE_MAX 512
#define MAX_EXPORT_BIOMES 64
#define MAX_EXPORT_DATASETS 64
#define MAX_EXPORT_BLOCKS 128
#define MAX_RMP_NODES 251
#define RMP_REC_SIZE 24
#define MAX_CUSTOM_BLUEPRINTS 512
#define MAX_CUSTOM_BLUEPRINT_PARTS 65536
#define MAX_BLUEPRINT_CAPTURE 1024
#define MAX_OXC_TERRAINS 1024
#define MAX_OXC_BLOCKS 16384
#define MAX_OXC_SCRIPTS 2048
#define MAX_OXC_COMMANDS 32768
#define MAX_OXC_SELECT 48
#define MAX_OXC_RECTS 24
#define MAX_OXC_CONDITIONALS 24
#define MAX_OXC_MCD_PATCHES 8192
#define PLAN_DIAG_FORCE_NONE 255

#define RGB32(r,g,b) (0xFF000000u | ((uint32_t)(r)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(b))

typedef struct { uint8_t r,g,b; } JMColor;
typedef struct { uint8_t part[4]; } MapCell;
typedef struct { uint8_t semantic,west,north,east,south,vlink,flags,objectSemantic,diag,floorShape; } PlanCell;
typedef struct {
    PlanCell plan;
    int lib[4];
    int local[4];
} PlanClipboardCell;
typedef struct {
    int x,y,z;
    MapCell *cells;
    PlanCell *plan;
    wchar_t path[PATH_CAP];
    int dirty;
} MapDoc;

typedef struct {
    wchar_t name[96];
    wchar_t mcdPath[PATH_CAP];
    wchar_t pckPath[PATH_CAP];
    wchar_t tabPath[PATH_CAP];
    int source; /* SRC_* */
    wchar_t origin[96]; /* nom du mod pour SRC_MOD, sinon libelle source */
    int mcdCount;
    int frameCount;
    int loaded;
    int loadError;
    uint8_t *mcdRaw;
    uint8_t *sprites; /* frameCount * 32*40 */
    uint8_t *floorShapeCache; /* 0xFF inconnu, sinon PLAN_FSHAPE_* */
} LibrarySet;

typedef struct {
    int libIndex;
    int baseIndex;
} ActiveSet;

typedef struct {
    int scene;
    int kind; /* 0 piece MAP/scene, 1 cellule logique PLAN */
    int group;
    int x,y,z,layer;
    uint8_t before,after;
    int beforeLib,beforeLocal,afterLib,afterLocal;
    PlanCell planBefore,planAfter;
} EditAction;

typedef struct {
    wchar_t mapName[96];
    wchar_t dataSets[MAX_PROFILE_DATASETS][96];
    int dataSetCount;
    wchar_t rulePath[PATH_CAP];
    int kind; /* 0 terrain, 1 craft X-COM, 2 USO, 3 autre */
    int source;
    wchar_t origin[96];
} MapProfile;
enum { OXC_CMD_UNKNOWN=0,OXC_CMD_ADDBLOCK,OXC_CMD_ADDLINE,OXC_CMD_ADDCRAFT,OXC_CMD_ADDUFO,OXC_CMD_DIGTUNNEL,OXC_CMD_FILLAREA,OXC_CMD_CHECKBLOCK,OXC_CMD_REMOVE,OXC_CMD_RESIZE };
#define OXC_UNSUPPORTED_VERTICAL_LEVELS 0x01u
#define OXC_UNSUPPORTED_RANDOM_TERRAIN  0x02u
#define OXC_UNSUPPORTED_CRAFT_GROUPS    0x04u
#define OXC_UNSUPPORTED_NAMED_CRAFT_UFO 0x08u
typedef struct {
    wchar_t name[96];
    int width,length,height;
    int groups[MAX_OXC_SELECT],groupCount;
} OxcBlockDef;
typedef struct {
    wchar_t name[96],rulePath[PATH_CAP],origin[96];
    int source,firstBlock,blockCount;
} OxcTerrainDef;
typedef struct {
    int type;
    int sizeX,sizeY,sizeZ;
    int groups[MAX_OXC_SELECT],groupCount;
    int blocks[MAX_OXC_SELECT],blockCount;
    int freqs[MAX_OXC_SELECT],freqCount;
    int maxUses[MAX_OXC_SELECT],maxUsesCount;
    int rects[MAX_OXC_RECTS][4],rectCount;
    int conditionals[MAX_OXC_CONDITIONALS],conditionalCount;
    int executionChances,executions,label,canBeSkipped;
    int direction,verticalGroup,horizontalGroup,crossingGroup;
    unsigned unsupportedFlags;
    wchar_t terrain[96];
} OxcCommandDef;
typedef struct {
    wchar_t name[96],rulePath[PATH_CAP],origin[96];
    int source,firstCommand,commandCount;
} OxcMapScriptDef;
static OxcTerrainDef gOxcTerrains[MAX_OXC_TERRAINS];static int gOxcTerrainCount=0;
static OxcBlockDef gOxcBlocks[MAX_OXC_BLOCKS];static int gOxcBlockCount=0;
static OxcMapScriptDef gOxcScripts[MAX_OXC_SCRIPTS];static int gOxcScriptCount=0;
static OxcCommandDef gOxcCommands[MAX_OXC_COMMANDS];static int gOxcCommandCount=0;
typedef struct {
    wchar_t dataset[96],origin[96];
    int source,mcdIndex,bigWall;
} OxcMcdPatch;
static OxcMcdPatch gOxcMcdPatches[MAX_OXC_MCD_PATCHES];static int gOxcMcdPatchCount=0;

typedef struct {
    wchar_t name[96];
    wchar_t path[PATH_CAP];
    int source; /* SRC_TFTD / SRC_OXCE / SRC_MOD */
    wchar_t origin[96];
    int x,y,z;
    int profileIndex;
    int profileInferred;
    int legacyDormant;
    int rmpPresent;
    int rmpNodeCount;
} MapEntry;

typedef struct {
    const wchar_t *name;
    const wchar_t *family;
    const wchar_t *status;
} LegacyDormantInfo;

/* Audit Bible/handoff. CARGO4 n'est PAS ici : dataset graphique orphelin != MAP dormante. */
static const LegacyDormantInfo LEGACY_DORMANT_MAPS[] = {
    {L"A_BASE01",L"A_BASE",L"UNKNOWN"}, {L"A_BASE02",L"A_BASE",L"UNKNOWN"},
    {L"A_BASE03",L"A_BASE",L"NEEDS_MAP_FIX"}, {L"A_BASE04",L"A_BASE",L"NEEDS_MAP_FIX"},
    {L"A_BASE05",L"A_BASE",L"UNKNOWN"}, {L"A_BASE06",L"A_BASE",L"GOOD_VARIANT_FOUNDATION"},
    {L"A_BASE10",L"A_BASE",L"GOOD_VARIANT_FOUNDATION"}, {L"ATLAN12",L"ATLAN",L"GOOD_VARIANT_FOUNDATION"},
    {L"CORAL12",L"CORAL",L"NEEDS_RMP_REVIEW"}, {L"GRUNGE16",L"GRUNGE",L"GOOD_VARIANT_FOUNDATION"},
    {L"MU12",L"MU",L"NEEDS_RMP_REVIEW"}, {L"MU13",L"MU",L"NEEDS_RMP_REVIEW"},
    {L"MU14",L"MU",L"NEEDS_RMP_REVIEW"}, {L"HAMMER1",L"HAMMER",L"UNKNOWN"},
    {L"HAMMER2",L"HAMMER",L"UNKNOWN"}, {L"PLANE",L"PLANE",L"UNKNOWN"}
};
#define LEGACY_DORMANT_COUNT ((int)(sizeof(LEGACY_DORMANT_MAPS)/sizeof(LEGACY_DORMANT_MAPS[0])))

/* Blueprints structurels RC15 : voisinages reciproques fixes a 100 %, plus
   composites multi-couches explicitement valides dans la Bible. La semantique
   visuelle reste facultative : dataset/MCD/couche/offset suffisent au placement. */
typedef struct {
    const wchar_t *dataset;
    int mcd, layer, dx, dy, dz;
} BlueprintPartDef;
typedef struct {
    const wchar_t *id;
    int firstPart, partCount, support, spanX, spanY, spanZ;
} BlueprintDef;
#include "blueprints_rc15.h"

typedef struct {
    wchar_t dataset[96];
    int mcd, layer, dx, dy, dz;
} CustomBlueprintPart;
typedef struct {
    wchar_t name[96];
    int firstPart, partCount, spanX, spanY, spanZ;
} CustomBlueprintDef;
typedef struct {
    char magic[8];
    uint32_t version;
    uint32_t blueprintCount;
    uint32_t partCount;
    uint32_t defSize;
    uint32_t partSize;
} BlueprintStoreHeader;
typedef struct {
    int x,y,z,layer,lib,local;
} BlueprintCapturePart;

#include "geo_types.h"
typedef struct { int lib[4]; int local[4]; } SceneCell;
enum { SCENE_MACRO_CRAFT=0x01,SCENE_MACRO_USO=0x02,SCENE_MACRO_UNDERLAY=0x04 };

typedef struct {
    int x,y,z;
    int links[5];
    uint8_t raw[RMP_REC_SIZE];
    int autoScore;
} RmpNode;

typedef struct {
    RmpNode nodes[MAX_RMP_NODES];
    int count;
    RmpNode proposals[MAX_RMP_NODES];
    int proposalCount;
    int loaded,show,editMode,dirty,inherited,selected;
    wchar_t path[PATH_CAP];
    wchar_t sourceLabel[160];
} RmpDoc;

typedef struct {
    int x,y,z;
    SceneCell *cells;
    PlanCell *plan;
    int active;
    int dirty;
    wchar_t title[160];
    int usoSlotActive, usoSlotX, usoSlotY, usoSlotW, usoSlotH;
    wchar_t usoSlotName[96];
    int usoMapIndex;
    int usoInserted;
    int craftX, craftY, craftW, craftH;
    int macroW,macroH;
    GeoInstance*geo;int geoCount;int*geoLookup;GeoDecor*geoDecor;int geoDecorCount;
    uint8_t *macroFlags; /* grille 10x10 : craft / USO / sous-sol reserve */
} SceneDoc;

typedef struct {
    HWND hwnd;
    int clientW, clientH;
    int sidebarW;
    int inspectorW;
    int zoom;
    int panX, panY;
    int currentZ;
    int showAllBelow;
    int showComplete; /* Normal view only; editing stays on currentZ. */
    int showGrid;
    int selectedLayer;
    int viewMode; /* 0 tuiles ISO, 1 PLAN 2D, 2 PLAN ISO */
    int planTool; /* 0 case, 1 mur, 2 porte, 3 liaison Z, 4 selection, 5 verrou */
    int planSemantic;
    int planObjectSemantic;
    int planSemanticLayer; /* 0 SOL/ZONE, 1 OBJET/DECOR */
    int planLinkType;
    int planDisplayMode; /* 0 TOUT, 1 SOL, 2 MURS, 3 OBJETS */
    int planWallMode; /* 0 bord cardinal, 2 diagonale NE-SO, 3 diagonale NO-SE */
    int planFloorShape; /* PLAN_FSHAPE_* */
    int planDiagWarnMask;
    int brushMode; /* 0 case, 1 cadre, 2 plein, 3 remplissage */
    int brushSize; /* 1..16 */
    int brushShape; /* 0 carre, 1 cercle, 2 losange */
    int planSelecting;
    int planSelActive;
    int planSelX0,planSelY0,planSelX1,planSelY1;
    int planPasteMode;
    int planPasteX,planPasteY;
    PlanClipboardCell *planClip;
    int planClipW,planClipH;
    int draggingPan;
    int draggingUsoSlot;
    int usoDragOffX, usoDragOffY;
    int dragLastX, dragLastY;
    int mouseX, mouseY;

    JMColor palette[256];
    int paletteLoaded;
    wchar_t palettePath[PATH_CAP];

    wchar_t tftdRoot[PATH_CAP];
    wchar_t oxceRoot[PATH_CAP];
    wchar_t tftdMapRoot[PATH_CAP];
    wchar_t oxceMapRoot[PATH_CAP];
    wchar_t modsRoot[PATH_CAP];
    wchar_t hdCustomRoot[PATH_CAP]; /* provider graphique HD manuel, independant des profils/datasets */
    wchar_t configPath[PATH_CAP];
    int detectedMapsTFTD, detectedMapsOXCE, detectedMapsMods;
    int detectedRulOXCE, detectedRulMods;
    MapProfile profiles[MAX_PROFILES];
    int profileCount;
    MapEntry maps[MAX_MAP_ENTRIES];
    int mapCount;
    int selectedMap;
    int autoProfileIndex;

    LibrarySet library[MAX_LIBRARY];
    int libraryCount;
    int libraryCountTFTD;
    int libraryCountOXCE;
    int libraryCountMods;

    ActiveSet active[MAX_ACTIVE];
    int activeCount;
    int activeTotalMcd;

    int browserMode; /* 0 cartes, 1 bibliotheque, 2 palette MAP */
    int resourceFilter; /* -1 tous, 0 FLOOR, 1 WEST, 2 NORTH, 3 OBJECT */
    int hdOverlayProvider; /* 1 remastered, 2 universal; independent REAL HD fallback */
    int assetRenderMode; /* 0 Legacy, 1 HD standard/remastered + fallback Legacy, 2 MOD HD manuel + fallback Legacy */
    int mapFilterDormant;
    int expandedLib;
    int expandedActive;
    int treeScroll;
    int treeContentH;
    int sourceScroll[3];
    int panelSplit1, panelSplit2;
    int resizingSidebar;
    int resizingInspector;
    int resizingPanel;

    int selectedLib;
    int selectedLocal;
    HWND searchEdit;
    wchar_t searchFilter[96];
    int hoverValid, hoverX, hoverY;
    int painting;
    int erasing;
    int lastPaintX, lastPaintY;
    int blueprintMode;       /* 0 = piece seule, 1 = habituel, 2 = perso */
    int blueprintIndex;      /* index BLUEPRINTS_RC15 choisi, -1 si aucun */
    int customBlueprintIndex;
    CustomBlueprintDef customBlueprints[MAX_CUSTOM_BLUEPRINTS];
    CustomBlueprintPart customBlueprintParts[MAX_CUSTOM_BLUEPRINT_PARTS];
    int customBlueprintCount, customBlueprintPartCount;
    wchar_t customBlueprintPath[PATH_CAP];
    wchar_t customBlueprintDataPath[PATH_CAP];
    wchar_t blueprintHangarDir[PATH_CAP];
    int blueprintCaptureMode;
    BlueprintCapturePart blueprintCapture[MAX_BLUEPRINT_CAPTURE];
    int blueprintCaptureCount;
    int currentEditGroup;
    int nextEditGroup;
    EditAction undo[UNDO_MAX];
    int undoCount, undoPos;

    MapDoc map;
    SceneDoc scene;
    RmpDoc rmp;
    int rmpTool;          /* 0 node, 1 liaison, 2 N, 3 E, 4 S, 5 O, 6 suppression */
    int rmpNewType;       /* bits authorables : 0x01 flying, 0x02 small */
    int rmpNewRank;       /* 0..9, 9 observe Legacy hors enum */
    int rmpNewFlags;      /* desirabilite de patrouille */
    int rmpNewTarget;     /* reserved=5 */
    int rmpNewPriority;   /* priorite de spawn, 0 = pas de spawn */
    int rmpAdvanced;      /* 0 interface simple, 1 parametres RMP avances */
    uint32_t *backbuf;
    int backW, backH;
    BITMAPINFO bmi;
    uint32_t *overviewBuf;
    int overviewW, overviewH, overviewDirty;
    wchar_t status[512];
} App;

static App A;
#include "workshop_editor_state.h"

static uint32_t gThumb[TILE_W*TILE_H];

typedef struct {
    int lib, local;
    int w, h;
    uint32_t *pixels; /* ARGB 0xAARRGGBB */
    wchar_t path[PATH_CAP];
} HDCacheEntry;
static HDCacheEntry gHdCache[HD_CACHE_MAX];
static int gHdCacheCount=0;
static ULONG_PTR gGdiPlusToken=0;


typedef struct {
    HWND hwnd, terrainCombo, craftCombo, usoModeCombo, usoCombo;
    HWND widthEdit, lengthEdit, seedEdit, craftXEdit, craftYEdit, usoXEdit, usoYEdit, minDistEdit;
    HWND modeCombo, scriptCombo, recipeBtn, infoText, advancedBtn;
    HWND advanced[24];
    int advancedCount, advancedVisible;
} ComposerUI;
static ComposerUI CUI;

typedef struct {
    HWND hwnd, modCombo, testModEdit, nameEdit, biomeList, groupEdit, infoText;
    int sourceMapIndex;
} ExportUI;
static ExportUI EUI;

typedef struct {
    HWND hwnd, xEdit, yEdit, zEdit;
} NewMapUI;
static NewMapUI NUI;
static int gNewMapLastX=10, gNewMapLastY=10, gNewMapLastZ=1;

typedef struct {
    HWND hwnd, list, nameEdit, pathText;
    HWND chooseBtn, captureBtn, saveBtn, useBtn, renameBtn, deleteBtn, cancelCaptureBtn;
    int selected;
} BlueprintHangarUI;
static BlueprintHangarUI BUI;

enum { CID_TERRAIN=3001,CID_WIDTH,CID_LENGTH,CID_CRAFT,CID_USOMODE,CID_USO,CID_SEED,CID_CRAFTX,CID_CRAFTY,CID_USOX,CID_USOY,CID_MINDIST,CID_GENERATE,CID_CLOSE,CID_RANDOM_SEED,CID_COMPOSER_MODE,CID_MAPSCRIPT,CID_RECIPE,CID_ADVANCED };
enum { CID_EXPORT_MOD=4001,CID_EXPORT_TESTMOD,CID_EXPORT_NAME,CID_EXPORT_BIOMES,CID_EXPORT_GROUP,CID_EXPORT_GO,CID_EXPORT_CANCEL };
enum { CID_NEWMAP_X=5001,CID_NEWMAP_Y,CID_NEWMAP_Z,CID_NEWMAP_CREATE,CID_NEWMAP_CANCEL };
enum { CID_HANGAR_LIST=6001,CID_HANGAR_NAME,CID_HANGAR_CHOOSE,CID_HANGAR_CAPTURE,CID_HANGAR_SAVE,CID_HANGAR_USE,CID_HANGAR_RENAME,CID_HANGAR_DELETE,CID_HANGAR_CANCEL_CAPTURE };

enum {
    IDM_NEW_MAP=1001,
    IDM_OPEN_MAP=1010, IDM_SAVE, IDM_SAVE_AS, IDM_EXIT,
    IDM_SELECT_TFTD=1020, IDM_SELECT_OXCE, IDM_SELECT_MODS, IDM_REINDEX, IDM_MANUAL_MCD, IDM_LOAD_PALETTE, IDM_CLEAR_ACTIVE, IDM_SELECT_HD_CUSTOM=1029,
    IDM_LAYER_FLOOR=1030, IDM_LAYER_WEST, IDM_LAYER_NORTH, IDM_LAYER_OBJECT,
    IDM_Z_UP=1040, IDM_Z_DOWN, IDM_Z_ALL, IDM_Z_COMPLETE, IDM_Z_SINGLE, IDM_Z_BELOW,
    IDM_ZOOM_IN=1050, IDM_ZOOM_OUT, IDM_CENTER,
    IDM_SELECT_MAPS_TFTD=1053, IDM_SELECT_MAPS_OXCE, IDM_COMPOSER, IDM_CLOSE_SCENE,
    IDM_SCENE_INSERT_USO, IDM_SCENE_SAVE, IDM_SCENE_OPEN,
    IDM_UNDO=1060, IDM_REDO, IDM_PIPETTE,
    IDM_RENDER_REAL=1210, IDM_RENDER_DEBUG=1211, IDM_RENDER_CYCLE=1212, IDM_GEO_LAB=1213, IDM_RENDER_LEGACY=1070, IDM_RENDER_HD=1071, IDM_RENDER_HD_CUSTOM=1072, IDM_EXPORT_MOD=1073, IDM_FILTER_DORMANT=1075, IDM_GRID_TOGGLE, IDM_VIEW_PLAN=1110, IDM_PLAN_COPY, IDM_PLAN_PASTE, IDM_PLAN_CANCEL_PASTE, IDM_VIEW_NORMAL=1120, IDM_VIEW_TOP, IDM_VIEW_ISO,
    IDM_RMP_TOGGLE=1080, IDM_RMP_EDIT, IDM_RMP_ANALYZE, IDM_RMP_CLEAR_PROPOSALS, IDM_RMP_APPLY_PROPOSALS,
    IDM_RMP_SOCKET_N, IDM_RMP_SOCKET_E, IDM_RMP_SOCKET_S, IDM_RMP_SOCKET_W, IDM_RMP_SAVE,
    IDM_BLUEPRINT_HANGAR=1090, IDM_BLUEPRINT_CAPTURE, IDM_BLUEPRINT_SELECT_FOLDER,
    IDM_ABOUT=1100
};

static void active_clear(void);
static void scene_free(void);
static int scene_alloc(int w,int h,int z);
static void populate_composer_combos(void);
static void parse_ruleset_generation(const wchar_t*path,int source,const wchar_t*origin);
static void parse_ruleset_mcd_patches(const wchar_t*path,int source,const wchar_t*origin);
static void generation_clear(void);
static int composer_generate_exact(void);
static int exact_source_score(int candSource,const wchar_t*candOrigin,int source,const wchar_t*origin);
static int exact_script_supported(const OxcMapScriptDef*sc,wchar_t*why,int cap);
static int exact_terrain_for_family(const wchar_t*pref,int source,const wchar_t*origin);
static int exact_script_terrain_compatible(const OxcMapScriptDef*sc,int terrainIndex,wchar_t*why,int cap);
static int composer_auto_select_compatible_terrain(const OxcMapScriptDef*sc);
static void composer_show_recipe(void);
static int active_add_lib(int lib,int noisy);
static void set_z(int z);
static void do_undo(void);
static void do_redo(void);
static void fill_rect_color(HDC hdc,int l,int t,int r,int b,COLORREF c);
static void hd_cache_clear(void);
static void rh_clear(void);
static const wchar_t*render_mode_label(int mode);
static void draw_button(HDC hdc,int l,int t,int r,int b,const wchar_t*txt,int active);
static const wchar_t* source_name(int s);
static void open_export_wizard(void);
static void open_new_map_dialog(void);
static int safe_save_current_mod(void);
static int current_map_source(void);
static const wchar_t* current_map_origin(void);
static int current_map_is_protected(void);
static int confirm_map_change(void);
static int proc_can_replace(HWND h);
static void proc_close_panel(void);
static int manifest_find_block_biome(const wchar_t*mod,const wchar_t*block,wchar_t*out,int cap);
static void rmp_clear(void);
static const wchar_t* rmp_rank_name(int rank);
static const wchar_t* rmp_type_name(int type);
static void rmp_load_for_current_map(void);
static void rmp_toggle_overlay(void);
static void rmp_auto_analyze(void);
static void rmp_apply_proposals(void);
static int rmp_save_to_mod(void);
static int rmp_hit_test(int mx,int my);
static void rmp_draw_overlay(void);
static void draw_rmp_toolbar(HDC hdc);
static int rmp_toolbar_click(int mx,int my);
static int rmp_handle_left_click(int mx,int my,WPARAM wp);
static void rmp_delete_selected(void);
static void rmp_toggle_selected_socket(int special);
static int rmp_grid_distance(int ax,int ay,int bx,int by,int z,int maxd);
static void blueprint_refresh_for_selection(void);
static void draw_blueprint_toolbar(HDC hdc);
static int blueprint_toolbar_click(int mx,int my);
static void save_panel_layout(void){
    if(!A.configPath[0])return;const wchar_t*keys[4]={L"LeftPanelWidth",L"RightPanelWidth",L"LeftSplit1",L"LeftSplit2"};int values[4]={A.sidebarW,A.inspectorW,A.panelSplit1,A.panelSplit2};for(int i=0;i<4;i++){wchar_t text[24];_snwprintf(text,23,L"%d",values[i]);WritePrivateProfileStringW(CFG_SECTION,keys[i],text,A.configPath);}
}
static void custom_blueprints_path_init(void);
static void custom_blueprints_load(void);
static int custom_blueprints_save(void);
static int blueprint_capture_click(int mx,int my);
static int mouse_to_tile(int mx,int my,int*tx,int*ty);
static void blueprint_capture_save(void);
static int blueprint_capture_save_named(const wchar_t*name);
static void blueprint_capture_cancel(void);
static void open_blueprint_hangar(void);
static void blueprint_hangar_refresh(void);
static void blueprint_hangar_select_folder(void);
static int custom_blueprint_delete(int bi);
static PlanCell* plan_cell_at(int x,int y,int z);
static void draw_plan(void);
static void plan_draw_underlay_marker(int cx,int cy,int r);
static int plan_toolbar_click(int mx,int my);
static void draw_plan_toolbar(HDC hdc);
static void draw_plan_panel(HDC hdc,int l);
static int plan_panel_click(int mx,int my);
static void plan_copy_selection(void);
static void plan_begin_paste(void);
static void plan_cancel_paste(void);
static void plan_commit_paste(int x,int y);
static void plan_edit_at(int mx,int my,int erase);
static void plan_apply_cell(int x,int y,int z,PlanCell v,int record);

static void set_status(const wchar_t *s){ wcsncpy(A.status,s,511); A.status[511]=0; if(A.hwnd){RECT r={A.sidebarW,A.clientH-30,A.clientW-A.inspectorW,A.clientH};InvalidateRect(A.hwnd,&r,FALSE);} }
static int clampi(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }
static int has_ext(const wchar_t *name,const wchar_t *ext){ const wchar_t *d=wcsrchr(name,L'.'); return d && _wcsicmp(d,ext)==0; }
static void path_dirname(wchar_t *p){ wchar_t *a=wcsrchr(p,L'\\'),*b=wcsrchr(p,L'/'); wchar_t *s=a;if(!s||(b&&b>s))s=b;if(s)*s=0; }
static void path_basename_noext(const wchar_t *p,wchar_t *out,size_t cap){ const wchar_t *a=wcsrchr(p,L'\\'),*b=wcsrchr(p,L'/'); const wchar_t *s=a;if(!s||(b&&b>s))s=b;s=s?s+1:p;wcsncpy(out,s,cap-1);out[cap-1]=0;wchar_t*d=wcsrchr(out,L'.');if(d)*d=0; }
static void sibling_ext(const wchar_t *src,const wchar_t *ext,wchar_t *out,size_t cap){ wcsncpy(out,src,cap-1);out[cap-1]=0; wchar_t *d=wcsrchr(out,L'.');if(d)*d=0; wcsncat(out,ext,cap-wcslen(out)-1); }
static int path_exists_file(const wchar_t *p){ DWORD a=GetFileAttributesW(p); return a!=INVALID_FILE_ATTRIBUTES && !(a&FILE_ATTRIBUTE_DIRECTORY); }
static int path_exists_dir(const wchar_t *p){ DWORD a=GetFileAttributesW(p); return a!=INVALID_FILE_ATTRIBUTES && (a&FILE_ATTRIBUTE_DIRECTORY); }
static int write_all(const wchar_t *path,const void *data,DWORD sz);

static int path_starts_with_ci(const wchar_t*path,const wchar_t*root){
    if(!path||!root||!root[0])return 0;size_t n=wcslen(root);if(_wcsnicmp(path,root,n)!=0)return 0;
    return path[n]==0||path[n]==L'\\'||path[n]==L'/';
}
static int ensure_dir_recursive(const wchar_t*dir){
    if(!dir||!dir[0])return 0;if(path_exists_dir(dir))return 1;
    wchar_t tmp[PATH_CAP];wcsncpy(tmp,dir,PATH_CAP-1);tmp[PATH_CAP-1]=0;
    size_t n=wcslen(tmp);for(size_t i=3;i<n;i++)if(tmp[i]==L'\\'||tmp[i]==L'/'){wchar_t c=tmp[i];tmp[i]=0;if(!path_exists_dir(tmp))CreateDirectoryW(tmp,NULL);tmp[i]=c;}
    if(!path_exists_dir(tmp))CreateDirectoryW(tmp,NULL);return path_exists_dir(tmp);
}
static int copy_file_if_exists(const wchar_t*src,const wchar_t*dst){
    if(!path_exists_file(src))return 1;wchar_t d[PATH_CAP];wcsncpy(d,dst,PATH_CAP-1);d[PATH_CAP-1]=0;path_dirname(d);ensure_dir_recursive(d);return CopyFileW(src,dst,FALSE)!=0;
}
static void make_timestamp(wchar_t*out,int cap){
    SYSTEMTIME st;GetLocalTime(&st);_snwprintf(out,cap-1,L"%04d-%02d-%02d_%02d-%02d-%02d-%03d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);out[cap-1]=0;
}

static int valid_folder_component(const wchar_t*n){
    if(!n||!n[0]||wcscmp(n,L".")==0||wcscmp(n,L"..")==0)return 0;
    for(const wchar_t*p=n;*p;p++)if(*p<L' '||wcschr(L"<>:\"/\\|?*",*p))return 0;
    return 1;
}
static void make_mod_id(const wchar_t*name,wchar_t*out,int cap){
    int j=0;for(const wchar_t*p=name;*p&&j+1<cap;p++){wchar_t c=towupper(*p);if(iswalnum(c))out[j++]=c;else if(j>0&&out[j-1]!=L'_')out[j++]=L'_';}
    while(j>0&&out[j-1]==L'_')j--;out[j]=0;if(!out[0]){wcsncpy(out,L"TEST",cap-1);out[cap-1]=0;}
}
static int delete_tree_recursive(const wchar_t*dir){
    if(!dir||!dir[0]||!path_exists_dir(dir))return 1;wchar_t pat[PATH_CAP];_snwprintf(pat,PATH_CAP-1,L"%ls\\*",dir);WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);int ok=1;
    if(h!=INVALID_HANDLE_VALUE){do{if(wcscmp(fd.cFileName,L".")==0||wcscmp(fd.cFileName,L"..")==0)continue;wchar_t p[PATH_CAP];_snwprintf(p,PATH_CAP-1,L"%ls\\%ls",dir,fd.cFileName);if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){if(!delete_tree_recursive(p))ok=0;}else{SetFileAttributesW(p,FILE_ATTRIBUTE_NORMAL);if(!DeleteFileW(p))ok=0;}}while(FindNextFileW(h,&fd));FindClose(h);}
    SetFileAttributesW(dir,FILE_ATTRIBUTE_NORMAL);if(!RemoveDirectoryW(dir))ok=0;return ok;
}
static int create_test_mod(const wchar_t*folder,const wchar_t*block,wchar_t*outFolder,int outCap){
    if(!A.modsRoot[0]||!valid_folder_component(folder))return 0;
    _snwprintf(outFolder,outCap-1,L"%ls\\%ls",A.modsRoot,folder);outFolder[outCap-1]=0;
    if(path_exists_dir(outFolder)||path_exists_file(outFolder))return -1;
    if(!ensure_dir_recursive(outFolder))return 0;
    wchar_t p[PATH_CAP];const wchar_t*dirs[]={L"Maps",L"ROUTES",L"Ruleset",L"Workshop"};
    for(int i=0;i<4;i++){_snwprintf(p,PATH_CAP-1,L"%ls\\%ls",outFolder,dirs[i]);if(!ensure_dir_recursive(p)){delete_tree_recursive(outFolder);return 0;}}
    wchar_t id[160];make_mod_id(folder,id,160);
    char name8[512],id8[512],block8[512],meta[4096],readme[4096];
    WideCharToMultiByte(CP_UTF8,0,folder,-1,name8,512,NULL,NULL);
    WideCharToMultiByte(CP_UTF8,0,id,-1,id8,512,NULL,NULL);
    WideCharToMultiByte(CP_UTF8,0,block,-1,block8,512,NULL,NULL);
    _snprintf(meta,sizeof(meta)-1,
        "name: \"%s\"\nversion: \"0.1.0-test\"\nauthor: \"Benjamin + TFTD Workshop\"\nid: \"%s\"\nmaster: xcom2\ndescription: \"Mod de test cree automatiquement par TFTD Workshop pour tester le macrobloc %s sans modifier les mods de production.\"\n",
        name8,id8,block8);meta[sizeof(meta)-1]=0;
    _snwprintf(p,PATH_CAP-1,L"%ls\\metadata.yml",outFolder);if(!write_all(p,meta,(DWORD)strlen(meta))){delete_tree_recursive(outFolder);return 0;}
    _snprintf(readme,sizeof(readme)-1,
        "TFTD WORKSHOP - MOD DE TEST\r\nMacrobloc initial : %s\r\nCe mod est un bac a sable cree automatiquement.\r\nActivez-le dans OXCE pour tester, puis exportez vers votre mod de production une fois le resultat valide.\r\nLes datasets TFTD/OXCE restent references a leur emplacement normal ; ils ne sont pas recopies ici.\r\n",
        block8);readme[sizeof(readme)-1]=0;
    _snwprintf(p,PATH_CAP-1,L"%ls\\README_WORKSHOP_TEST.txt",outFolder);if(!write_all(p,readme,(DWORD)strlen(readme))){delete_tree_recursive(outFolder);return 0;}
    return 1;
}

static uint8_t *read_all(const wchar_t *path,DWORD *outSize){
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return NULL;
    DWORD hi=0,sz=GetFileSize(f,&hi); if(sz==INVALID_FILE_SIZE||hi){CloseHandle(f);return NULL;}
    uint8_t *b=(uint8_t*)malloc(sz?sz:1); if(!b){CloseHandle(f);return NULL;}
    DWORD got=0; BOOL ok=ReadFile(f,b,sz,&got,NULL); CloseHandle(f);
    if(!ok||got!=sz){free(b);return NULL;} if(outSize)*outSize=sz; return b;
}
static int write_all(const wchar_t *path,const void *data,DWORD sz){
    HANDLE f=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return 0;DWORD wr=0;BOOL ok=WriteFile(f,data,sz,&wr,NULL);CloseHandle(f);return ok&&wr==sz;
}
static DWORD file_size32(const wchar_t *path){ HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);if(f==INVALID_HANDLE_VALUE)return 0;DWORD hi=0,sz=GetFileSize(f,&hi);CloseHandle(f);return hi?0:sz; }

static int repair_literal_newlines_owned_file(const wchar_t*path,const char*signature){
    DWORD sz=0;uint8_t*raw=read_all(path,&sz);if(!raw||!sz){free(raw);return 0;}
    char*txt=(char*)malloc((size_t)sz+1);if(!txt){free(raw);return 0;}memcpy(txt,raw,sz);txt[sz]=0;free(raw);
    if(!strstr(txt,signature)||(!strstr(txt,"\\n")&&!strstr(txt,"\\r"))){free(txt);return 0;}
    wchar_t bak[PATH_CAP];_snwprintf(bak,PATH_CAP-1,L"%ls.V21_LITERAL_N_BACKUP",path);bak[PATH_CAP-1]=0;
    if(!path_exists_file(bak))CopyFileW(path,bak,TRUE);
    char*out=(char*)malloc((size_t)sz+1);if(!out){free(txt);return 0;}size_t j=0;
    for(size_t i=0;i<sz;i++){
        if(txt[i]=='\\'&&i+1<sz&&txt[i+1]=='n'){out[j++]='\n';i++;}
        else if(txt[i]=='\\'&&i+1<sz&&txt[i+1]=='r'){out[j++]='\r';i++;}
        else out[j++]=txt[i];
    }
    int ok=write_all(path,out,(DWORD)j);free(out);free(txt);return ok;
}
static int repair_workshop_generated_files(const wchar_t*modRoot){
    int repaired=0;wchar_t p[PATH_CAP];
    _snwprintf(p,PATH_CAP-1,L"%ls\\metadata.yml",modRoot);if(path_exists_file(p))repaired+=repair_literal_newlines_owned_file(p,"Benjamin + TFTD Workshop");
    _snwprintf(p,PATH_CAP-1,L"%ls\\Ruleset\\000_workshop_generated.rul",modRoot);if(path_exists_file(p))repaired+=repair_literal_newlines_owned_file(p,"Generated by TFTD Workshop");
    _snwprintf(p,PATH_CAP-1,L"%ls\\README_WORKSHOP_TEST.txt",modRoot);if(path_exists_file(p))repaired+=repair_literal_newlines_owned_file(p,"TFTD WORKSHOP - MOD DE TEST");
    return repaired;
}

static int open_file_dialog(HWND owner,wchar_t *out,DWORD cap,const wchar_t *filter,const wchar_t *title){
    OPENFILENAMEW o;ZeroMemory(&o,sizeof(o));out[0]=0;o.lStructSize=sizeof(o);o.hwndOwner=owner;o.lpstrFilter=filter;o.lpstrFile=out;o.nMaxFile=cap;o.lpstrTitle=title;o.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;return GetOpenFileNameW(&o);
}
static int save_file_dialog(HWND owner,wchar_t *out,DWORD cap,const wchar_t *filter,const wchar_t *title,const wchar_t *defext){
    OPENFILENAMEW o;ZeroMemory(&o,sizeof(o));o.lStructSize=sizeof(o);o.hwndOwner=owner;o.lpstrFilter=filter;o.lpstrFile=out;o.nMaxFile=cap;o.lpstrTitle=title;o.lpstrDefExt=defext;o.Flags=OFN_EXPLORER|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;return GetSaveFileNameW(&o);
}
static int CALLBACK browse_cb(HWND hwnd,UINT msg,LPARAM lp,LPARAM data){ (void)lp; if(msg==BFFM_INITIALIZED && data){ SendMessageW(hwnd,BFFM_SETSELECTIONW,TRUE,data); } return 0; }
static int browse_folder(HWND owner,wchar_t *out,size_t cap,const wchar_t *title,const wchar_t *initial){
    BROWSEINFOW bi;ZeroMemory(&bi,sizeof(bi));wchar_t display[MAX_PATH];
    bi.hwndOwner=owner;bi.pszDisplayName=display;bi.lpszTitle=title;bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE|BIF_EDITBOX;bi.lpfn=browse_cb;bi.lParam=(LPARAM)initial;
    PIDLIST_ABSOLUTE pidl=SHBrowseForFolderW(&bi); if(!pidl)return 0;
    wchar_t tmp[PATH_CAP];int ok=SHGetPathFromIDListW(pidl,tmp);CoTaskMemFree(pidl);if(!ok)return 0;wcsncpy(out,tmp,cap-1);out[cap-1]=0;return 1;
}

static void config_path_init(void){ DWORD n=GetModuleFileNameW(NULL,A.configPath,PATH_CAP); if(!n||n>=PATH_CAP){wcscpy(A.configPath,L"TFTD_Workshop.ini");return;} path_dirname(A.configPath); wcsncat(A.configPath,L"\\TFTD_Workshop.ini",PATH_CAP-wcslen(A.configPath)-1); }
static void config_load(void){i18n_load(A.configPath);A.hdOverlayProvider=clampi(GetPrivateProfileIntW(CFG_SECTION,L"RealHdPngProvider",1,A.configPath),1,2);
    GetPrivateProfileStringW(CFG_SECTION,L"TFTD",L"",A.tftdRoot,PATH_CAP,A.configPath);
    GetPrivateProfileStringW(CFG_SECTION,L"OXCE",L"",A.oxceRoot,PATH_CAP,A.configPath);
    GetPrivateProfileStringW(CFG_SECTION,L"TFTD_MAPS",L"",A.tftdMapRoot,PATH_CAP,A.configPath);
    GetPrivateProfileStringW(CFG_SECTION,L"OXCE_MAPS",L"",A.oxceMapRoot,PATH_CAP,A.configPath);
    GetPrivateProfileStringW(CFG_SECTION,L"MODS",L"",A.modsRoot,PATH_CAP,A.configPath);
    GetPrivateProfileStringW(CFG_SECTION,L"HD_CUSTOM_MOD",L"",A.hdCustomRoot,PATH_CAP,A.configPath);
    GetPrivateProfileStringW(CFG_SECTION,L"BLUEPRINT_HANGAR",L"",A.blueprintHangarDir,PATH_CAP,A.configPath);
    A.sidebarW=clampi(GetPrivateProfileIntW(CFG_SECTION,L"LeftPanelWidth",A.sidebarW,A.configPath),180,3000);
    A.inspectorW=clampi(GetPrivateProfileIntW(CFG_SECTION,L"RightPanelWidth",A.inspectorW,A.configPath),180,3000);
    A.panelSplit1=GetPrivateProfileIntW(CFG_SECTION,L"LeftSplit1",0,A.configPath);
    A.panelSplit2=GetPrivateProfileIntW(CFG_SECTION,L"LeftSplit2",0,A.configPath);
    int visibility=GetPrivateProfileIntW(CFG_SECTION,L"NormalVisibility",1,A.configPath);if(visibility<0||visibility>2)visibility=1;A.showComplete=visibility==2;A.showAllBelow=visibility!=0;
}
static void config_save(void){ WritePrivateProfileStringW(CFG_SECTION,L"RealHdPngProvider",A.hdOverlayProvider==2?L"2":L"1",A.configPath); WritePrivateProfileStringW(CFG_SECTION,L"TFTD",A.tftdRoot,A.configPath);WritePrivateProfileStringW(CFG_SECTION,L"OXCE",A.oxceRoot,A.configPath);WritePrivateProfileStringW(CFG_SECTION,L"TFTD_MAPS",A.tftdMapRoot,A.configPath);WritePrivateProfileStringW(CFG_SECTION,L"OXCE_MAPS",A.oxceMapRoot,A.configPath);WritePrivateProfileStringW(CFG_SECTION,L"MODS",A.modsRoot,A.configPath);WritePrivateProfileStringW(CFG_SECTION,L"HD_CUSTOM_MOD",A.hdCustomRoot,A.configPath);WritePrivateProfileStringW(CFG_SECTION,L"BLUEPRINT_HANGAR",A.blueprintHangarDir,A.configPath); }

static void custom_blueprints_path_init(void){
    if(A.blueprintHangarDir[0]&&path_exists_dir(A.blueprintHangarDir)){
        _snwprintf(A.customBlueprintPath,PATH_CAP-1,L"%ls\\TFTD_Workshop_Blueprints.ini",A.blueprintHangarDir);A.customBlueprintPath[PATH_CAP-1]=0;
        _snwprintf(A.customBlueprintDataPath,PATH_CAP-1,L"%ls\\TFTD_Workshop_Blueprints.jmb",A.blueprintHangarDir);A.customBlueprintDataPath[PATH_CAP-1]=0;return;
    }
    wcsncpy(A.customBlueprintPath,A.configPath,PATH_CAP-1);A.customBlueprintPath[PATH_CAP-1]=0;path_dirname(A.customBlueprintPath);
    wcsncpy(A.blueprintHangarDir,A.customBlueprintPath,PATH_CAP-1);A.blueprintHangarDir[PATH_CAP-1]=0;
    _snwprintf(A.customBlueprintDataPath,PATH_CAP-1,L"%ls\\TFTD_Workshop_Blueprints.jmb",A.blueprintHangarDir);A.customBlueprintDataPath[PATH_CAP-1]=0;
    wcsncat(A.customBlueprintPath,L"\\TFTD_Workshop_Blueprints.ini",PATH_CAP-wcslen(A.customBlueprintPath)-1);
}
static void custom_blueprint_recalc(CustomBlueprintDef*b){
    if(!b||b->partCount<=0)return;int maxx=0,maxy=0,maxz=0;
    for(int i=0;i<b->partCount;i++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+i];if(p->dx>maxx)maxx=p->dx;if(p->dy>maxy)maxy=p->dy;if(p->dz>maxz)maxz=p->dz;}
    b->spanX=maxx+1;b->spanY=maxy+1;b->spanZ=maxz+1;
}
static int custom_blueprints_load_binary(void){
    if(!A.customBlueprintDataPath[0]||!path_exists_file(A.customBlueprintDataPath))return 0;
    DWORD sz=0;uint8_t*raw=read_all(A.customBlueprintDataPath,&sz);if(!raw||sz<sizeof(BlueprintStoreHeader)){free(raw);return 0;}
    BlueprintStoreHeader h;memcpy(&h,raw,sizeof(h));
    if(memcmp(h.magic,"JMBP2616",8)!=0||h.version!=1||h.defSize!=sizeof(CustomBlueprintDef)||h.partSize!=sizeof(CustomBlueprintPart)||h.blueprintCount>MAX_CUSTOM_BLUEPRINTS||h.partCount>MAX_CUSTOM_BLUEPRINT_PARTS){free(raw);return 0;}
    size_t need=sizeof(BlueprintStoreHeader)+(size_t)h.blueprintCount*sizeof(CustomBlueprintDef)+(size_t)h.partCount*sizeof(CustomBlueprintPart);if(need!=(size_t)sz){free(raw);return 0;}
    const uint8_t*q=raw+sizeof(BlueprintStoreHeader);memcpy(A.customBlueprints,q,(size_t)h.blueprintCount*sizeof(CustomBlueprintDef));q+=(size_t)h.blueprintCount*sizeof(CustomBlueprintDef);memcpy(A.customBlueprintParts,q,(size_t)h.partCount*sizeof(CustomBlueprintPart));
    A.customBlueprintCount=(int)h.blueprintCount;A.customBlueprintPartCount=(int)h.partCount;
    for(int i=0;i<A.customBlueprintCount;i++){CustomBlueprintDef*b=&A.customBlueprints[i];b->name[95]=0;if(b->firstPart<0||b->partCount<1||b->firstPart+b->partCount>A.customBlueprintPartCount){A.customBlueprintCount=0;A.customBlueprintPartCount=0;free(raw);return 0;}custom_blueprint_recalc(b);}
    for(int i=0;i<A.customBlueprintPartCount;i++){CustomBlueprintPart*p=&A.customBlueprintParts[i];p->dataset[95]=0;if(p->layer<0||p->layer>3||p->mcd<0){A.customBlueprintCount=0;A.customBlueprintPartCount=0;free(raw);return 0;}}
    free(raw);return 1;
}
static void custom_blueprints_load_legacy_ini(void){
    if(!A.customBlueprintPath[0]||!path_exists_file(A.customBlueprintPath))return;
    int count=GetPrivateProfileIntW(L"Blueprints",L"Count",0,A.customBlueprintPath);if(count<0)count=0;if(count>MAX_CUSTOM_BLUEPRINTS)count=MAX_CUSTOM_BLUEPRINTS;
    for(int i=0;i<count;i++){wchar_t sec[32],nm[96];_snwprintf(sec,31,L"BP%03d",i);GetPrivateProfileStringW(sec,L"Name",L"",nm,96,A.customBlueprintPath);
        int pc=GetPrivateProfileIntW(sec,L"PartCount",0,A.customBlueprintPath);if(pc<1||A.customBlueprintPartCount+pc>MAX_CUSTOM_BLUEPRINT_PARTS)continue;
        CustomBlueprintDef*b=&A.customBlueprints[A.customBlueprintCount];ZeroMemory(b,sizeof(*b));b->firstPart=A.customBlueprintPartCount;b->partCount=0;
        if(nm[0])wcsncpy(b->name,nm,95);else _snwprintf(b->name,95,L"PERSO_%03d",A.customBlueprintCount+1);b->name[95]=0;
        int ok=1;for(int j=0;j<pc;j++){wchar_t key[32],line[512],tmp[512];_snwprintf(key,31,L"Part%d",j);GetPrivateProfileStringW(sec,key,L"",line,512,A.customBlueprintPath);if(!line[0]){ok=0;break;}wcsncpy(tmp,line,511);tmp[511]=0;
            wchar_t*ctx=NULL,*tok=wcstok(tmp,L";",&ctx);CustomBlueprintPart*p=&A.customBlueprintParts[A.customBlueprintPartCount+b->partCount];ZeroMemory(p,sizeof(*p));
            if(!tok){ok=0;break;}wcsncpy(p->dataset,tok,95);p->dataset[95]=0;int vals[5];for(int k=0;k<5;k++){tok=wcstok(NULL,L";",&ctx);if(!tok){ok=0;break;}vals[k]=_wtoi(tok);}if(!ok)break;
            p->mcd=vals[0];p->layer=vals[1];p->dx=vals[2];p->dy=vals[3];p->dz=vals[4];if(p->layer<0||p->layer>3){ok=0;break;}b->partCount++;}
        if(ok&&b->partCount>=1){A.customBlueprintPartCount+=b->partCount;custom_blueprint_recalc(b);A.customBlueprintCount++;}
    }
}
static void custom_blueprints_load(void){
    A.customBlueprintCount=0;A.customBlueprintPartCount=0;
    if(custom_blueprints_load_binary())return;
    custom_blueprints_load_legacy_ini();
}
static int custom_blueprints_save(void){
    if(!A.customBlueprintDataPath[0])return 0;
    BlueprintStoreHeader h;memset(&h,0,sizeof(h));memcpy(h.magic,"JMBP2616",8);h.version=1;h.blueprintCount=(uint32_t)A.customBlueprintCount;h.partCount=(uint32_t)A.customBlueprintPartCount;h.defSize=(uint32_t)sizeof(CustomBlueprintDef);h.partSize=(uint32_t)sizeof(CustomBlueprintPart);
    size_t total=sizeof(h)+(size_t)A.customBlueprintCount*sizeof(CustomBlueprintDef)+(size_t)A.customBlueprintPartCount*sizeof(CustomBlueprintPart);if(total>0xffffffffu)return 0;
    uint8_t*buf=(uint8_t*)malloc(total);if(!buf)return 0;uint8_t*q=buf;memcpy(q,&h,sizeof(h));q+=sizeof(h);if(A.customBlueprintCount){memcpy(q,A.customBlueprints,(size_t)A.customBlueprintCount*sizeof(CustomBlueprintDef));q+=(size_t)A.customBlueprintCount*sizeof(CustomBlueprintDef);}if(A.customBlueprintPartCount)memcpy(q,A.customBlueprintParts,(size_t)A.customBlueprintPartCount*sizeof(CustomBlueprintPart));
    wchar_t tmp[PATH_CAP];_snwprintf(tmp,PATH_CAP-1,L"%ls.tmp",A.customBlueprintDataPath);tmp[PATH_CAP-1]=0;int ok=write_all(tmp,buf,(DWORD)total);free(buf);if(!ok){DeleteFileW(tmp);return 0;}if(!MoveFileExW(tmp,A.customBlueprintDataPath,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(tmp);return 0;}
    return 1;
}

static void default_palette(void){ for(int i=0;i<256;i++){A.palette[i].r=A.palette[i].g=A.palette[i].b=(uint8_t)i;} A.palette[0].r=A.palette[0].g=A.palette[0].b=0;A.paletteLoaded=0;A.palettePath[0]=0;A.overviewDirty=1; }
static int load_lbm_palette(const wchar_t *path){
    DWORD sz=0;uint8_t *b=read_all(path,&sz);if(!b)return 0;int found=0;
    for(DWORD i=0;i+776<=sz;i++)if(b[i]=='C'&&b[i+1]=='M'&&b[i+2]=='A'&&b[i+3]=='P'){
        uint32_t n=((uint32_t)b[i+4]<<24)|((uint32_t)b[i+5]<<16)|((uint32_t)b[i+6]<<8)|b[i+7];
        if(n>=768&&i+8+768<=sz){for(int k=0;k<256;k++){A.palette[k].r=b[i+8+k*3];A.palette[k].g=b[i+8+k*3+1];A.palette[k].b=b[i+8+k*3+2];}found=1;break;}
    }
    free(b);if(found){A.paletteLoaded=1;wcsncpy(A.palettePath,path,PATH_CAP-1);A.palettePath[PATH_CAP-1]=0;A.overviewDirty=1;if(A.hwnd)InvalidateRect(A.hwnd,NULL,FALSE);}return found;
}

static int decode_pck_frames(const uint8_t *pck,DWORD pckSz,const uint8_t *tab,DWORD tabSz,uint8_t **outPixels,int *outCount){
    if(tabSz<2)return 0;
    int stride=2;
    if(tabSz>=4 && tab[0]==0 && tab[1]==0 && tab[2]==0 && tab[3]==0 && (tabSz%4)==0)stride=4;
    int count=(int)(tabSz/stride);if(count<=0||count>8192)return 0;
    uint8_t *pix=(uint8_t*)calloc((size_t)count,TILE_W*TILE_H);if(!pix)return 0;
    for(int f=0;f<count;f++){
        uint32_t off=0;
        if(stride==2)off=(uint32_t)tab[f*2]|((uint32_t)tab[f*2+1]<<8);
        else off=(uint32_t)tab[f*4]|((uint32_t)tab[f*4+1]<<8)|((uint32_t)tab[f*4+2]<<16)|((uint32_t)tab[f*4+3]<<24);
        if(off>=pckSz && off*2u<pckSz)off*=2u;
        if(off>=pckSz)continue;
        uint32_t cur=off,pos=(uint32_t)pck[cur++]*TILE_W;
        while(cur<pckSz&&pos<TILE_W*TILE_H){
            uint8_t c=pck[cur++];
            if(c==255)break;
            if(c==254){if(cur>=pckSz)break;pos+=pck[cur++];}
            else{pix[(size_t)f*TILE_W*TILE_H+pos]=c;pos++;}
        }
    }
    *outPixels=pix;*outCount=count;return 1;
}

static void library_free_loaded(LibrarySet *s){ if(s->mcdRaw)free(s->mcdRaw);if(s->sprites)free(s->sprites);if(s->floorShapeCache)free(s->floorShapeCache);s->mcdRaw=NULL;s->sprites=NULL;s->floorShapeCache=NULL;s->frameCount=0;s->loaded=0;s->loadError=0; }
static void library_clear(void){editor_reset(); for(int i=0;i<A.libraryCount;i++)library_free_loaded(&A.library[i]);ZeroMemory(A.library,sizeof(A.library));A.libraryCount=0;A.libraryCountTFTD=0;A.libraryCountOXCE=0;A.libraryCountMods=0;A.expandedLib=-1;A.selectedLib=-1;A.selectedLocal=0;A.blueprintIndex=-1;A.customBlueprintIndex=-1;A.blueprintMode=0;A.blueprintCaptureMode=0;A.blueprintCaptureCount=0; }
static int library_path_exists(const wchar_t *path){ for(int i=0;i<A.libraryCount;i++)if(_wcsicmp(A.library[i].mcdPath,path)==0)return 1;return 0; }
static int library_add_mcd(const wchar_t *mcdPath,int source,const wchar_t *origin){
    if(A.libraryCount>=MAX_LIBRARY||library_path_exists(mcdPath))return 0;
    DWORD mcdSz=file_size32(mcdPath);if(!mcdSz||mcdSz%MCD_SIZE)return 0;
    wchar_t pck[PATH_CAP],tab[PATH_CAP];sibling_ext(mcdPath,L".PCK",pck,PATH_CAP);sibling_ext(mcdPath,L".TAB",tab,PATH_CAP);
    if(!path_exists_file(pck)||!path_exists_file(tab))return 0;
    LibrarySet *s=&A.library[A.libraryCount++];ZeroMemory(s,sizeof(*s));path_basename_noext(mcdPath,s->name,96);wcsncpy(s->mcdPath,mcdPath,PATH_CAP-1);wcsncpy(s->pckPath,pck,PATH_CAP-1);wcsncpy(s->tabPath,tab,PATH_CAP-1);s->source=source;if(origin){wcsncpy(s->origin,origin,95);s->origin[95]=0;}s->mcdCount=(int)(mcdSz/MCD_SIZE);
    if(source==SRC_TFTD)A.libraryCountTFTD++;else if(source==SRC_OXCE)A.libraryCountOXCE++;else if(source==SRC_MOD)A.libraryCountMods++;
    return 1;
}
static int library_cmp(const LibrarySet *a,const LibrarySet *b){ if(a->source!=b->source)return a->source-b->source;int o=_wcsicmp(a->origin,b->origin);if(o)return o;int c=_wcsicmp(a->name,b->name);if(c)return c;return _wcsicmp(a->mcdPath,b->mcdPath); }
static void library_sort(void){ for(int i=1;i<A.libraryCount;i++){LibrarySet key=A.library[i];int j=i-1;while(j>=0&&library_cmp(&A.library[j],&key)>0){A.library[j+1]=A.library[j];j--;}A.library[j+1]=key;} }
static int library_load(int idx){
    if(idx<0||idx>=A.libraryCount)return 0;LibrarySet *s=&A.library[idx];if(s->loaded)return !s->loadError;
    DWORD ms=0,ps=0,ts=0;uint8_t *m=read_all(s->mcdPath,&ms),*p=read_all(s->pckPath,&ps),*t=read_all(s->tabPath,&ts);if(!m||!p||!t||ms!=(DWORD)s->mcdCount*MCD_SIZE){free(m);free(p);free(t);s->loaded=1;s->loadError=1;return 0;}
    s->mcdRaw=m;if(!decode_pck_frames(p,ps,t,ts,&s->sprites,&s->frameCount)){free(p);free(t);library_free_loaded(s);s->loaded=1;s->loadError=1;return 0;}free(p);free(t);s->floorShapeCache=(uint8_t*)malloc((size_t)s->mcdCount);if(s->floorShapeCache)memset(s->floorShapeCache,0xFF,(size_t)s->mcdCount);s->loaded=1;return 1;
}
static int mcd_frame(int lib,int local){ if(!library_load(lib))return -1;LibrarySet *s=&A.library[lib];if(local<0||local>=s->mcdCount)return -1;return s->mcdRaw[local*MCD_SIZE]; }
static int mcd_plevel(int lib,int local){ if(!library_load(lib))return 0;LibrarySet *s=&A.library[lib];if(local<0||local>=s->mcdCount)return 0;return s->mcdRaw[local*MCD_SIZE+49]; }
static int mcd_tlevel(int lib,int local){ if(!library_load(lib))return 0;LibrarySet *s=&A.library[lib];if(local<0||local>=s->mcdCount)return 0;return (int8_t)s->mcdRaw[local*MCD_SIZE+48]; }

static int mcd_u8(int lib,int local,int off){ if(!library_load(lib))return 0;LibrarySet*s=&A.library[lib];if(local<0||local>=s->mcdCount||off<0||off>=MCD_SIZE)return 0;return s->mcdRaw[local*MCD_SIZE+off]; }
static const wchar_t* layer_name(int p){ const wchar_t* n[4]={tr(L"SOL"),tr(L"MUR OUEST"),tr(L"MUR NORD"),tr(L"OBJET")}; return (p>=0&&p<4)?n[p]:tr(L"?"); }
enum {
    PLAN_AUTO=0,PLAN_FLOOR,PLAN_WATER,PLAN_SAND,PLAN_ROCK,PLAN_CORRIDOR,PLAN_CABIN,PLAN_STORAGE,
    PLAN_ENGINEERING,PLAN_BRIDGE,PLAN_RESTAURANT,PLAN_SERVICE,PLAN_CIVILIAN,PLAN_CARGO,PLAN_LAB,PLAN_CONTROL,
    PLAN_POWER,PLAN_HABITAT,PLAN_DEFENSE,PLAN_KELP,PLAN_REEF,PLAN_CAVE,PLAN_DEBRIS,PLAN_PADDING
};
static const wchar_t* PLAN_NAMES[PLAN_SEMANTIC_COUNT]={
    L"AUTO",L"SOL",L"EAU",L"SABLE",L"ROCHE",L"COULOIR",L"CABINE",L"STOCKAGE",
    L"TECHNIQUE",L"PASSERELLE",L"RESTAURANT",L"SERVICE",L"CIVIL",L"CARGAISON",L"LABO",L"CONTROLE",
    L"ENERGIE",L"HABITAT",L"DEFENSE",L"KELP",L"RECIF",L"GROTTE",L"DEBRIS",L"MARGE TECH."
};
static const uint32_t PLAN_COLORS[PLAN_SEMANTIC_COUNT]={
    0xFF172028u,0xFF6C7880u,0xFF245E86u,0xFFC7B986u,0xFF6A6663u,0xFFB8B2A2u,0xFF9A6E7Au,0xFF9A7446u,
    0xFF6C778Au,0xFF6F8FA7u,0xFF9B7D67u,0xFF7C8490u,0xFF948A76u,0xFF8A6445u,0xFF705D8Fu,0xFF4E7188u,
    0xFF9A8438u,0xFF718D78u,0xFF7A4F56u,0xFF416C4Eu,0xFF526E66u,0xFF4F5359u,0xFF705D51u,0xFF3A4045u
};
enum { PLAN_OBJ_AUTO=0,PLAN_OBJ_OBJECT,PLAN_OBJ_DECOR,PLAN_OBJ_DEBRIS,PLAN_OBJ_VEGETATION,PLAN_OBJ_KELP,PLAN_OBJ_REEF,PLAN_OBJ_ROCK,PLAN_OBJ_CARGO };
enum { PLAN_FSHAPE_AUTO=0,PLAN_FSHAPE_FULL,PLAN_FSHAPE_TRI_NO,PLAN_FSHAPE_TRI_NE,PLAN_FSHAPE_TRI_SE,PLAN_FSHAPE_TRI_SO };
static const wchar_t* PLAN_FSHAPE_NAMES[]={L"AUTO",L"PLEIN",L"TRI NO",L"TRI NE",L"TRI SE",L"TRI SO"};
static const wchar_t* PLAN_DISPLAY_NAMES[]={L"TOUT",L"SOL",L"MURS",L"OBJETS"};
static const wchar_t* BRUSH_MODE_NAMES[]={L"CASE",L"CADRE",L"PLEIN",L"REMPLIR"};
static const wchar_t* BRUSH_SHAPE_NAMES[]={L"CARRE",L"CERCLE",L"LOSANGE"};
static const wchar_t* PLAN_OBJECT_NAMES[PLAN_OBJECT_COUNT]={L"AUTO",L"OBJET",L"DECOR",L"DEBRIS",L"VEGETATION",L"KELP",L"RECIF",L"ROCHE",L"CARGAISON"};
static const uint32_t PLAN_OBJECT_COLORS[PLAN_OBJECT_COUNT]={0x00000000u,0xFFE1E8ECu,0xFF9BA8AEu,0xFF7C5D4Du,0xFF5A8B64u,0xFF3B7552u,0xFF548F82u,0xFF77716Du,0xFFB58249u};
static const wchar_t* plan_semantic_name(int v){return (v>=0&&v<PLAN_SEMANTIC_COUNT)?tr(PLAN_NAMES[v]):tr(L"?");}
static const wchar_t* plan_object_name(int v){return (v>=0&&v<PLAN_OBJECT_COUNT)?tr(PLAN_OBJECT_NAMES[v]):tr(L"?");}
static int plan_view_active(void){return A.viewMode==1||A.viewMode==2;}
static void reset_undo(void){ A.undoCount=A.undoPos=0; }
static void push_edit(int x,int y,int z,int layer,uint8_t before,uint8_t after){
    if(before==after)return;
    if(A.undoPos<A.undoCount)A.undoCount=A.undoPos;
    if(A.undoCount>=UNDO_MAX){ memmove(A.undo,A.undo+1,sizeof(EditAction)*(UNDO_MAX-1));A.undoCount=UNDO_MAX-1;if(A.undoPos>0)A.undoPos--; }
    EditAction *e=&A.undo[A.undoCount++];ZeroMemory(e,sizeof(*e));e->scene=0;e->group=A.currentEditGroup?A.currentEditGroup:++A.nextEditGroup;e->x=x;e->y=y;e->z=z;e->layer=layer;e->before=before;e->after=after;e->beforeLib=e->beforeLocal=e->afterLib=e->afterLocal=-1;A.undoPos=A.undoCount;
}
static void push_scene_edit(int x,int y,int z,int layer,int beforeLib,int beforeLocal,int afterLib,int afterLocal){
    if(beforeLib==afterLib && beforeLocal==afterLocal)return;
    if(A.undoPos<A.undoCount)A.undoCount=A.undoPos;
    if(A.undoCount>=UNDO_MAX){ memmove(A.undo,A.undo+1,sizeof(EditAction)*(UNDO_MAX-1));A.undoCount=UNDO_MAX-1;if(A.undoPos>0)A.undoPos--; }
    EditAction *e=&A.undo[A.undoCount++];ZeroMemory(e,sizeof(*e));e->scene=1;e->group=A.currentEditGroup?A.currentEditGroup:++A.nextEditGroup;e->x=x;e->y=y;e->z=z;e->layer=layer;e->beforeLib=beforeLib;e->beforeLocal=beforeLocal;e->afterLib=afterLib;e->afterLocal=afterLocal;A.undoPos=A.undoCount;
}
static int plan_equal(PlanCell a,PlanCell b){return memcmp(&a,&b,sizeof(a))==0;}
static void push_plan_edit(int x,int y,int z,PlanCell before,PlanCell after){
    if(plan_equal(before,after))return;if(A.undoPos<A.undoCount)A.undoCount=A.undoPos;
    if(A.undoCount>=UNDO_MAX){memmove(A.undo,A.undo+1,sizeof(EditAction)*(UNDO_MAX-1));A.undoCount=UNDO_MAX-1;if(A.undoPos>0)A.undoPos--;}
    EditAction*e=&A.undo[A.undoCount++];ZeroMemory(e,sizeof(*e));e->kind=1;e->scene=A.scene.active;e->group=A.currentEditGroup?A.currentEditGroup:++A.nextEditGroup;e->x=x;e->y=y;e->z=z;e->planBefore=before;e->planAfter=after;A.undoPos=A.undoCount;
}

static const LegacyDormantInfo* legacy_dormant_info(const wchar_t*name){
    if(!name||!name[0])return NULL;
    for(int i=0;i<LEGACY_DORMANT_COUNT;i++)if(_wcsicmp(LEGACY_DORMANT_MAPS[i].name,name)==0)return &LEGACY_DORMANT_MAPS[i];
    return NULL;
}
static void name_family(const wchar_t*name,wchar_t*out,size_t cap){
    if(!cap)return;wcsncpy(out,name?name:L"",cap-1);out[cap-1]=0;size_t n=wcslen(out);while(n>0&&iswdigit(out[n-1]))out[--n]=0;
}
static int source_allowed_for_ctx(int candidateSource,const wchar_t*candidateOrigin,int source,const wchar_t*origin){
    if(candidateSource==SRC_MOD)return source==SRC_MOD&&origin&&origin[0]&&candidateOrigin&&_wcsicmp(candidateOrigin,origin)==0;
    if(candidateSource==SRC_MANUAL)return source==SRC_MANUAL;
    if(source==SRC_MOD||source==SRC_OXCE)return candidateSource==SRC_OXCE||candidateSource==SRC_TFTD;
    if(source==SRC_TFTD)return candidateSource==SRC_TFTD||candidateSource==SRC_OXCE;
    return candidateSource==SRC_TFTD||candidateSource==SRC_OXCE;
}
static int library_find_name_ctx(const wchar_t *name,int source,const wchar_t *origin){
    int best=-1,bestScore=-99999;
    for(int i=0;i<A.libraryCount;i++){
        LibrarySet*s=&A.library[i];if(_wcsicmp(s->name,name)!=0)continue;if(!source_allowed_for_ctx(s->source,s->origin,source,origin))continue;int sc=0;
        if(source==SRC_MOD){if(s->source==SRC_MOD)sc=1000;else if(s->source==SRC_OXCE)sc=700;else if(s->source==SRC_TFTD)sc=600;}
        else if(source==SRC_OXCE){if(s->source==SRC_OXCE)sc=1000;else if(s->source==SRC_TFTD)sc=800;}
        else if(source==SRC_TFTD){if(s->source==SRC_TFTD)sc=1000;else if(s->source==SRC_OXCE)sc=200;}
        else {if(s->source==SRC_MANUAL)sc=1000;else if(s->source==SRC_TFTD)sc=700;else if(s->source==SRC_OXCE)sc=600;}
        if(sc>bestScore){best=i;bestScore=sc;}
    }return best;
}
static int library_find_foreign_mod(const wchar_t*name,const wchar_t*targetMod){
    for(int i=0;i<A.libraryCount;i++){LibrarySet*s=&A.library[i];if(s->source!=SRC_MOD||_wcsicmp(s->name,name)!=0)continue;if(!targetMod||!targetMod[0]||_wcsicmp(s->origin,targetMod)!=0)return i;}return -1;
}
static int library_find_name(const wchar_t *name){return library_find_name_ctx(name,SRC_TFTD,L"");}
static void profile_clear(void){ A.profileCount=0; A.autoProfileIndex=-1; ZeroMemory(A.profiles,sizeof(A.profiles)); }
static void trim_ascii(char *s){
    char *p=s;while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n')p++;
    if(p!=s)memmove(s,p,strlen(p)+1);
    size_t n=strlen(s);while(n&& (s[n-1]==' '||s[n-1]=='\t'||s[n-1]=='\r'||s[n-1]=='\n'))s[--n]=0;
    char *hash=strchr(s,'#');if(hash)*hash=0;n=strlen(s);while(n&& (s[n-1]==' '||s[n-1]=='\t'))s[--n]=0;
    if(n>=2 && ((s[0]=='"'&&s[n-1]=='"')||(s[0]=='\''&&s[n-1]=='\''))){memmove(s,s+1,n-2);s[n-2]=0;}
}
static void ascii_to_wide(const char *src,wchar_t *dst,size_t cap){
    if(!cap)return;int n=MultiByteToWideChar(CP_UTF8,0,src,-1,dst,(int)cap);if(!n){size_t i=0;for(;src[i]&&i+1<cap;i++)dst[i]=(unsigned char)src[i];dst[i]=0;}else dst[cap-1]=0;
}
static int ruleset_kind(const wchar_t *rulePath){
    wchar_t base[96];path_basename_noext(rulePath,base,96);
    if(_wcsicmp(base,L"crafts")==0)return 1;if(_wcsicmp(base,L"ufos")==0)return 2;if(_wcsicmp(base,L"terrains")==0)return 0;return 3;
}
static void profile_add(const char *mapName,char ds[][96],int dsCount,const wchar_t *rulePath,int source,const wchar_t *origin,int kind){
    if(!mapName[0]||dsCount<=0||A.profileCount>=MAX_PROFILES)return;wchar_t wmap[96];ascii_to_wide(mapName,wmap,96);
    for(int i=0;i<A.profileCount;i++){MapProfile*q=&A.profiles[i];if(q->source!=source||q->kind!=kind||_wcsicmp(q->mapName,wmap)!=0||q->dataSetCount!=dsCount)continue;if(source==SRC_MOD&&_wcsicmp(q->origin,origin?origin:L"")!=0)continue;int same=1;for(int k=0;k<dsCount;k++){wchar_t w[96];ascii_to_wide(ds[k],w,96);if(_wcsicmp(q->dataSets[k],w)!=0){same=0;break;}}if(same)return;}
    MapProfile*q=&A.profiles[A.profileCount++];ZeroMemory(q,sizeof(*q));wcsncpy(q->mapName,wmap,95);q->mapName[95]=0;q->dataSetCount=dsCount;q->kind=kind;q->source=source;
    for(int k=0;k<dsCount&&k<MAX_PROFILE_DATASETS;k++)ascii_to_wide(ds[k],q->dataSets[k],96);if(rulePath){wcsncpy(q->rulePath,rulePath,PATH_CAP-1);q->rulePath[PATH_CAP-1]=0;}if(origin){wcsncpy(q->origin,origin,95);q->origin[95]=0;}
}
static int parse_inline_datasets(const char*line,char ds[][96]){
    const char*l=strchr(line,'['),*r=strrchr(line,']');if(!l||!r||r<=l)return 0;char tmp[768];size_t n=(size_t)(r-l-1);if(n>=sizeof(tmp))n=sizeof(tmp)-1;memcpy(tmp,l+1,n);tmp[n]=0;int count=0;char*p=tmp;
    while(*p&&count<MAX_PROFILE_DATASETS){char*c=strchr(p,',');if(c)*c=0;trim_ascii(p);if(*p){strncpy(ds[count],p,95);ds[count][95]=0;count++;}if(!c)break;p=c+1;}return count;
}
static void profile_add_generic_terrain(const char*terrain,char ds[][96],int dsCount,const wchar_t*path,int source,const wchar_t*origin){
    if(!terrain[0]||dsCount<=0)return;for(int n=0;n<100&&A.profileCount<MAX_PROFILES;n++){char name[96];_snprintf(name,95,"%s%02d",terrain,n);name[95]=0;profile_add(name,ds,dsCount,path,source,origin,0);}
}

static void parse_workshop_manifest_profiles(const wchar_t*modRoot,const wchar_t*origin){
    wchar_t manifest[PATH_CAP];_snwprintf(manifest,PATH_CAP-1,L"%ls\\Workshop\\workshop_exports.ini",modRoot);manifest[PATH_CAP-1]=0;
    if(!path_exists_file(manifest))return;
    wchar_t sections[16384];sections[0]=0;GetPrivateProfileSectionNamesW(sections,16384,manifest);
    for(wchar_t*sec=sections;*sec;sec+=wcslen(sec)+1){
        if(_wcsnicmp(sec,L"Terrain:",8)!=0)continue;const wchar_t*terrainW=sec+8;
        wchar_t csv[4096];GetPrivateProfileStringW(sec,L"ResolvedDatasets",L"",csv,4096,manifest);if(!csv[0])continue;
        char terrain[96],ds[MAX_PROFILE_DATASETS][96];WideCharToMultiByte(CP_UTF8,0,terrainW,-1,terrain,96,NULL,NULL);int dn=0;
        wchar_t tmp[4096];wcsncpy(tmp,csv,4095);tmp[4095]=0;wchar_t*ctx=NULL,*p=wcstok(tmp,L",",&ctx);
        while(p&&dn<MAX_PROFILE_DATASETS){
            while(*p==L' ')p++;wchar_t*wEnd=p+wcslen(p);while(wEnd>p&&wEnd[-1]==L' ')*--wEnd=0;
            if(*p){WideCharToMultiByte(CP_UTF8,0,p,-1,ds[dn],96,NULL,NULL);dn++;}p=wcstok(NULL,L",",&ctx);
        }
        if(dn>0)profile_add_generic_terrain(terrain,ds,dn,manifest,SRC_MOD,origin);
    }
}

static void parse_ruleset_profiles(const wchar_t *path,int source,const wchar_t *origin){
    DWORD sz=0;uint8_t*raw=read_all(path,&sz);if(!raw||!sz){free(raw);return;}char*buf=(char*)malloc((size_t)sz+1);if(!buf){free(raw);return;}memcpy(buf,raw,sz);buf[sz]=0;free(raw);
    char ds[MAX_PROFILE_DATASETS][96];int dsCount=0,blocksIndent=-1,collectDS=0,collectBlocks=0,sectionKind=ruleset_kind(path),blockCount=0;char terrainName[96]={0};
    char*cur=buf;while(*cur){char*end=strpbrk(cur,"\r\n");if(end)*end=0;int indent=0;while(cur[indent]==' ')indent++;char line[768];strncpy(line,cur+indent,sizeof(line)-1);line[sizeof(line)-1]=0;trim_ascii(line);
        if(line[0]){
            if(indent==0&&strncmp(line,"terrains:",9)==0){if(sectionKind==0&&terrainName[0]&&dsCount>0&&blockCount==0)profile_add_generic_terrain(terrainName,ds,dsCount,path,source,origin);sectionKind=0;terrainName[0]=0;dsCount=0;collectDS=collectBlocks=0;}
            else if(indent==0&&strncmp(line,"crafts:",7)==0){sectionKind=1;terrainName[0]=0;dsCount=0;collectDS=collectBlocks=0;}
            else if(indent==0&&strncmp(line,"ufos:",5)==0){sectionKind=2;terrainName[0]=0;dsCount=0;collectDS=collectBlocks=0;}

            if(collectBlocks && indent<blocksIndent){collectBlocks=0;if(sectionKind==0&&terrainName[0]&&dsCount>0&&blockCount==0)profile_add_generic_terrain(terrainName,ds,dsCount,path,source,origin);}
            if(collectDS && !(line[0]=='-'&&line[1]==' '&&strchr(line+2,':')==NULL) && strncmp(line,"mapDataSets:",12)!=0)collectDS=0;

            if(sectionKind==0&&!collectBlocks&&strncmp(line,"- name:",7)==0){
                if(terrainName[0]&&dsCount>0&&blockCount==0)profile_add_generic_terrain(terrainName,ds,dsCount,path,source,origin);char v[96];strncpy(v,line+7,95);v[95]=0;trim_ascii(v);strncpy(terrainName,v,95);terrainName[95]=0;dsCount=0;blockCount=0;
            }
            if(strncmp(line,"mapDataSets:",12)==0){dsCount=parse_inline_datasets(line,ds);collectDS=dsCount==0;collectBlocks=0;}
            else if(collectDS&&line[0]=='-'&&line[1]==' '&&strchr(line+2,':')==NULL){char v[96];strncpy(v,line+2,95);v[95]=0;trim_ascii(v);if(v[0]&&dsCount<MAX_PROFILE_DATASETS){strncpy(ds[dsCount],v,95);ds[dsCount][95]=0;dsCount++;}}

            if(strncmp(line,"mapBlocks:",10)==0&&dsCount>0){blocksIndent=indent;collectBlocks=1;collectDS=0;blockCount=0;}
            else if(collectBlocks&&indent>=blocksIndent&&strncmp(line,"- name:",7)==0){char v[96];strncpy(v,line+7,95);v[95]=0;trim_ascii(v);profile_add(v,ds,dsCount,path,source,origin,sectionKind);blockCount++;}
        }
        if(!end)break;cur=end+1;if(*cur=='\n'||*cur=='\r')cur++;
    }
    if(sectionKind==0&&terrainName[0]&&dsCount>0&&blockCount==0)profile_add_generic_terrain(terrainName,ds,dsCount,path,source,origin);free(buf);
}
static void generation_clear(void){gOxcTerrainCount=gOxcBlockCount=gOxcScriptCount=gOxcCommandCount=gOxcMcdPatchCount=0;ZeroMemory(gOxcTerrains,sizeof(gOxcTerrains));ZeroMemory(gOxcBlocks,sizeof(gOxcBlocks));ZeroMemory(gOxcScripts,sizeof(gOxcScripts));ZeroMemory(gOxcCommands,sizeof(gOxcCommands));ZeroMemory(gOxcMcdPatches,sizeof(gOxcMcdPatches));}
static const char* yaml_value_ascii(const char*line){const char*c=strchr(line,':');return c?c+1:"";}
static int parse_ints_ascii_safe(const char*s,int*out,int cap){int n=0;while(*s&&n<cap){while(*s&&*s!='-'&&!(*s>='0'&&*s<='9'))s++;if(!*s)break;char*e=NULL;long v=strtol(s,&e,10);if(e==s){s++;continue;}out[n++]=(int)v;s=e;}return n;}
static void yaml_string_value(const char*line,wchar_t*out,int cap){char v[256];strncpy(v,yaml_value_ascii(line),255);v[255]=0;trim_ascii(v);ascii_to_wide(v,out,(size_t)cap);}
static int oxc_cmd_type(const char*s){if(_stricmp(s,"addBlock")==0)return OXC_CMD_ADDBLOCK;if(_stricmp(s,"addLine")==0)return OXC_CMD_ADDLINE;if(_stricmp(s,"addCraft")==0)return OXC_CMD_ADDCRAFT;if(_stricmp(s,"addUFO")==0)return OXC_CMD_ADDUFO;if(_stricmp(s,"digTunnel")==0)return OXC_CMD_DIGTUNNEL;if(_stricmp(s,"fillArea")==0)return OXC_CMD_FILLAREA;if(_stricmp(s,"checkBlock")==0)return OXC_CMD_CHECKBLOCK;if(_stricmp(s,"removeBlock")==0)return OXC_CMD_REMOVE;if(_stricmp(s,"resize")==0)return OXC_CMD_RESIZE;return OXC_CMD_UNKNOWN;}
static const wchar_t* oxc_cmd_name(int t){switch(t){case OXC_CMD_ADDBLOCK:return tr(L"placer un macro-bloc");case OXC_CMD_ADDLINE:return tr(L"ajouter une ligne");case OXC_CMD_ADDCRAFT:return tr(L"placer le vaisseau X-COM");case OXC_CMD_ADDUFO:return tr(L"placer l'USO");case OXC_CMD_DIGTUNNEL:return tr(L"creuser les raccords");case OXC_CMD_FILLAREA:return tr(L"remplir les cases libres");case OXC_CMD_CHECKBLOCK:return tr(L"verifier un bloc");case OXC_CMD_REMOVE:return tr(L"retirer un bloc");case OXC_CMD_RESIZE:return tr(L"redimensionner");default:return tr(L"commande inconnue");}}
static void oxc_command_defaults(OxcCommandDef*c){ZeroMemory(c,sizeof(*c));c->sizeX=1;c->sizeY=1;c->sizeZ=0;c->executionChances=100;c->executions=1;c->canBeSkipped=1;c->verticalGroup=1;c->horizontalGroup=2;c->crossingGroup=3;for(int i=0;i<MAX_OXC_SELECT;i++)c->maxUses[i]=-1;}
static void parse_ruleset_generation(const wchar_t*path,int source,const wchar_t*origin){
    DWORD sz=0;uint8_t*raw=read_all(path,&sz);if(!raw||!sz){free(raw);return;}char*buf=(char*)malloc((size_t)sz+1);if(!buf){free(raw);return;}memcpy(buf,raw,sz);buf[sz]=0;free(raw);
    int section=0,ti=-1,bi=-1,si=-1,ci=-1,inBlocks=0,blocksIndent=-1,blockIndent=-1,inCommands=0,commandsIndent=-1,commandIndent=-1,collectRects=0,rectIndent=-1,collectCmdList=0,cmdListIndent=-1,collectBlockGroups=0,blockGroupsIndent=-1;
    char*cur=buf;while(*cur){char*end=strpbrk(cur,"\r\n");if(end)*end=0;int indent=0;while(cur[indent]==' ')indent++;char line[1024];strncpy(line,cur+indent,1023);line[1023]=0;trim_ascii(line);
        if(line[0]){
            if(indent==0&&strcmp(line,"terrains:")==0){section=1;ti=bi=-1;inBlocks=0;collectBlockGroups=0;}
            else if(indent==0&&strcmp(line,"mapScripts:")==0){section=2;si=ci=-1;inCommands=0;collectCmdList=0;collectRects=0;}
            else if(indent==0&&line[0]!='-'&&strchr(line,':')){section=0;ti=bi=si=ci=-1;inBlocks=inCommands=0;collectCmdList=collectRects=collectBlockGroups=0;}
            if(section==1){
                if(inBlocks&&indent<=blocksIndent&&strncmp(line,"mapBlocks:",10)!=0){inBlocks=0;bi=-1;collectBlockGroups=0;}
                if(strncmp(line,"mapBlocks:",10)==0&&ti>=0){inBlocks=1;blocksIndent=indent;bi=-1;collectBlockGroups=0;}
                else if(strncmp(line,"- name:",7)==0){
                    char v[256];strncpy(v,line+7,255);v[255]=0;trim_ascii(v);
                    if(inBlocks&&indent>blocksIndent){if(gOxcBlockCount<MAX_OXC_BLOCKS){bi=gOxcBlockCount++;OxcBlockDef*b=&gOxcBlocks[bi];ZeroMemory(b,sizeof(*b));ascii_to_wide(v,b->name,96);b->width=b->length=10;b->height=4;blockIndent=indent;collectBlockGroups=0;if(ti>=0)gOxcTerrains[ti].blockCount++;}}
                    else if(gOxcTerrainCount<MAX_OXC_TERRAINS){ti=gOxcTerrainCount++;OxcTerrainDef*t=&gOxcTerrains[ti];ZeroMemory(t,sizeof(*t));ascii_to_wide(v,t->name,96);t->source=source;t->firstBlock=gOxcBlockCount;wcsncpy(t->rulePath,path,PATH_CAP-1);if(origin)wcsncpy(t->origin,origin,95);bi=-1;inBlocks=0;}
                }else if(bi>=0&&inBlocks&&indent>blockIndent){OxcBlockDef*b=&gOxcBlocks[bi];
                    if(collectBlockGroups&&indent<=blockGroupsIndent)collectBlockGroups=0;
                    if(collectBlockGroups&&line[0]=='-'&&strchr(line,':')==NULL){int v[4],n=parse_ints_ascii_safe(line,v,4);for(int q=0;q<n&&b->groupCount<MAX_OXC_SELECT;q++)b->groups[b->groupCount++]=v[q];}
                    else if(strncmp(line,"width:",6)==0)b->width=atoi(yaml_value_ascii(line));else if(strncmp(line,"length:",7)==0)b->length=atoi(yaml_value_ascii(line));else if(strncmp(line,"height:",7)==0)b->height=atoi(yaml_value_ascii(line));else if(strncmp(line,"groups:",7)==0){b->groupCount=parse_ints_ascii_safe(yaml_value_ascii(line),b->groups,MAX_OXC_SELECT);if(b->groupCount==0){collectBlockGroups=1;blockGroupsIndent=indent;}}
                }
            }else if(section==2){
                if(inCommands&&indent<commandsIndent&&strncmp(line,"commands:",9)!=0){inCommands=0;ci=-1;collectRects=0;collectCmdList=0;}
                if(strncmp(line,"commands:",9)==0&&si>=0){inCommands=1;commandsIndent=indent;ci=-1;collectRects=0;collectCmdList=0;}
                else if(strncmp(line,"- type:",7)==0){char v[256];strncpy(v,line+7,255);v[255]=0;trim_ascii(v);
                    if(inCommands&&indent>=commandsIndent){if(gOxcCommandCount<MAX_OXC_COMMANDS){ci=gOxcCommandCount++;OxcCommandDef*c=&gOxcCommands[ci];oxc_command_defaults(c);c->type=oxc_cmd_type(v);commandIndent=indent;collectRects=0;collectCmdList=0;if(c->type==OXC_CMD_ADDCRAFT||c->type==OXC_CMD_ADDUFO){c->groups[0]=1;c->groupCount=1;}if(si>=0)gOxcScripts[si].commandCount++;}}
                    else if(gOxcScriptCount<MAX_OXC_SCRIPTS){si=gOxcScriptCount++;OxcMapScriptDef*sc=&gOxcScripts[si];ZeroMemory(sc,sizeof(*sc));ascii_to_wide(v,sc->name,96);sc->source=source;sc->firstCommand=gOxcCommandCount;wcsncpy(sc->rulePath,path,PATH_CAP-1);if(origin)wcsncpy(sc->origin,origin,95);inCommands=0;ci=-1;}
                }else if(ci>=0&&inCommands&&indent>commandIndent){OxcCommandDef*c=&gOxcCommands[ci];
                    if(collectRects&&indent<=rectIndent)collectRects=0;
                    if(collectCmdList&&indent<=cmdListIndent)collectCmdList=0;
                    if(collectRects&&line[0]=='-'&&c->rectCount<MAX_OXC_RECTS){int v[8],n=parse_ints_ascii_safe(line,v,8);if(n>=4){for(int q=0;q<4;q++)c->rects[c->rectCount][q]=v[q];c->rectCount++;}}
                    else if(collectCmdList&&line[0]=='-'&&strchr(line,':')==NULL){int v[8],n=parse_ints_ascii_safe(line,v,8);for(int q=0;q<n;q++){if(collectCmdList==1&&c->groupCount<MAX_OXC_SELECT)c->groups[c->groupCount++]=v[q];else if(collectCmdList==2&&c->blockCount<MAX_OXC_SELECT)c->blocks[c->blockCount++]=v[q];else if(collectCmdList==3&&c->freqCount<MAX_OXC_SELECT)c->freqs[c->freqCount++]=v[q];else if(collectCmdList==4&&c->maxUsesCount<MAX_OXC_SELECT)c->maxUses[c->maxUsesCount++]=v[q];else if(collectCmdList==5&&c->conditionalCount<MAX_OXC_CONDITIONALS)c->conditionals[c->conditionalCount++]=v[q];}}
                    else if(strncmp(line,"rects:",6)==0){int v[MAX_OXC_RECTS*4],n=parse_ints_ascii_safe(yaml_value_ascii(line),v,MAX_OXC_RECTS*4);for(int k=0;k+3<n&&c->rectCount<MAX_OXC_RECTS;k+=4){for(int q=0;q<4;q++)c->rects[c->rectCount][q]=v[k+q];c->rectCount++;}if(n==0){collectRects=1;rectIndent=indent;}collectCmdList=0;}
                    else if(strncmp(line,"groups:",7)==0){c->groupCount=parse_ints_ascii_safe(yaml_value_ascii(line),c->groups,MAX_OXC_SELECT);if(c->groupCount==0){collectCmdList=1;cmdListIndent=indent;}}
                    else if(strncmp(line,"blocks:",7)==0){c->blockCount=parse_ints_ascii_safe(yaml_value_ascii(line),c->blocks,MAX_OXC_SELECT);if(c->blockCount==0){collectCmdList=2;cmdListIndent=indent;}}
                    else if(strncmp(line,"freqs:",6)==0){c->freqCount=parse_ints_ascii_safe(yaml_value_ascii(line),c->freqs,MAX_OXC_SELECT);if(c->freqCount==0){collectCmdList=3;cmdListIndent=indent;}}
                    else if(strncmp(line,"maxUses:",8)==0){c->maxUsesCount=parse_ints_ascii_safe(yaml_value_ascii(line),c->maxUses,MAX_OXC_SELECT);if(c->maxUsesCount==0){collectCmdList=4;cmdListIndent=indent;}}
                    else if(strncmp(line,"conditionals:",13)==0){c->conditionalCount=parse_ints_ascii_safe(yaml_value_ascii(line),c->conditionals,MAX_OXC_CONDITIONALS);if(c->conditionalCount==0){collectCmdList=5;cmdListIndent=indent;}}
                    else if(strncmp(line,"size:",5)==0){int v[3],n=parse_ints_ascii_safe(yaml_value_ascii(line),v,3);if(n==1){c->sizeX=c->sizeY=v[0];}else if(n>=2){c->sizeX=v[0];c->sizeY=v[1];if(n>=3)c->sizeZ=v[2];}}
                    else if(strncmp(line,"executionChances:",17)==0)c->executionChances=atoi(yaml_value_ascii(line));
                    else if(strncmp(line,"executions:",11)==0)c->executions=atoi(yaml_value_ascii(line));
                    else if(strncmp(line,"label:",6)==0)c->label=abs(atoi(yaml_value_ascii(line)));
                    else if(strncmp(line,"canBeSkipped:",13)==0){char v[32];strncpy(v,yaml_value_ascii(line),31);v[31]=0;trim_ascii(v);c->canBeSkipped=_stricmp(v,"false")!=0&&strcmp(v,"0")!=0;}
                    else if(strncmp(line,"terrain:",8)==0)yaml_string_value(line,c->terrain,96);
                    else if(strncmp(line,"verticalLevels:",15)==0)c->unsupportedFlags|=OXC_UNSUPPORTED_VERTICAL_LEVELS;
                    else if(strncmp(line,"randomTerrain:",14)==0)c->unsupportedFlags|=OXC_UNSUPPORTED_RANDOM_TERRAIN;
                    else if(strncmp(line,"craftGroups:",12)==0)c->unsupportedFlags|=OXC_UNSUPPORTED_CRAFT_GROUPS;
                    else if(strncmp(line,"craftName:",10)==0||strncmp(line,"UFOName:",8)==0)c->unsupportedFlags|=OXC_UNSUPPORTED_NAMED_CRAFT_UFO;
                    else if(strncmp(line,"direction:",10)==0){char v[32];strncpy(v,yaml_value_ascii(line),31);v[31]=0;trim_ascii(v);c->direction=(toupper((unsigned char)v[0])=='V')?1:(toupper((unsigned char)v[0])=='H'?2:(toupper((unsigned char)v[0])=='B'?3:0));}
                    else if(strncmp(line,"verticalGroup:",14)==0)c->verticalGroup=atoi(yaml_value_ascii(line));
                    else if(strncmp(line,"horizontalGroup:",16)==0)c->horizontalGroup=atoi(yaml_value_ascii(line));
                    else if(strncmp(line,"crossingGroup:",14)==0)c->crossingGroup=atoi(yaml_value_ascii(line));
                }
            }
        }
        if(!end)break;cur=end+1;if(*cur=='\n'||*cur=='\r')cur++;
    }free(buf);
}

static void parse_ruleset_mcd_patches(const wchar_t*path,int source,const wchar_t*origin){
    DWORD sz=0;uint8_t*raw=read_all(path,&sz);if(!raw||!sz){free(raw);return;}char*buf=(char*)malloc((size_t)sz+1);if(!buf){free(raw);return;}memcpy(buf,raw,sz);buf[sz]=0;free(raw);
    int inSection=0,typeIndent=-1,dataIndent=-1,currentIndex=-1;char dataset[96]={0};
    char*cur=buf;while(*cur){char*end=strpbrk(cur,"\r\n");if(end)*end=0;int indent=0;while(cur[indent]==' ')indent++;char line[512];strncpy(line,cur+indent,511);line[511]=0;trim_ascii(line);
        if(line[0]){
            if(indent==0&&strncmp(line,"MCDPatches:",11)==0){inSection=1;dataset[0]=0;currentIndex=-1;}
            else if(indent==0&&inSection&&line[0]!='#'&&strncmp(line,"MCDPatches:",11)!=0){inSection=0;dataset[0]=0;currentIndex=-1;}
            if(inSection){
                if(strncmp(line,"- type:",7)==0){char v[128];strncpy(v,line+7,127);v[127]=0;trim_ascii(v);strncpy(dataset,v,95);dataset[95]=0;typeIndent=indent;dataIndent=-1;currentIndex=-1;}
                else if(dataset[0]&&strncmp(line,"data:",5)==0&&indent>typeIndent){dataIndent=indent;currentIndex=-1;}
                else if(dataset[0]&&dataIndent>=0&&indent>dataIndent&&strncmp(line,"- MCDIndex:",11)==0){currentIndex=atoi(line+11);}
                else if(dataset[0]&&currentIndex>=0&&strncmp(line,"bigWall:",8)==0){int bw=atoi(yaml_value_ascii(line));if(gOxcMcdPatchCount<MAX_OXC_MCD_PATCHES){OxcMcdPatch*q=&gOxcMcdPatches[gOxcMcdPatchCount++];ZeroMemory(q,sizeof(*q));ascii_to_wide(dataset,q->dataset,96);q->source=source;q->mcdIndex=currentIndex;q->bigWall=bw;if(origin)wcsncpy(q->origin,origin,95);}}
            }
        }
        if(!end)break;cur=end+1;if(*cur=='\n'||*cur=='\r')cur++;
    }
    free(buf);
}
static int mcd_patch_score(const OxcMcdPatch*p,const LibrarySet*s){
    if(!p||!s||_wcsicmp(p->dataset,s->name)!=0)return -999999;
    if(s->source==SRC_MOD){if(p->source==SRC_MOD&&s->origin[0]&&_wcsicmp(p->origin,s->origin)==0)return 300;if(p->source==SRC_OXCE)return 200;if(p->source==SRC_TFTD)return 100;}
    if(s->source==SRC_OXCE){if(p->source==SRC_OXCE)return 300;if(p->source==SRC_TFTD)return 200;}
    if(s->source==SRC_TFTD){if(p->source==SRC_OXCE)return 300;if(p->source==SRC_TFTD)return 200;}
    if(s->source==SRC_MANUAL){if(p->source==SRC_OXCE)return 200;if(p->source==SRC_TFTD)return 100;}
    return -999999;
}
static int mcd_bigwall_effective(int lib,int local){
    if(lib<0||lib>=A.libraryCount||local<0)return 0;int v=mcd_u8(lib,local,33),best=-999999;LibrarySet*s=&A.library[lib];
    for(int i=0;i<gOxcMcdPatchCount;i++){OxcMcdPatch*q=&gOxcMcdPatches[i];if(q->mcdIndex!=local)continue;int sc=mcd_patch_score(q,s);if(sc>best){best=sc;v=q->bigWall;}}
    return v;
}

static int profile_score_ctx(int idx,int source,const wchar_t*origin){
    if(idx<0||idx>=A.profileCount)return -999999;MapProfile *q=&A.profiles[idx];if(!source_allowed_for_ctx(q->source,q->origin,source,origin))return -999999;int score=0;
    if(source==SRC_MOD&&q->source==SRC_MOD&&origin&&origin[0]&&_wcsicmp(q->origin,origin)==0&&q->rulePath[0]){wchar_t pb[96];path_basename_noext(q->rulePath,pb,96);if(_wcsicmp(pb,L"workshop_exports")==0)score+=5000;else if(_wcsicmp(pb,L"000_workshop_generated")==0)score+=4000;}
    if(source==SRC_MOD){if(q->source==SRC_MOD)score+=1200;else if(q->source==SRC_OXCE)score+=900;else if(q->source==SRC_TFTD)score+=800;}
    else if(source==SRC_OXCE){if(q->source==SRC_OXCE)score+=1000;else if(q->source==SRC_TFTD)score+=800;}
    else if(source==SRC_TFTD){if(q->source==SRC_TFTD)score+=1000;else if(q->source==SRC_OXCE)score+=700;}
    else {if(q->source==SRC_MANUAL)score+=1100;else if(q->source==SRC_TFTD)score+=800;else if(q->source==SRC_OXCE)score+=700;}
    for(int k=0;k<q->dataSetCount;k++){int lib=library_find_name_ctx(q->dataSets[k],source,origin);if(lib>=0)score+=10;else score-=50;}return score;
}
static int profile_find_best_ctx(const wchar_t *mapName,int source,const wchar_t*origin){
    int best=-1,bestScore=-999999;for(int i=0;i<A.profileCount;i++)if(_wcsicmp(A.profiles[i].mapName,mapName)==0){int sc=profile_score_ctx(i,source,origin);if(sc>bestScore){best=i;bestScore=sc;}}return best;
}
static int profile_find_family_ctx(const wchar_t*family,int source,const wchar_t*origin){
    if(!family||!family[0])return -1;int best=-1,bestScore=-999999;for(int i=0;i<A.profileCount;i++){MapProfile*q=&A.profiles[i];if(q->kind!=0)continue;wchar_t pref[96];name_family(q->mapName,pref,96);if(_wcsicmp(pref,family)!=0)continue;int sc=profile_score_ctx(i,source,origin);if(sc>bestScore){best=i;bestScore=sc;}}return best;
}
static int map_index_by_path(const wchar_t*path){for(int i=0;i<A.mapCount;i++)if(_wcsicmp(A.maps[i].path,path)==0)return i;return -1;}
static int apply_profile_for_map(const wchar_t *path,int quiet){
    wchar_t name[96];path_basename_noext(path,name,96);int mi=map_index_by_path(path);int source=SRC_MANUAL;const wchar_t*origin=tr(L"MANUEL");
    if(mi>=0){source=A.maps[mi].source;origin=A.maps[mi].origin;}int pi=(mi>=0?A.maps[mi].profileIndex:profile_find_best_ctx(name,source,origin));A.autoProfileIndex=-1;if(pi<0)return 0;
    MapProfile *q=&A.profiles[pi];active_clear();int missing=0;
    for(int k=0;k<q->dataSetCount;k++){int lib=library_find_name_ctx(q->dataSets[k],source,origin);if(lib<0){missing++;continue;}if(active_add_lib(lib,0)<0){missing++;}}
    A.autoProfileIndex=pi;
    wchar_t st[512];if(!missing){if(mi>=0&&A.maps[mi].profileInferred)_snwprintf(st,511,tr(L"MAP %ls [LEGACY DORMANT] : profil familial OXCE infere (%ls, %d datasets)."),name,q->mapName,q->dataSetCount);else _snwprintf(st,511,tr(L"MAP %ls [%ls%ls%ls] : palette automatique chargee (%d datasets)."),name,source_name(source),source==SRC_MOD?L" / ":L"",source==SRC_MOD?origin:L"",q->dataSetCount);}else _snwprintf(st,511,tr(L"MAP %ls : palette auto partielle, %d dataset(s) introuvable(s)."),name,missing);set_status(st);
    if(missing&&!quiet)MessageBoxW(A.hwnd,tr(L"La MAP a ete identifiee, mais certains datasets requis n'ont pas ete trouves dans la meme source / priorite de mod."),APP_TITLE,MB_ICONWARNING);
    return 1;
}


static int map_path_exists(const wchar_t *path){for(int i=0;i<A.mapCount;i++)if(_wcsicmp(A.maps[i].path,path)==0)return 1;return 0;}
static void map_add(const wchar_t *path,int source,const wchar_t *origin){
    if(A.mapCount>=MAX_MAP_ENTRIES||map_path_exists(path))return;
    DWORD sz=0;uint8_t*b=read_all(path,&sz);if(!b||sz<3){free(b);return;}
    MapEntry *m=&A.maps[A.mapCount++];ZeroMemory(m,sizeof(*m));path_basename_noext(path,m->name,96);wcsncpy(m->path,path,PATH_CAP-1);m->source=source;if(origin){wcsncpy(m->origin,origin,95);m->origin[95]=0;}m->y=b[0];m->x=b[1];m->z=b[2];m->profileIndex=-1;m->rmpNodeCount=-1;m->legacyDormant=(source==SRC_TFTD&&legacy_dormant_info(m->name)!=NULL);
    wchar_t rp[PATH_CAP],mapDir[PATH_CAP],root[PATH_CAP];wcsncpy(mapDir,path,PATH_CAP-1);mapDir[PATH_CAP-1]=0;path_dirname(mapDir);wcsncpy(root,mapDir,PATH_CAP-1);root[PATH_CAP-1]=0;path_dirname(root);_snwprintf(rp,PATH_CAP-1,L"%ls\\ROUTES\\%ls.RMP",root,m->name);rp[PATH_CAP-1]=0;
    if(path_exists_file(rp)){DWORD rsz=0;uint8_t*rb=read_all(rp,&rsz);if(rb){m->rmpPresent=1;m->rmpNodeCount=(rsz%RMP_REC_SIZE==0)?(int)(rsz/RMP_REC_SIZE):-1;free(rb);}}
    free(b);
}
static int map_cmp(const MapEntry*a,const MapEntry*b){if(a->source!=b->source)return a->source-b->source;int o=_wcsicmp(a->origin,b->origin);if(o)return o;return _wcsicmp(a->name,b->name);}
static void map_sort(void){for(int i=1;i<A.mapCount;i++){MapEntry key=A.maps[i];int j=i-1;while(j>=0&&map_cmp(&A.maps[j],&key)>0){A.maps[j+1]=A.maps[j];j--;}A.maps[j+1]=key;}}
static void map_refresh_profiles(void){for(int i=0;i<A.mapCount;i++){MapEntry*m=&A.maps[i];m->profileInferred=0;m->profileIndex=profile_find_best_ctx(m->name,m->source,m->origin);if(m->profileIndex<0&&m->legacyDormant){const LegacyDormantInfo*di=legacy_dormant_info(m->name);if(di){m->profileIndex=profile_find_family_ctx(di->family,m->source,m->origin);if(m->profileIndex>=0)m->profileInferred=1;}}}}
static void scan_map_dir_recursive(const wchar_t*dir,int source,int depth,const wchar_t*origin){
    if(depth>32||!dir||!dir[0])return;wchar_t pat[PATH_CAP];_snwprintf(pat,PATH_CAP-1,L"%ls\\*",dir);pat[PATH_CAP-1]=0;WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);if(h==INVALID_HANDLE_VALUE)return;
    do{if(wcscmp(fd.cFileName,L".")==0||wcscmp(fd.cFileName,L"..")==0)continue;wchar_t full[PATH_CAP];_snwprintf(full,PATH_CAP-1,L"%ls\\%ls",dir,fd.cFileName);full[PATH_CAP-1]=0;if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)scan_map_dir_recursive(full,source,depth+1,origin);else if(has_ext(fd.cFileName,L".MAP"))map_add(full,source,origin);}while(FindNextFileW(h,&fd));FindClose(h);
}
static void scan_dir_recursive(const wchar_t *dir,int source,int depth,const wchar_t*origin){
    if(depth>32)return;wchar_t pat[PATH_CAP];_snwprintf(pat,PATH_CAP-1,L"%ls\\*",dir);pat[PATH_CAP-1]=0;WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);if(h==INVALID_HANDLE_VALUE)return;
    do{
        if(wcscmp(fd.cFileName,L".")==0||wcscmp(fd.cFileName,L"..")==0)continue;
        if((fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&(_wcsicmp(fd.cFileName,L".git")==0||_wcsicmp(fd.cFileName,L"node_modules")==0))continue;
        if(source==SRC_OXCE&&(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&(_wcsicmp(fd.cFileName,L"user")==0||_wcsicmp(fd.cFileName,L"mods")==0||_wcsicmp(fd.cFileName,L"xcom1")==0))continue;
        wchar_t full[PATH_CAP];_snwprintf(full,PATH_CAP-1,L"%ls\\%ls",dir,fd.cFileName);full[PATH_CAP-1]=0;
        if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)scan_dir_recursive(full,source,depth+1,origin);
        else{
            if(has_ext(fd.cFileName,L".MCD"))library_add_mcd(full,source,origin);
            else if(has_ext(fd.cFileName,L".MAP")){map_add(full,source,origin);if(source==SRC_TFTD)A.detectedMapsTFTD++;else if(source==SRC_OXCE)A.detectedMapsOXCE++;else if(source==SRC_MOD)A.detectedMapsMods++;}
            else if((source==SRC_OXCE||source==SRC_MOD)&&has_ext(fd.cFileName,L".rul")){if(source==SRC_MOD)A.detectedRulMods++;else A.detectedRulOXCE++;parse_ruleset_profiles(full,source,origin);parse_ruleset_generation(full,source,origin);parse_ruleset_mcd_patches(full,source,origin);}
            if(source==SRC_TFTD&&!A.paletteLoaded&&_wcsicmp(fd.cFileName,L"D0.LBM")==0)load_lbm_palette(full);
        }
    }while(FindNextFileW(h,&fd));FindClose(h);
}
static void scan_mods_root(const wchar_t*root){
    if(!root||!root[0]||!path_exists_dir(root))return;wchar_t pat[PATH_CAP];_snwprintf(pat,PATH_CAP-1,L"%ls\\*",root);WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);if(h==INVALID_HANDLE_VALUE)return;
    do{
        if(wcscmp(fd.cFileName,L".")==0||wcscmp(fd.cFileName,L"..")==0)continue;if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))continue;
        wchar_t full[PATH_CAP];_snwprintf(full,PATH_CAP-1,L"%ls\\%ls",root,fd.cFileName);full[PATH_CAP-1]=0;
        repair_workshop_generated_files(full);
        scan_dir_recursive(full,SRC_MOD,0,fd.cFileName);
        parse_workshop_manifest_profiles(full,fd.cFileName);
    }while(FindNextFileW(h,&fd));FindClose(h);
}
static void active_clear(void){ ZeroMemory(A.active,sizeof(A.active));A.activeCount=0;A.activeTotalMcd=0;A.expandedActive=-1;A.overviewDirty=1; }
static int active_find_lib(int lib){ for(int i=0;i<A.activeCount;i++)if(A.active[i].libIndex==lib)return i;return -1; }
static int active_add_lib(int lib,int noisy){
    if(lib<0||lib>=A.libraryCount)return -1;int a=active_find_lib(lib);if(a>=0)return A.active[a].baseIndex;
    /*
       Un MAP stocke chaque reference sur 8 bits, mais OXCE resout l'index en
       soustrayant successivement la taille de chaque dataset. La somme des
       datasets peut donc depasser 256 : seules les entrees dont l'index
       cumule final est <=255 sont adressables par ce MAP.
       V1.6/V1.7 refusaient a tort tout dataset faisant depasser la somme de
       256, meme si la tuile choisie restait parfaitement adressable.
    */
    LibrarySet *s=&A.library[lib];if(A.activeCount>=MAX_ACTIVE){if(noisy)MessageBoxW(A.hwnd,tr(L"Trop de datasets actifs pour le Workshop."),APP_TITLE,MB_ICONWARNING);return -1;}
    /* Une palette chargee depuis un vrai profil OXCE reste basee a 0, exactement
       comme le moteur. En revanche, sur une MAP neuve / palette manuelle,
       l'octet 0 signifie "aucun element". On reserve donc 0 avant le premier
       dataset ajoute manuellement afin que son MCD 000 devienne l'indice MAP 1. */
    if(noisy&&A.activeCount==0&&A.activeTotalMcd==0&&A.autoProfileIndex<0)A.activeTotalMcd=1;
    if(noisy&&A.map.cells&&A.activeCount>0){
        int lo=A.activeTotalMcd,hi=lo+s->mcdCount-1;if(hi>255)hi=255;int collisions=0;
        if(lo<=255)for(size_t ci=0,n=(size_t)A.map.x*A.map.y*A.map.z;ci<n;ci++)for(int p=0;p<4;p++){int raw=A.map.cells[ci].part[p];if(raw>=lo&&raw<=hi)collisions++;}
        if(collisions>0){
            wchar_t msg[700];_snwprintf(msg,699,tr(L"%d reference(s) MAP actuellement NON RESOLUE(S) utilisent deja les indices %d..%d.\n\nAjouter %ls maintenant les reinterpreterait comme ce dataset et ferait apparaitre des pieces fantomes.\n\nOUI : normaliser ces references invalides en vide, puis ajouter le dataset.\nNON : annuler pour verifier/reparer le profil."),collisions,lo,hi,s->name);
            if(MessageBoxW(A.hwnd,msg,tr(L"Collision d'indices detectee"),MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES)return -1;
            for(size_t ci=0,n=(size_t)A.map.x*A.map.y*A.map.z;ci<n;ci++)for(int p=0;p<4;p++){int raw=A.map.cells[ci].part[p];if(raw>=lo&&raw<=hi)A.map.cells[ci].part[p]=0;}
            A.map.dirty=1;A.overviewDirty=1;
        }
    }
    a=A.activeCount++;A.active[a].libIndex=lib;A.active[a].baseIndex=A.activeTotalMcd;A.activeTotalMcd+=s->mcdCount;A.overviewDirty=1;
    if(noisy){wchar_t st[320];int first=A.active[a].baseIndex,last=first+s->mcdCount-1,usableLast=last>255?255:last;
        if(first>255)_snwprintf(st,319,tr(L"%ls ajoute, mais aucune de ses entrees n'est adressable dans ce MAP (base %d > 255)."),s->name,first);
        else if(last>255)_snwprintf(st,319,tr(L"%ls ajoute : indices MAP %d a %d utilisables ; les entrees suivantes depassent 255."),s->name,first,usableLast);
        else _snwprintf(st,319,tr(L"%ls ajoute a la palette MAP : indices %d a %d."),s->name,first,last);
        set_status(st);
    }return A.active[a].baseIndex;
}
static int active_resolve_raw(int raw,int *lib,int *local){
    if(raw<=0)return 0;for(int i=0;i<A.activeCount;i++){int b=A.active[i].baseIndex;LibrarySet *s=&A.library[A.active[i].libIndex];if(raw>=b&&raw<b+s->mcdCount){*lib=A.active[i].libIndex;*local=raw-b;return 1;}}return 0;
}
static int raw_index_for_lib_local(int lib,int local,int autoAdd){
    if(lib<0||lib>=A.libraryCount||local<0||local>=A.library[lib].mcdCount)return -1;
    int a=active_find_lib(lib),base;if(a<0){if(!autoAdd)return -1;base=active_add_lib(lib,1);if(base<0)return -1;}else base=A.active[a].baseIndex;int raw=base+local;
    if(raw==0){if(autoAdd)MessageBoxW(A.hwnd,tr(L"L'indice MAP 0 signifie 'aucun element'. Chargez cette piece via une palette manuelle ou un profil qui lui attribue un indice non nul."),APP_TITLE,MB_ICONINFORMATION);return -1;}
    if(raw>255){if(autoAdd){wchar_t msg[640];_snwprintf(msg,639,tr(L"Cette piece n'est pas adressable dans la liste de datasets actuelle.\n\n%ls / MCD %d : base dataset %d + index local %d = indice MAP %d.\n\nLe format MAP stocke cet indice sur 1 octet : 0 = vide, 1..255 = pieces adressables.\n\nSur une MAP neuve, la liste doit commencer vide ; retirez les datasets inutiles si cette limite est atteinte."),A.library[lib].name,local,base,local,raw);MessageBoxW(A.hwnd,msg,APP_TITLE,MB_ICONWARNING);}return -1;}
    return raw;
}
static int selected_raw_index(int autoAdd){return raw_index_for_lib_local(A.selectedLib,A.selectedLocal,autoAdd);}

static int blueprint_find_matches_for(int lib,int local,int*out,int cap){
    if(lib<0||lib>=A.libraryCount||local<0||local>=A.library[lib].mcdCount)return 0;
    int count=0;const wchar_t*name=A.library[lib].name;
    for(int bi=0;bi<BLUEPRINT_RC15_COUNT;bi++){const BlueprintDef*b=&BLUEPRINTS_RC15[bi];int match=0;
        for(int j=0;j<b->partCount;j++){const BlueprintPartDef*p=&BLUEPRINT_PARTS_RC15[b->firstPart+j];if(_wcsicmp(p->dataset,name)==0&&p->mcd==local){match=1;break;}}
        if(match){if(out&&count<cap)out[count]=bi;count++;}
    }return count;
}
static int custom_blueprint_find_matches_for(int lib,int local,int*out,int cap){
    if(lib<0||lib>=A.libraryCount||local<0||local>=A.library[lib].mcdCount)return 0;int count=0;const wchar_t*name=A.library[lib].name;
    for(int bi=0;bi<A.customBlueprintCount;bi++){CustomBlueprintDef*b=&A.customBlueprints[bi];int match=0;for(int j=0;j<b->partCount;j++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+j];if(_wcsicmp(p->dataset,name)==0&&p->mcd==local){match=1;break;}}if(match){if(out&&count<cap)out[count]=bi;count++;}}
    return count;
}
static int blueprint_anchor_part(int bi){
    if(bi<0||bi>=BLUEPRINT_RC15_COUNT||A.selectedLib<0||A.selectedLib>=A.libraryCount)return -1;const BlueprintDef*b=&BLUEPRINTS_RC15[bi];
    for(int j=0;j<b->partCount;j++){const BlueprintPartDef*p=&BLUEPRINT_PARTS_RC15[b->firstPart+j];if(_wcsicmp(p->dataset,A.library[A.selectedLib].name)==0&&p->mcd==A.selectedLocal)return j;}return -1;
}
static int custom_blueprint_anchor_part(int bi){
    if(bi<0||bi>=A.customBlueprintCount||A.selectedLib<0||A.selectedLib>=A.libraryCount)return -1;CustomBlueprintDef*b=&A.customBlueprints[bi];
    for(int j=0;j<b->partCount;j++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+j];if(_wcsicmp(p->dataset,A.library[A.selectedLib].name)==0&&p->mcd==A.selectedLocal)return j;}return -1;
}
static int blueprint_best_match(const int*matches,int n){
    if(!matches||n<=0)return -1;int best=matches[0];for(int i=1;i<n;i++){int bi=matches[i];const BlueprintDef*a=&BLUEPRINTS_RC15[best],*b=&BLUEPRINTS_RC15[bi];if(b->partCount>a->partCount||(b->partCount==a->partCount&&b->spanZ>a->spanZ)||(b->partCount==a->partCount&&b->spanZ==a->spanZ&&b->support>a->support))best=bi;}return best;
}
static int custom_blueprint_best_match(const int*matches,int n){
    if(!matches||n<=0)return -1;int best=matches[0];for(int i=1;i<n;i++){int bi=matches[i];CustomBlueprintDef*a=&A.customBlueprints[best],*b=&A.customBlueprints[bi];if(b->partCount>a->partCount||(b->partCount==a->partCount&&b->spanZ>a->spanZ))best=bi;}return best;
}
static int blueprint_current_matches_selection(void){
    if(A.blueprintMode==1)return A.blueprintIndex>=0&&A.blueprintIndex<BLUEPRINT_RC15_COUNT&&blueprint_anchor_part(A.blueprintIndex)>=0;
    if(A.blueprintMode==2)return A.customBlueprintIndex>=0&&A.customBlueprintIndex<A.customBlueprintCount&&custom_blueprint_anchor_part(A.customBlueprintIndex)>=0;
    return 0;
}
static void blueprint_refresh_for_selection(void){
    int hm[64],cm[64],hn=blueprint_find_matches_for(A.selectedLib,A.selectedLocal,hm,64),cn=custom_blueprint_find_matches_for(A.selectedLib,A.selectedLocal,cm,64);
    if(cn>0){A.customBlueprintIndex=custom_blueprint_best_match(cm,cn);A.blueprintMode=2;}
    else if(hn>0){A.blueprintIndex=blueprint_best_match(hm,hn);A.blueprintMode=1;}
    else{A.blueprintIndex=-1;A.customBlueprintIndex=-1;A.blueprintMode=0;}
}
static int blueprint_resolve_lib(const wchar_t*dataset){
    if(!dataset||!dataset[0])return -1;if(A.selectedLib>=0&&A.selectedLib<A.libraryCount&&_wcsicmp(A.library[A.selectedLib].name,dataset)==0)return A.selectedLib;
    for(int i=0;i<A.activeCount;i++){int li=A.active[i].libIndex;if(li>=0&&li<A.libraryCount&&_wcsicmp(A.library[li].name,dataset)==0)return li;}
    int source=current_map_source();const wchar_t*origin=current_map_origin();if(A.scene.active&&A.selectedLib>=0&&A.selectedLib<A.libraryCount){source=A.library[A.selectedLib].source;origin=A.library[A.selectedLib].origin;}return library_find_name_ctx(dataset,source,origin);
}

static void reindex_resources(void){
    proc_close_panel();
    scene_free();hd_cache_clear();library_clear();active_clear();profile_clear();generation_clear();A.mapCount=0;A.selectedMap=-1;A.detectedMapsTFTD=A.detectedMapsOXCE=A.detectedMapsMods=0;A.detectedRulOXCE=A.detectedRulMods=0;default_palette();
    if(A.tftdRoot[0]&&path_exists_dir(A.tftdRoot))scan_dir_recursive(A.tftdRoot,SRC_TFTD,0,L"TFTD ORIGINAL");
    if(A.oxceRoot[0]&&path_exists_dir(A.oxceRoot))scan_dir_recursive(A.oxceRoot,SRC_OXCE,0,L"OXCE STANDARD");
    if(A.modsRoot[0]&&path_exists_dir(A.modsRoot))scan_mods_root(A.modsRoot);
    if(A.tftdMapRoot[0]&&path_exists_dir(A.tftdMapRoot))scan_map_dir_recursive(A.tftdMapRoot,SRC_TFTD,0,L"TFTD ORIGINAL");
    if(A.oxceMapRoot[0]&&path_exists_dir(A.oxceMapRoot))scan_map_dir_recursive(A.oxceMapRoot,SRC_OXCE,0,L"OXCE STANDARD");
    library_sort();map_sort();map_refresh_profiles();A.treeScroll=0;for(int i=0;i<3;i++)A.sourceScroll[i]=0;if(CUI.hwnd)populate_composer_combos();
    if(A.map.cells&&A.map.path[0]){apply_profile_for_map(A.map.path,1);rmp_load_for_current_map();}
    wchar_t st[512];_snwprintf(st,511,tr(L"Indexation : TFTD %d datasets | OXCE %d | MODS %d | %d MAP | %d profils | %d MapScripts."),A.libraryCountTFTD,A.libraryCountOXCE,A.libraryCountMods,A.mapCount,A.profileCount,gOxcScriptCount);set_status(st);
}


static void scene_geo_free(SceneDoc*s){for(int i=0;i<s->geoCount;i++)free(s->geo[i].dynamic);free(s->geo);free(s->geoLookup);free(s->geoDecor);s->geo=NULL;s->geoLookup=NULL;s->geoDecor=NULL;s->geoCount=s->geoDecorCount=0;}
static int geo_validate(const GeoInstance*ins,int count,int grounded,int crease);
static int scene_geo_index(SceneDoc*s);
static void scene_geo_draw(int x,int y,int z,int sx,int sy);
static size_t scene_geo_bytes(const SceneDoc*s);
static void scene_geo_write(uint8_t*b,const SceneDoc*s);
static int scene_geo_read(const uint8_t*b,size_t size,SceneDoc*s);
static void scene_free(void){editor_reset();scene_geo_free(&A.scene);free(A.scene.cells);free(A.scene.plan);free(A.scene.macroFlags);ZeroMemory(&A.scene,sizeof(A.scene));A.planSelActive=A.planSelecting=A.planPasteMode=0;}
static SceneCell* scene_cell_at(int x,int y,int z){if(!A.scene.active||!A.scene.cells||x<0||y<0||z<0||x>=A.scene.x||y>=A.scene.y||z>=A.scene.z)return NULL;return &A.scene.cells[(z*A.scene.y+y)*A.scene.x+x];}
static PlanCell* plan_cell_at(int x,int y,int z){
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y,dz=A.scene.active?A.scene.z:A.map.z;
    PlanCell*p=A.scene.active?A.scene.plan:A.map.plan;if(!p||x<0||y<0||z<0||x>=dx||y>=dy||z>=dz)return NULL;return &p[(z*dy+y)*dx+x];
}
static void scene_clear_rect(int x0,int y0,int w,int h){
    if(!A.scene.active)return;
    for(int z=0;z<A.scene.z;z++)for(int y=y0;y<y0+h;y++)for(int x=x0;x<x0+w;x++){
        SceneCell*c=scene_cell_at(x,y,z);if(!c)continue;for(int p=0;p<4;p++){c->lib[p]=-1;c->local[p]=-1;}PlanCell*pc=plan_cell_at(x,y,z);if(pc)ZeroMemory(pc,sizeof(*pc));
    }
}
static int point_in_uso_slot(int tx,int ty){return A.scene.active&&A.scene.usoSlotActive&&tx>=A.scene.usoSlotX&&ty>=A.scene.usoSlotY&&tx<A.scene.usoSlotX+A.scene.usoSlotW&&ty<A.scene.usoSlotY+A.scene.usoSlotH;}

static int doc_zmax(void){return A.scene.active?A.scene.z:(A.map.cells?A.map.z:0);}
static int profile_resolve_raw(int pi,int raw,int source,const wchar_t*origin,int*lib,int*local){
    if(pi<0||pi>=A.profileCount||raw<=0)return 0;MapProfile*q=&A.profiles[pi];int v=raw;
    for(int k=0;k<q->dataSetCount;k++){int li=library_find_name_ctx(q->dataSets[k],source,origin);if(li<0)continue;int n=A.library[li].mcdCount;if(v<n){*lib=li;*local=v;return 1;}v-=n;}return 0;
}
static int paste_map_to_scene(int mapIndex,int ox,int oy,int oz){
    if(mapIndex<0||mapIndex>=A.mapCount||!A.scene.cells)return 0;MapEntry*m=&A.maps[mapIndex];int pi=m->profileIndex;if(pi<0)pi=profile_find_best_ctx(m->name,m->source,m->origin);if(pi<0)return 0;
    DWORD sz=0;uint8_t*b=read_all(m->path,&sz);if(!b||sz<3){free(b);return 0;}int sy=b[0],sx=b[1],szm=b[2];uint32_t need=3u+(uint32_t)sx*sy*szm*4u;if(need>sz){free(b);return 0;}uint32_t off=3;
    for(int fz=0;fz<szm;fz++){int z=oz+szm-1-fz;for(int y=0;y<sy;y++)for(int x=0;x<sx;x++){SceneCell*c=scene_cell_at(ox+x,oy+y,z);for(int p=0;p<4;p++){int raw=b[off++];if(!c||raw<=0)continue;int lib,local;if(profile_resolve_raw(pi,raw,m->source,m->origin,&lib,&local)){c->lib[p]=lib;c->local[p]=local;}}}}
    free(b);return 1;
}
static MapCell *cell_at(int x,int y,int z){ if(!A.map.cells||x<0||y<0||z<0||x>=A.map.x||y>=A.map.y||z>=A.map.z)return NULL;return &A.map.cells[(z*A.map.y+y)*A.map.x+x]; }
static void free_map(void){editor_reset(); free(A.map.cells);free(A.map.plan);ZeroMemory(&A.map,sizeof(A.map));rmp_clear();A.currentZ=0;A.hoverValid=0;A.overviewDirty=1;A.planSelActive=A.planSelecting=A.planPasteMode=0;reset_undo(); }
static int new_map(int x,int y,int z){ scene_free();free_map();active_clear();A.autoProfileIndex=-1;A.selectedMap=-1;A.map.x=x;A.map.y=y;A.map.z=z;size_t n=(size_t)x*y*z;A.map.cells=(MapCell*)calloc(n,sizeof(MapCell));A.map.plan=(PlanCell*)calloc(n,sizeof(PlanCell));if(!A.map.cells||!A.map.plan){free(A.map.cells);free(A.map.plan);ZeroMemory(&A.map,sizeof(A.map));return 0;}A.map.dirty=1;A.currentZ=z-1;A.panX=A.panY=0;A.overviewDirty=1;A.planSelActive=0;A.planDiagWarnMask=0;reset_undo();set_status(tr(L"Nouvelle MAP : vue classique + MODE PLAN disponibles. Les datasets seront ajoutes a mesure du placement."));InvalidateRect(A.hwnd,NULL,FALSE);return 1; }
static int load_map_file(const wchar_t *path){
    scene_free();DWORD sz=0;uint8_t *b=read_all(path,&sz);if(!b||sz<3){free(b);return 0;}int y=b[0],x=b[1],z=b[2];uint32_t need=3u+(uint32_t)x*y*z*4u;if(!x||!y||!z||need>sz){free(b);return 0;}free_map();A.map.x=x;A.map.y=y;A.map.z=z;size_t cellCount=(size_t)x*y*z;A.map.cells=(MapCell*)calloc(cellCount,sizeof(MapCell));A.map.plan=(PlanCell*)calloc(cellCount,sizeof(PlanCell));if(!A.map.cells||!A.map.plan){free(A.map.cells);free(A.map.plan);ZeroMemory(&A.map,sizeof(A.map));free(b);return 0;}
    uint32_t off=3;for(int fz=0;fz<z;fz++){int iz=z-1-fz;for(int fy=0;fy<y;fy++){int iy=fy;for(int fx=0;fx<x;fx++){MapCell*c=cell_at(fx,iy,iz);for(int p=0;p<4;p++)c->part[p]=b[off++];}}}
    wcsncpy(A.map.path,path,PATH_CAP-1);A.map.path[PATH_CAP-1]=0;A.map.dirty=0;A.currentZ=z-1;A.panX=A.panY=0;A.overviewDirty=1;A.planDiagWarnMask=0;reset_undo();free(b);if(!apply_profile_for_map(path,1)){active_clear();set_status(tr(L"MAP chargee, mais aucun profil de datasets correspondant n'a ete trouve dans les rulesets OXCE. Palette MAP vide."));}rmp_load_for_current_map();InvalidateRect(A.hwnd,NULL,FALSE);return 1;
}
static int save_map_file(const wchar_t *path){
    if(!A.map.cells)return 0;
    if(path_starts_with_ci(path,A.tftdRoot)||(path_starts_with_ci(path,A.oxceRoot)&&!path_starts_with_ci(path,A.modsRoot))){MessageBoxW(A.hwnd,tr(L"Ecriture refusee : TFTD ORIGINAL et OXCE STANDARD sont proteges en lecture seule.\n\nUtilisez Fichier > Exporter / copier vers un mod."),APP_TITLE,MB_ICONERROR);return 0;}
    uint32_t sz=3u+(uint32_t)A.map.x*A.map.y*A.map.z*4u;uint8_t*b=(uint8_t*)malloc(sz);if(!b)return 0;b[0]=(uint8_t)A.map.y;b[1]=(uint8_t)A.map.x;b[2]=(uint8_t)A.map.z;uint32_t off=3;
    for(int fz=0;fz<A.map.z;fz++){int iz=A.map.z-1-fz;for(int fy=0;fy<A.map.y;fy++){int iy=fy;for(int fx=0;fx<A.map.x;fx++){MapCell*c=cell_at(fx,iy,iz);for(int p=0;p<4;p++)b[off++]=c->part[p];}}}
    int ok=write_all(path,b,sz);free(b);if(ok){wcsncpy(A.map.path,path,PATH_CAP-1);A.map.dirty=0;}return ok;
}

static void ensure_backbuf(int w,int h){ if(w<=0||h<=0)return;if(w==A.backW&&h==A.backH&&A.backbuf)return;free(A.backbuf);A.backbuf=(uint32_t*)calloc((size_t)w*h,4);A.backW=w;A.backH=h;ZeroMemory(&A.bmi,sizeof(A.bmi));A.bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);A.bmi.bmiHeader.biWidth=w;A.bmi.bmiHeader.biHeight=-h;A.bmi.bmiHeader.biPlanes=1;A.bmi.bmiHeader.biBitCount=32;A.bmi.bmiHeader.biCompression=BI_RGB; }
static void clear_back(uint32_t c){ if(!A.backbuf)return;for(size_t i=0,n=(size_t)A.backW*A.backH;i<n;i++)A.backbuf[i]=c; }
static void putpx(int x,int y,uint32_t c){ if(x>=0&&y>=0&&x<A.backW&&y<A.backH)A.backbuf[(size_t)y*A.backW+x]=c; }
static void line_px(int x0,int y0,int x1,int y1,uint32_t c){ int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;for(;;){putpx(x0,y0,c);if(x0==x1&&y0==y1)break;int e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}} }
static void line_px_thick(int x0,int y0,int x1,int y1,uint32_t c){
    int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
    for(;;){for(int yy=-1;yy<=1;yy++)for(int xx=-1;xx<=1;xx++)putpx(x0+xx,y0+yy,c);if(x0==x1&&y0==y1)break;int e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}}
}
static uint32_t pal_color(uint8_t idx){JMColor c=A.palette[idx];return RGB32(c.r,c.g,c.b);}

static void hd_cache_clear(void){
    rh_clear();
    for(int i=0;i<gHdCacheCount;i++){free(gHdCache[i].pixels);gHdCache[i].pixels=NULL;}
    ZeroMemory(gHdCache,sizeof(gHdCache));gHdCacheCount=0;
}
static int hd_candidate_path(const wchar_t*mod,const wchar_t*dataset,int local,wchar_t*out,int cap){
    if(!A.modsRoot[0]||!mod||!mod[0]||!dataset||!dataset[0])return 0;
    _snwprintf(out,cap-1,L"%ls\\%ls\\Resources\\TFTD_HD\\Terrain\\%ls\\%03d.png",A.modsRoot,mod,dataset,local);out[cap-1]=0;
    return path_exists_file(out);
}
static int hd_custom_candidate_path(const wchar_t*dataset,int local,wchar_t*out,int cap){
    if(!A.hdCustomRoot[0]||!dataset||!dataset[0])return 0;
    /* Le chemin configure peut etre la racine du mod (cas normal), Resources, TFTD_HD ou directement Terrain. */
    _snwprintf(out,cap-1,L"%ls\\Resources\\TFTD_HD\\Terrain\\%ls\\%03d.png",A.hdCustomRoot,dataset,local);out[cap-1]=0;if(path_exists_file(out))return 1;
    _snwprintf(out,cap-1,L"%ls\\TFTD_HD\\Terrain\\%ls\\%03d.png",A.hdCustomRoot,dataset,local);out[cap-1]=0;if(path_exists_file(out))return 1;
    _snwprintf(out,cap-1,L"%ls\\Terrain\\%ls\\%03d.png",A.hdCustomRoot,dataset,local);out[cap-1]=0;if(path_exists_file(out))return 1;
    _snwprintf(out,cap-1,L"%ls\\%ls\\%03d.png",A.hdCustomRoot,dataset,local);out[cap-1]=0;return path_exists_file(out);
}
static int hd_find_path(int lib,int local,wchar_t*out,int cap){
    if(lib<0||lib>=A.libraryCount||local<0)return 0;LibrarySet*s=&A.library[lib];
    /* 'local' est deja Frame[0] du MCD : hd_load_entry effectue MCD -> Frame[0] -> PNG. */
    int provider=A.assetRenderMode>=3?(A.hdOverlayProvider==2?2:1):A.assetRenderMode;
    if(provider==2){
        /* Provider manuel purement graphique : aucune incidence sur profils/datasets. Absence PNG => fallback Legacy. */
        if(hd_custom_candidate_path(s->name,local,out,cap))return 1;
        return 0;
    }
    if(provider!=1||!A.modsRoot[0])return 0;
    if(s->source==SRC_MOD&&s->origin[0]&&hd_candidate_path(s->origin,s->name,local,out,cap))return 1;
    /* Provider graphique connu : il ne participe jamais a la resolution des profils/datasets. */
    if(hd_candidate_path(L"TFTD PNG remastered",s->name,local,out,cap))return 1;
    /* V2.6 : aucun fallback implicite vers un autre mod installe. */
    return 0;
}
static HDCacheEntry* hd_load_entry(int lib,int local){
    if(lib<0||lib>=A.libraryCount||local<0)return NULL;
    int frame=mcd_frame(lib,local);
    for(int i=0;i<gHdCacheCount;i++)if(gHdCache[i].lib==lib&&gHdCache[i].local==frame)return &gHdCache[i];
    if(gHdCacheCount>=HD_CACHE_MAX)return NULL;
    wchar_t path[PATH_CAP];if(!hd_find_path(lib,frame,path,PATH_CAP))return NULL;
    GpBitmap*bmp=NULL;if(GdipCreateBitmapFromFile(path,&bmp)!=Ok||!bmp)return NULL;
    UINT w=0,h=0;GdipGetImageWidth((GpImage*)bmp,&w);GdipGetImageHeight((GpImage*)bmp,&h);
    if(!w||!h||w>4096||h>4096){GdipDisposeImage((GpImage*)bmp);return NULL;}
    GpRect r={0,0,(INT)w,(INT)h};BitmapData bd;ZeroMemory(&bd,sizeof(bd));
    if(GdipBitmapLockBits(bmp,&r,ImageLockModeRead,PixelFormat32bppARGB,&bd)!=Ok){GdipDisposeImage((GpImage*)bmp);return NULL;}
    uint32_t*pix=(uint32_t*)malloc((size_t)w*h*4);if(!pix){GdipBitmapUnlockBits(bmp,&bd);GdipDisposeImage((GpImage*)bmp);return NULL;}
    for(UINT y=0;y<h;y++){
        BYTE*row=(BYTE*)bd.Scan0+(ptrdiff_t)y*bd.Stride;
        memcpy(pix+(size_t)y*w,row,(size_t)w*4);
    }
    GdipBitmapUnlockBits(bmp,&bd);GdipDisposeImage((GpImage*)bmp);
    HDCacheEntry*e=&gHdCache[gHdCacheCount++];ZeroMemory(e,sizeof(*e));e->lib=lib;e->local=frame;e->w=(int)w;e->h=(int)h;e->pixels=pix;wcsncpy(e->path,path,PATH_CAP-1);
    return e;
}
static uint32_t alpha_over(uint32_t dst,uint32_t src){
    unsigned a=(src>>24)&255;if(a==0)return dst;if(a==255)return 0xFF000000u|(src&0x00FFFFFFu);
    unsigned ia=255-a;
    unsigned sr=(src>>16)&255,sg=(src>>8)&255,sb=src&255;
    unsigned dr=(dst>>16)&255,dg=(dst>>8)&255,db=dst&255;
    unsigned r=(sr*a+dr*ia+127)/255,g=(sg*a+dg*ia+127)/255,b=(sb*a+db*ia+127)/255;
    return 0xFF000000u|(r<<16)|(g<<8)|b;
}
static int draw_hd_scaled(int lib,int local,int dx,int dy,int scale){
    HDCacheEntry*e=hd_load_entry(lib,local);if(!e||!e->pixels)return 0;
    int dw=TILE_W*scale,dh=TILE_H*scale;if(dw<=0||dh<=0)return 1;
    if(dx>=A.backW||dy>=A.backH||dx+dw<=0||dy+dh<=0)return 1;
    int x0=dx<0?-dx:0,y0=dy<0?-dy:0,x1=dw,y1=dh;
    if(dx+x1>A.backW)x1=A.backW-dx;if(dy+y1>A.backH)y1=A.backH-dy;
    for(int y=y0;y<y1;y++){int sy=(int)((int64_t)y*e->h/dh);if(sy>=e->h)sy=e->h-1;
        for(int x=x0;x<x1;x++){int sx=(int)((int64_t)x*e->w/dw);if(sx>=e->w)sx=e->w-1;
            uint32_t sp=e->pixels[(size_t)sy*e->w+sx];editor_apply_opacity(&sp);if((sp>>24)==0)continue;
            size_t di=(size_t)(dy+y)*A.backW+(dx+x);A.backbuf[di]=alpha_over(A.backbuf[di],sp);
        }
    }return 1;
}
static void draw_sprite_scaled(int lib,int local,int frame,int dx,int dy,int scale){
    if(A.assetRenderMode!=0&&draw_hd_scaled(lib,local,dx,dy,scale))return;
    if(!library_load(lib))return;LibrarySet*s=&A.library[lib];if(!s->sprites||frame<0||frame>=s->frameCount)return;uint8_t*sp=s->sprites+(size_t)frame*TILE_W*TILE_H;for(int y=0;y<TILE_H;y++)for(int x=0;x<TILE_W;x++){uint8_t p=sp[y*TILE_W+x];if(!p)continue;uint32_t c=pal_color(p);for(int yy=0;yy<scale;yy++)for(int xx=0;xx<scale;xx++){int px=dx+x*scale+xx,py=dy+y*scale+yy;if(px>=0&&py>=0&&px<A.backW&&py<A.backH){uint32_t sp=c;editor_apply_opacity(&sp);putpx(px,py,alpha_over(A.backbuf[(size_t)py*A.backW+px],sp));}}}
}
static void hangar_blit_part(uint32_t*buf,int bw,int bh,int lib,int local,int dx,int dy,int dw,int dh){
    if(!buf||bw<=0||bh<=0||dw<=0||dh<=0||lib<0||lib>=A.libraryCount||local<0)return;
    HDCacheEntry*he=NULL;if(A.assetRenderMode!=0)he=hd_load_entry(lib,local);
    LibrarySet*ls=NULL;uint8_t*legacy=NULL;int frame=-1;if(!he){if(!library_load(lib))return;ls=&A.library[lib];frame=mcd_frame(lib,local);if(!ls->sprites||frame<0||frame>=ls->frameCount)return;legacy=ls->sprites+(size_t)frame*TILE_W*TILE_H;}
    int x0=dx<0?-dx:0,y0=dy<0?-dy:0,x1=dw,y1=dh;if(dx+x1>bw)x1=bw-dx;if(dy+y1>bh)y1=bh-dy;if(x0>=x1||y0>=y1)return;
    for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){
        uint32_t sp=0;if(he){int sx=(int)((int64_t)x*he->w/dw),sy=(int)((int64_t)y*he->h/dh);if(sx>=he->w)sx=he->w-1;if(sy>=he->h)sy=he->h-1;sp=he->pixels[(size_t)sy*he->w+sx];}
        else{int sx=(int)((int64_t)x*TILE_W/dw),sy=(int)((int64_t)y*TILE_H/dh);if(sx>=TILE_W)sx=TILE_W-1;if(sy>=TILE_H)sy=TILE_H-1;uint8_t pi=legacy[sy*TILE_W+sx];if(pi)sp=pal_color(pi);}
        if((sp>>24)==0)continue;size_t di=(size_t)(dy+y)*bw+(dx+x);buf[di]=alpha_over(buf[di],sp);
    }
}
static int hangar_order_before(const CustomBlueprintPart*a,const CustomBlueprintPart*b){
    if(a->dz!=b->dz)return a->dz<b->dz;if(a->dy!=b->dy)return a->dy<b->dy;if(a->dx!=b->dx)return a->dx<b->dx;return a->layer<b->layer;
}
static int gHangarSortFirstPart=0;
static int hangar_order_compare(const void*va,const void*vb){
    int ia=*(const int*)va,ib=*(const int*)vb;const CustomBlueprintPart*a=&A.customBlueprintParts[gHangarSortFirstPart+ia],*b=&A.customBlueprintParts[gHangarSortFirstPart+ib];
    if(hangar_order_before(a,b))return -1;if(hangar_order_before(b,a))return 1;return 0;
}
static void draw_hangar_blueprint_preview(HDC hdc,const RECT*rc,int bi){
    if(!hdc||!rc)return;int w=rc->right-rc->left,h=rc->bottom-rc->top;if(w<40||h<40)return;
    HBRUSH bg=CreateSolidBrush(RGB(9,18,24));FillRect(hdc,rc,bg);DeleteObject(bg);HPEN pen=CreatePen(PS_SOLID,1,RGB(55,78,88));HGDIOBJ oldp=SelectObject(hdc,pen);Rectangle(hdc,rc->left,rc->top,rc->right,rc->bottom);SelectObject(hdc,oldp);DeleteObject(pen);
    SetBkMode(hdc,TRANSPARENT);if(bi<0||bi>=A.customBlueprintCount){SetTextColor(hdc,RGB(130,160,170));const wchar_t*t=tr(L"Selectionnez un blueprint du Hangar");TextOutW(hdc,rc->left+18,rc->top+18,t,(int)wcslen(t));return;}
    CustomBlueprintDef*b=&A.customBlueprints[bi];if(b->partCount<=0)return;int minx=0x3fffffff,miny=0x3fffffff,maxx=-0x3fffffff,maxy=-0x3fffffff;
    for(int i=0;i<b->partCount;i++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+i];int lib=blueprint_resolve_lib(p->dataset);int pl=(lib>=0)?mcd_plevel(lib,p->mcd):0;int px=(p->dx-p->dy)*16,py=(p->dx+p->dy)*8-p->dz*24-pl;if(px<minx)minx=px;if(py<miny)miny=py;if(px+32>maxx)maxx=px+32;if(py+40>maxy)maxy=py+40;}
    if(maxx<=minx||maxy<=miny)return;int margin=18,caption=34,vw=w-margin*2,vh=h-margin*2-caption;if(vw<4||vh<4)return;double sx=(double)vw/(double)(maxx-minx),sy=(double)vh/(double)(maxy-miny),scale=sx<sy?sx:sy;if(scale>8.0)scale=8.0;if(scale<0.002)scale=0.002;
    int rw=(int)((maxx-minx)*scale+0.5),rh=(int)((maxy-miny)*scale+0.5);if(rw<1)rw=1;if(rh<1)rh=1;int bw=vw,bh=vh;uint32_t*buf=(uint32_t*)malloc((size_t)bw*bh*4);if(!buf)return;for(size_t i=0,n=(size_t)bw*bh;i<n;i++)buf[i]=RGB32(9,18,24);
    int*order=(int*)malloc(sizeof(int)*b->partCount);if(!order){free(buf);return;}for(int i=0;i<b->partCount;i++)order[i]=i;gHangarSortFirstPart=b->firstPart;qsort(order,(size_t)b->partCount,sizeof(int),hangar_order_compare);
    int offx=(bw-rw)/2,offy=(bh-rh)/2;for(int oi=0;oi<b->partCount;oi++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+order[oi]];int lib=blueprint_resolve_lib(p->dataset);if(lib<0||p->mcd<0||p->mcd>=A.library[lib].mcdCount)continue;int pl=mcd_plevel(lib,p->mcd);int px=(p->dx-p->dy)*16,py=(p->dx+p->dy)*8-p->dz*24-pl;int ddx=offx+(int)((px-minx)*scale+0.5),ddy=offy+(int)((py-miny)*scale+0.5),dw=(int)(32*scale+0.5),dh=(int)(40*scale+0.5);if(dw<1)dw=1;if(dh<1)dh=1;hangar_blit_part(buf,bw,bh,lib,p->mcd,ddx,ddy,dw,dh);}
    free(order);BITMAPINFO bmi;ZeroMemory(&bmi,sizeof(bmi));bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bmi.bmiHeader.biWidth=bw;bmi.bmiHeader.biHeight=-bh;bmi.bmiHeader.biPlanes=1;bmi.bmiHeader.biBitCount=32;bmi.bmiHeader.biCompression=BI_RGB;StretchDIBits(hdc,rc->left+margin,rc->top+margin+caption,bw,bh,0,0,bw,bh,buf,&bmi,DIB_RGB_COLORS,SRCCOPY);free(buf);
    SetTextColor(hdc,RGB(225,235,238));TextOutW(hdc,rc->left+margin,rc->top+12,b->name,(int)wcslen(b->name));wchar_t info[220];_snwprintf(info,219,tr(L"%d pieces  |  etendue %dx%dx%d  |  preview auto-fit multi-Z"),b->partCount,b->spanX,b->spanY,b->spanZ);SetTextColor(hdc,RGB(130,180,190));TextOutW(hdc,rc->left+margin,rc->top+30,info,(int)wcslen(info));
}
static void project_tile(int x,int y,int z,int*sx,int*sy){*sx=(x-y)*16;*sy=(x+y)*8-z*24;}
static int normal_z_min(void){return (A.showComplete||A.showAllBelow)?0:A.currentZ;}
static int normal_z_max(void){return A.showComplete?doc_zmax()-1:A.currentZ;}
static int normal_z_visible(int z){return z>=normal_z_min()&&z<=normal_z_max();}
static const wchar_t* normal_visibility_label(void){return A.showComplete?tr(L"Vue complete >"):(A.showAllBelow?tr(L"Z actif + dessous >"):tr(L"Z actif seul >"));}
static void set_normal_visibility(int mode){
    A.showComplete=(mode==2);if(mode!=2)A.showAllBelow=(mode==1);
    if(A.configPath[0]){wchar_t value[8];_snwprintf(value,7,L"%d",mode);WritePrivateProfileStringW(CFG_SECTION,L"NormalVisibility",value,A.configPath);}
    CheckMenuItem(GetMenu(A.hwnd),IDM_Z_COMPLETE,MF_BYCOMMAND|(A.showComplete?MF_CHECKED:MF_UNCHECKED));
    set_status(A.showComplete?tr(L"Vue complete : tous les etages sont visibles. Placement et effacement sur le Z actif."):tr(L"Vue complete desactivee. Le niveau actif reste le niveau d'edition."));
    InvalidateRect(A.hwnd,NULL,FALSE);
}
static void world_origin(int*ox,int*oy){int vw=A.clientW-A.sidebarW-A.inspectorW;if(vw<200)vw=200;*ox=A.sidebarW+vw/2+A.panX;*oy=116+A.panY;}

static void blueprint_outline_cell(int x,int y,int z,uint32_t color){
    int ox,oy,sx,sy;world_origin(&ox,&oy);project_tile(x,y,z,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;int hw=16*A.zoom;int topx=sx+hw,topy=sy+24*A.zoom;
    line_px_thick(topx,topy,sx+32*A.zoom,sy+32*A.zoom,color);line_px_thick(sx+32*A.zoom,sy+32*A.zoom,topx,sy+40*A.zoom,color);line_px_thick(topx,sy+40*A.zoom,sx,sy+32*A.zoom,color);line_px_thick(sx,sy+32*A.zoom,topx,topy,color);
}
static void draw_single_piece_preview(void){
    if(A.rmp.editMode||A.blueprintMode||!A.hoverValid||A.selectedLib<0||A.selectedLib>=A.libraryCount||A.selectedLocal<0||A.selectedLocal>=A.library[A.selectedLib].mcdCount)return;
    int dxmax=A.scene.active?A.scene.x:A.map.x,dymax=A.scene.active?A.scene.y:A.map.y,dzmax=A.scene.active?A.scene.z:A.map.z;
    int tx=A.hoverX,ty=A.hoverY,tz=A.currentZ;if(tx<0||ty<0||tz<0||tx>=dxmax||ty>=dymax||tz>=dzmax)return;
    int occupied=0;if(A.scene.active){SceneCell*sc=scene_cell_at(tx,ty,tz);occupied=sc&&sc->lib[A.selectedLayer]>=0;}else{MapCell*mc=cell_at(tx,ty,tz);occupied=mc&&mc->part[A.selectedLayer]!=0;}
    int ox,oy,sx,sy;world_origin(&ox,&oy);project_tile(tx,ty,tz,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;
    int fr=mcd_frame(A.selectedLib,A.selectedLocal),pl=mcd_plevel(A.selectedLib,A.selectedLocal);draw_sprite_scaled(A.selectedLib,A.selectedLocal,fr,sx,sy-pl*A.zoom,A.zoom);
    blueprint_outline_cell(tx,ty,tz,occupied?0xFFFFB52Eu:0xFF37D6E8u);
}
static void draw_blueprint_capture_overlay(void){
    if(!A.blueprintCaptureMode)return;for(int i=0;i<A.blueprintCaptureCount;i++){BlueprintCapturePart*p=&A.blueprintCapture[i];if(!normal_z_visible(p->z))continue;blueprint_outline_cell(p->x,p->y,p->z,i==0?0xFFFFE050u:0xFFFF55D8u);}
}
static void draw_habitual_blueprint_preview(void){
    if(A.blueprintMode!=1||A.blueprintIndex<0||A.blueprintIndex>=BLUEPRINT_RC15_COUNT||!A.hoverValid||A.rmp.editMode)return;const BlueprintDef*b=&BLUEPRINTS_RC15[A.blueprintIndex];int aj=blueprint_anchor_part(A.blueprintIndex);if(aj<0)return;const BlueprintPartDef*a=&BLUEPRINT_PARTS_RC15[b->firstPart+aj];
    int dxmax=A.scene.active?A.scene.x:A.map.x,dymax=A.scene.active?A.scene.y:A.map.y,dzmax=A.scene.active?A.scene.z:A.map.z;if(dxmax<=0||dymax<=0||dzmax<=0)return;int ox,oy;world_origin(&ox,&oy);
    for(int j=0;j<b->partCount;j++){const BlueprintPartDef*p=&BLUEPRINT_PARTS_RC15[b->firstPart+j];int tx=A.hoverX+(p->dx-a->dx),ty=A.hoverY+(p->dy-a->dy),tz=A.currentZ+(p->dz-a->dz);uint32_t c=0xFF37D6E8u;if(tx<0||ty<0||tz<0||tx>=dxmax||ty>=dymax||tz>=dzmax)continue;int lib=blueprint_resolve_lib(p->dataset);if(lib<0||p->mcd<0||p->mcd>=A.library[lib].mcdCount){blueprint_outline_cell(tx,ty,tz,0xFFFF4A4Au);continue;}int occupied=0;if(A.scene.active){SceneCell*sc=scene_cell_at(tx,ty,tz);occupied=sc&&sc->lib[p->layer]>=0;}else{MapCell*mc=cell_at(tx,ty,tz);occupied=mc&&mc->part[p->layer]!=0;}if(occupied)c=0xFFFFB52Eu;int sx,sy;project_tile(tx,ty,tz,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;int fr=mcd_frame(lib,p->mcd),pl=mcd_plevel(lib,p->mcd);draw_sprite_scaled(lib,p->mcd,fr,sx,sy-pl*A.zoom,A.zoom);blueprint_outline_cell(tx,ty,tz,c);}
}
static void draw_custom_blueprint_preview(void){
    if(A.blueprintMode!=2||A.customBlueprintIndex<0||A.customBlueprintIndex>=A.customBlueprintCount||!A.hoverValid||A.rmp.editMode)return;CustomBlueprintDef*b=&A.customBlueprints[A.customBlueprintIndex];int aj=custom_blueprint_anchor_part(A.customBlueprintIndex);if(aj<0)return;CustomBlueprintPart*a=&A.customBlueprintParts[b->firstPart+aj];int dxmax=A.scene.active?A.scene.x:A.map.x,dymax=A.scene.active?A.scene.y:A.map.y,dzmax=A.scene.active?A.scene.z:A.map.z;if(dxmax<=0||dymax<=0||dzmax<=0)return;int ox,oy;world_origin(&ox,&oy);
    for(int j=0;j<b->partCount;j++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+j];int tx=A.hoverX+(p->dx-a->dx),ty=A.hoverY+(p->dy-a->dy),tz=A.currentZ+(p->dz-a->dz);uint32_t c=0xFFB86BFFu;if(tx<0||ty<0||tz<0||tx>=dxmax||ty>=dymax||tz>=dzmax)continue;int lib=blueprint_resolve_lib(p->dataset);if(lib<0||p->mcd<0||p->mcd>=A.library[lib].mcdCount){blueprint_outline_cell(tx,ty,tz,0xFFFF4A4Au);continue;}int occupied=0;if(A.scene.active){SceneCell*sc=scene_cell_at(tx,ty,tz);occupied=sc&&sc->lib[p->layer]>=0;}else{MapCell*mc=cell_at(tx,ty,tz);occupied=mc&&mc->part[p->layer]!=0;}if(occupied)c=0xFFFFB52Eu;int sx,sy;project_tile(tx,ty,tz,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;int fr=mcd_frame(lib,p->mcd),pl=mcd_plevel(lib,p->mcd);draw_sprite_scaled(lib,p->mcd,fr,sx,sy-pl*A.zoom,A.zoom);blueprint_outline_cell(tx,ty,tz,c);}
}
static void draw_placement_preview(void){
    if(E.moving||E.tool!=EDIT_PLACE){draw_blueprint_capture_overlay();return;}
    draw_blueprint_capture_overlay();if(A.blueprintCaptureMode)return;if(A.blueprintMode==1&&blueprint_current_matches_selection())draw_habitual_blueprint_preview();else if(A.blueprintMode==2&&blueprint_current_matches_selection())draw_custom_blueprint_preview();else draw_single_piece_preview();
}

static void draw_scene_slot_rect(int rx,int ry,int rw,int rh,uint32_t color){
    int ox,oy;world_origin(&ox,&oy);int z=A.currentZ;
    for(int y=ry;y<ry+rh;y++)for(int x=rx;x<rx+rw;x++){
        if(x<0||y<0||x>=A.scene.x||y>=A.scene.y)continue;
        if(x!=rx&&x!=rx+rw-1&&y!=ry&&y!=ry+rh-1)continue;
        int sx,sy;project_tile(x,y,z,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;int hw=16*A.zoom;int topx=sx+hw,topy=sy+24*A.zoom;
        line_px(topx,topy,sx+32*A.zoom,sy+32*A.zoom,color);line_px(sx+32*A.zoom,sy+32*A.zoom,topx,sy+40*A.zoom,color);line_px(topx,sy+40*A.zoom,sx,sy+32*A.zoom,color);line_px(sx,sy+32*A.zoom,topx,topy,color);
    }
}
static void draw_scene_macro_reservations(void){
    if(!A.showGrid||!A.scene.active||!A.scene.macroFlags)return;
    for(int my=0;my<A.scene.macroH;my++)for(int mx=0;mx<A.scene.macroW;mx++){uint8_t f=A.scene.macroFlags[my*A.scene.macroW+mx];if(!(f&(SCENE_MACRO_CRAFT|SCENE_MACRO_USO)))continue;int x=mx*10,y=my*10,w=(x+10<=A.scene.x)?10:A.scene.x-x,h=(y+10<=A.scene.y)?10:A.scene.y-y;uint32_t c=(f&SCENE_MACRO_CRAFT)?0xFF55D77Au:0xFFFFB52Eu;draw_scene_slot_rect(x,y,w,h,c);if(f&SCENE_MACRO_UNDERLAY){int ox,oy,sx,sy;world_origin(&ox,&oy);project_tile(x+w/2,y+h/2,A.currentZ,&sx,&sy);sx=ox+sx*A.zoom+16*A.zoom;sy=oy+sy*A.zoom+32*A.zoom;plan_draw_underlay_marker(sx,sy,clampi(2*A.zoom,2,6));}}
}
static void plan_geometry(int*ox,int*oy,int*cell){
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;
    int vw=A.clientW-A.sidebarW-A.inspectorW;if(vw<200)vw=200;int vh=A.clientH-150;if(vh<160)vh=160;
    *cell=clampi(12*A.zoom,8,64);*ox=A.sidebarW+vw/2-(dx*(*cell))/2+A.panX;*oy=108+vh/2-(dy*(*cell))/2+A.panY;
}
static void plan_fill_px(int l,int t,int r,int b,uint32_t c){
    if(l>r){int q=l;l=r;r=q;}if(t>b){int q=t;t=b;b=q;}l=clampi(l,0,A.backW);r=clampi(r,0,A.backW);t=clampi(t,0,A.backH);b=clampi(b,0,A.backH);
    for(int y=t;y<b;y++)for(int x=l;x<r;x++)A.backbuf[(size_t)y*A.backW+x]=c;
}
static int plan_actual_layer(int x,int y,int z,int part,int*outLib,int*outLocal){
    if(outLib)*outLib=-1;if(outLocal)*outLocal=-1;if(part<0||part>3)return 0;
    if(A.scene.active){SceneCell*c=scene_cell_at(x,y,z);if(!c||c->lib[part]<0||c->local[part]<0)return 0;if(outLib)*outLib=c->lib[part];if(outLocal)*outLocal=c->local[part];return 1;}
    MapCell*c=cell_at(x,y,z);if(!c||c->part[part]==0)return 0;int lib=-1,local=-1;if(!active_resolve_raw(c->part[part],&lib,&local))return 0;if(outLib)*outLib=lib;if(outLocal)*outLocal=local;return 1;
}
static int plan_contains_ci(const wchar_t*hay,const wchar_t*needle){if(!hay||!needle)return 0;wchar_t h[128],n[64];wcsncpy(h,hay,127);h[127]=0;wcsncpy(n,needle,63);n[63]=0;CharUpperBuffW(h,(DWORD)wcslen(h));CharUpperBuffW(n,(DWORD)wcslen(n));return wcsstr(h,n)!=NULL;}
/* Semantique automatique PLAN : ne pas deduire le sens d'une piece uniquement du nom du dataset.
   Les cartes UFO/base alien melangent de nombreux roles dans le meme MCD. OXCE stocke justement
   le role explicite dans MCD.Target_Type (offset 59). Ces valeurs sont donc une source fiable pour
   colorer les objets et, lorsqu'une salle fermee ne contient qu'un seul role explicite, son sol. */
static int plan_target_zone_semantic(int target){
    switch(target){
        case 2: return PLAN_POWER;       /* UFO_POWER_SOURCE */
        case 3: return PLAN_CONTROL;     /* UFO_NAVIGATION */
        case 4: return PLAN_ENGINEERING; /* UFO_CONSTRUCTION */
        case 5: return PLAN_STORAGE;     /* ALIEN_FOOD */
        case 6: return PLAN_HABITAT;     /* ALIEN_REPRODUCTION */
        case 7: return PLAN_RESTAURANT;  /* ALIEN_ENTERTAINMENT */
        case 8: return PLAN_LAB;         /* ALIEN_SURGERY */
        case 9: return PLAN_LAB;         /* EXAM_ROOM */
        case 10:return PLAN_CARGO;       /* ALIEN_ALLOYS */
        case 11:return PLAN_HABITAT;     /* ALIEN_HABITAT */
        default:return PLAN_AUTO;
    }
}
static uint32_t plan_target_object_color(int target){
    int sem=plan_target_zone_semantic(target);
    return (sem>PLAN_AUTO&&sem<PLAN_SEMANTIC_COUNT)?PLAN_COLORS[sem]:0;
}
static const wchar_t* plan_target_object_name(int target){
    switch(target){
        case 2:return tr(L"ENERGIE");case 3:return tr(L"NAVIGATION");case 4:return tr(L"CONSTRUCTION");
        case 5:return tr(L"NOURRITURE");case 6:return tr(L"REPRODUCTION");case 7:return tr(L"DIVERTISSEMENT");
        case 8:return tr(L"CHIRURGIE");case 9:return tr(L"EXAMEN");case 10:return tr(L"ALLIAGES");case 11:return tr(L"HABITAT");
        default:return NULL;
    }
}
static int plan_object_semantic_from_lib(int lib){
    if(lib<0||lib>=A.libraryCount)return PLAN_OBJ_OBJECT;const wchar_t*n=A.library[lib].name;
    if(plan_contains_ci(n,L"DEBR")||plan_contains_ci(n,L"WRECK")||plan_contains_ci(n,L"RUBBLE")||plan_contains_ci(n,L"JUNK")||plan_contains_ci(n,L"ASUNK")||plan_contains_ci(n,L"XBITS")||plan_contains_ci(n,L"UFOBITS"))return PLAN_OBJ_DEBRIS;
    if(plan_contains_ci(n,L"KELP"))return PLAN_OBJ_KELP;
    if(plan_contains_ci(n,L"WEED")||plan_contains_ci(n,L"PLANT")||plan_contains_ci(n,L"GRASS")||plan_contains_ci(n,L"ALGA"))return PLAN_OBJ_VEGETATION;
    if(plan_contains_ci(n,L"CORAL")||plan_contains_ci(n,L"REEF"))return PLAN_OBJ_REEF;
    if(plan_contains_ci(n,L"ROCK")||plan_contains_ci(n,L"STONE"))return PLAN_OBJ_ROCK;
    if(plan_contains_ci(n,L"CARGO")||plan_contains_ci(n,L"CRATE")||plan_contains_ci(n,L"BOX"))return PLAN_OBJ_CARGO;
    return PLAN_OBJ_OBJECT;
}
static int plan_floor_semantic_from_lib(int lib){if(lib<0||lib>=A.libraryCount)return PLAN_FLOOR;const wchar_t*n=A.library[lib].name;if(plan_contains_ci(n,L"SAND"))return PLAN_SAND;if(plan_contains_ci(n,L"SEA")||plan_contains_ci(n,L"WATER"))return PLAN_WATER;if(plan_contains_ci(n,L"ROCK")||plan_contains_ci(n,L"STONE"))return PLAN_ROCK;if(plan_contains_ci(n,L"CORAL")||plan_contains_ci(n,L"REEF"))return PLAN_REEF;if(plan_contains_ci(n,L"CAVE"))return PLAN_CAVE;return PLAN_FLOOR;}
static int plan_floor_base_effective(int x,int y,int z){PlanCell*p=plan_cell_at(x,y,z);if(p&&p->semantic!=PLAN_AUTO)return p->semantic;int lib=-1,local=-1;(void)local;if(plan_actual_layer(x,y,z,0,&lib,&local))return plan_floor_semantic_from_lib(lib);return PLAN_AUTO;}
static int plan_object_zone_seed(int x,int y,int z){int lib=-1,local=-1;if(!plan_actual_layer(x,y,z,3,&lib,&local))return PLAN_AUTO;return plan_target_zone_semantic(mcd_u8(lib,local,59));}
static int plan_floor_effective(int x,int y,int z){int sem=plan_floor_base_effective(x,y,z);if(sem==PLAN_FLOOR){int seed=plan_object_zone_seed(x,y,z);if(seed!=PLAN_AUTO)return seed;}return sem;}
static int plan_object_effective(int x,int y,int z){
    PlanCell*p=plan_cell_at(x,y,z);if(p&&p->objectSemantic!=PLAN_OBJ_AUTO)return p->objectSemantic;
    int lib=-1,local=-1;if(!plan_actual_layer(x,y,z,3,&lib,&local))return PLAN_OBJ_AUTO;
    /* Un BigWall est de la geometrie de mur, pas un decor : ne pas rajouter un carre blanc par-dessus. */
    if(mcd_bigwall_effective(lib,local)!=0)return PLAN_OBJ_AUTO;
    return plan_object_semantic_from_lib(lib);
}
static uint32_t plan_object_visual_color(int x,int y,int z,int obj){
    PlanCell*p=plan_cell_at(x,y,z);
    if(p&&p->objectSemantic!=PLAN_OBJ_AUTO)return (obj>PLAN_OBJ_AUTO&&obj<PLAN_OBJECT_COUNT)?PLAN_OBJECT_COLORS[obj]:0;
    int lib=-1,local=-1;if(!plan_actual_layer(x,y,z,3,&lib,&local))return 0;
    if(mcd_bigwall_effective(lib,local)!=0)return 0;
    uint32_t special=plan_target_object_color(mcd_u8(lib,local,59));if(special)return special;
    if(mcd_u8(lib,local,34))return 0xFF43C8D8u; /* GravLift */
    return (obj>PLAN_OBJ_AUTO&&obj<PLAN_OBJECT_COUNT)?PLAN_OBJECT_COLORS[obj]:0;
}
static const wchar_t* plan_object_effective_name(int x,int y,int z,int obj){
    PlanCell*p=plan_cell_at(x,y,z);if(p&&p->objectSemantic!=PLAN_OBJ_AUTO)return plan_object_name(obj);
    int lib=-1,local=-1;if(plan_actual_layer(x,y,z,3,&lib,&local)){
        if(mcd_bigwall_effective(lib,local)!=0)return tr(L"STRUCTURE MUR");
        const wchar_t*n=plan_target_object_name(mcd_u8(lib,local,59));if(n)return n;
        if(mcd_u8(lib,local,34))return tr(L"ASCENSEUR");
    }
    return plan_object_name(obj);
}
static int plan_actual_wall(int x,int y,int z,int edge){
    int lib=-1,local=-1;
    if(edge==1)return plan_actual_layer(x,y,z,1,&lib,&local)?(mcd_u8(lib,local,35)?2:1):0;
    if(edge==2)return plan_actual_layer(x,y,z,2,&lib,&local)?(mcd_u8(lib,local,35)?2:1):0;
    if(edge==3)return plan_actual_layer(x+1,y,z,1,&lib,&local)?(mcd_u8(lib,local,35)?2:1):0;
    if(edge==4)return plan_actual_layer(x,y+1,z,2,&lib,&local)?(mcd_u8(lib,local,35)?2:1):0;
    return 0;
}
static int plan_edge_effective(int x,int y,int z,int edge){
    PlanCell*p=plan_cell_at(x,y,z);if(!p)return 0;int v=edge==1?p->west:edge==2?p->north:edge==3?p->east:p->south;if(v==3)return 0;if(v)return v;
    if(edge==3){PlanCell*n=plan_cell_at(x+1,y,z);if(n){if(n->west==3)return 0;if(n->west)return n->west;}}
    if(edge==4){PlanCell*n=plan_cell_at(x,y+1,z);if(n){if(n->north==3)return 0;if(n->north)return n->north;}}
    return plan_actual_wall(x,y,z,edge);
}
static int plan_actual_diag(int x,int y,int z){int lib=-1,local=-1;if(!plan_actual_layer(x,y,z,3,&lib,&local))return 0;int bw=mcd_bigwall_effective(lib,local);return (bw==2||bw==3)?bw:0;}
static int plan_diag_effective(int x,int y,int z){PlanCell*p=plan_cell_at(x,y,z);if(!p)return 0;if(p->diag==PLAN_DIAG_FORCE_NONE)return 0;if(p->diag==2||p->diag==3)return p->diag;return plan_actual_diag(x,y,z);}
static int plan_object_bigwall_blocks_edge(int x,int y,int z,int edge){
    int lib=-1,local=-1;if(!plan_actual_layer(x,y,z,3,&lib,&local))return 0;int bw=mcd_bigwall_effective(lib,local);
    if(bw==1||bw==2||bw==3)return bw!=0;
    if(edge==1)return bw==4||bw==9; /* OUEST */
    if(edge==2)return bw==5||bw==9; /* NORD */
    if(edge==3)return bw==6||bw==8; /* EST */
    if(edge==4)return bw==7||bw==8; /* SUD */
    return 0;
}
static int plan_room_transition_open(int x,int y,int z,int nx,int ny,int edge){
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(nx<0||ny<0||nx>=dx||ny>=dy)return 0;
    int opp=edge==1?3:edge==2?4:edge==3?1:2;
    if(plan_edge_effective(x,y,z,edge))return 0;
    if(plan_object_bigwall_blocks_edge(x,y,z,edge)||plan_object_bigwall_blocks_edge(nx,ny,z,opp))return 0;
    return 1;
}
static int plan_auto_room_candidate(int x,int y,int z){
    PlanCell*p=plan_cell_at(x,y,z);if(!p||p->semantic!=PLAN_AUTO)return 0;
    if(!plan_actual_layer(x,y,z,0,NULL,NULL))return 0;
    return plan_floor_base_effective(x,y,z)==PLAN_FLOOR;
}
static uint8_t* plan_build_render_floor_layer(int z){
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(dx<=0||dy<=0)return NULL;size_t n=(size_t)dx*dy;
    uint8_t*out=(uint8_t*)malloc(n),*seen=(uint8_t*)calloc(n,1);int*q=(int*)malloc(n*sizeof(int));if(!out||!seen||!q){free(out);free(seen);free(q);return NULL;}
    for(int y=0;y<dy;y++)for(int x=0;x<dx;x++)out[(size_t)y*dx+x]=(uint8_t)plan_floor_base_effective(x,y,z);
    for(int sy=0;sy<dy;sy++)for(int sx=0;sx<dx;sx++){
        int start=sy*dx+sx;if(seen[start]||!plan_auto_room_candidate(sx,sy,z))continue;int h=0,t=0,seed=PLAN_AUTO,mixed=0;q[t++]=start;seen[start]=1;
        while(h<t){int id=q[h++],x=id%dx,y=id/dx;int role=plan_object_zone_seed(x,y,z);if(role!=PLAN_AUTO){if(seed==PLAN_AUTO)seed=role;else if(seed!=role)mixed=1;}
            const int nx[4]={x-1,x,x+1,x},ny[4]={y,y-1,y,y+1},ed[4]={1,2,3,4};
            for(int k=0;k<4;k++){int xx=nx[k],yy=ny[k];if(xx<0||yy<0||xx>=dx||yy>=dy)continue;int ni=yy*dx+xx;if(seen[ni]||!plan_auto_room_candidate(xx,yy,z))continue;if(!plan_room_transition_open(x,y,z,xx,yy,ed[k]))continue;seen[ni]=1;q[t++]=ni;}
        }
        if(seed!=PLAN_AUTO&&!mixed)for(int i=0;i<t;i++)out[q[i]]=(uint8_t)seed;
        else if(mixed)for(int i=0;i<t;i++){int id=q[i],x=id%dx,y=id/dx,role=plan_object_zone_seed(x,y,z);if(role!=PLAN_AUTO)out[id]=(uint8_t)role;}
    }
    free(seen);free(q);return out;
}
static int plan_floor_display_effective(int x,int y,int z){
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(x<0||y<0||x>=dx||y>=dy)return PLAN_AUTO;uint8_t*a=plan_build_render_floor_layer(z);if(!a)return plan_floor_effective(x,y,z);int v=a[y*dx+x];free(a);return v;
}
static int plan_floor_shape_from_sprite(int lib,int local){
    if(lib<0||lib>=A.libraryCount||local<0)return PLAN_FSHAPE_FULL;if(!library_load(lib))return PLAN_FSHAPE_FULL;LibrarySet*s=&A.library[lib];if(local>=s->mcdCount)return PLAN_FSHAPE_FULL;if(s->floorShapeCache&&s->floorShapeCache[local]!=0xFF)return s->floorShapeCache[local];
    int fr=mcd_frame(lib,local),shape=PLAN_FSHAPE_FULL;if(fr>=0&&fr<s->frameCount&&s->sprites){uint8_t*sp=s->sprites+(size_t)fr*TILE_W*TILE_H;int minx=TILE_W,maxx=-1,miny=TILE_H,maxy=-1,total=0;for(int y=0;y<TILE_H;y++)for(int x=0;x<TILE_W;x++)if(sp[y*TILE_W+x]){if(x<minx)minx=x;if(x>maxx)maxx=x;if(y<miny)miny=y;if(y>maxy)maxy=y;total++;}if(total>8&&maxx>=minx&&maxy>=miny){int cx=(minx+maxx)/2,cy=(miny+maxy)/2,top=0,bottom=0,left=0,right=0;for(int y=miny;y<=maxy;y++)for(int x=minx;x<=maxx;x++)if(sp[y*TILE_W+x]){if(y<=cy)top++;else bottom++;if(x<=cx)left++;else right++;}int strong=17;if(top*10>bottom*strong)shape=PLAN_FSHAPE_TRI_NO;else if(bottom*10>top*strong)shape=PLAN_FSHAPE_TRI_SE;else if(right*10>left*strong)shape=PLAN_FSHAPE_TRI_NE;else if(left*10>right*strong)shape=PLAN_FSHAPE_TRI_SO;}}
    if(s->floorShapeCache)s->floorShapeCache[local]=(uint8_t)shape;return shape;
}
static int plan_neighbor_has_floor(int x,int y,int z){return plan_floor_effective(x,y,z)!=PLAN_AUTO||plan_actual_layer(x,y,z,0,NULL,NULL);}
static int plan_floor_shape_effective(int x,int y,int z){
    PlanCell*p=plan_cell_at(x,y,z);if(p&&p->floorShape>PLAN_FSHAPE_AUTO&&p->floorShape<=PLAN_FSHAPE_TRI_SO)return p->floorShape;int lib=-1,local=-1;if(plan_actual_layer(x,y,z,0,&lib,&local)){int sh=plan_floor_shape_from_sprite(lib,local);if(sh!=PLAN_FSHAPE_FULL)return sh;}
    int d=plan_diag_effective(x,y,z);if(d==2){int a=plan_neighbor_has_floor(x,y-1,z)+plan_neighbor_has_floor(x-1,y,z),b=plan_neighbor_has_floor(x,y+1,z)+plan_neighbor_has_floor(x+1,y,z);if(a>b)return PLAN_FSHAPE_TRI_NO;if(b>a)return PLAN_FSHAPE_TRI_SE;}else if(d==3){int a=plan_neighbor_has_floor(x,y-1,z)+plan_neighbor_has_floor(x+1,y,z),b=plan_neighbor_has_floor(x,y+1,z)+plan_neighbor_has_floor(x-1,y,z);if(a>b)return PLAN_FSHAPE_TRI_NE;if(b>a)return PLAN_FSHAPE_TRI_SO;}return PLAN_FSHAPE_FULL;
}
static uint32_t plan_dim_color(uint32_t c,int percent);
static void plan_line_edge_dim(int x0,int y0,int x1,int y1,int type,int dim){
    uint32_t c=plan_dim_color(type==2?0xFFFFB52Eu:0xFFE8EFF2u,dim);line_px_thick(x0,y0,x1,y1,c);if(type==2){int mx=(x0+x1)/2,my=(y0+y1)/2;uint32_t gap=plan_dim_color(0xFF172028u,dim);for(int d=-2;d<=2;d++)putpx(mx+d,my,gap);}
}
static void plan_line_edge_context_dim(int x0,int y0,int x1,int y1,int type,int dim,int hasDiag){
    if(hasDiag&&type!=2){
        int d=dim*48/100;if(d<18)d=18;
        line_px(x0,y0,x1,y1,plan_dim_color(0xFFB9C8CDu,d));
    }else plan_line_edge_dim(x0,y0,x1,y1,type,dim);
}
static void plan_line_diag_dim(int x0,int y0,int x1,int y1,int dim){line_px_thick(x0,y0,x1,y1,plan_dim_color(0xFFF2F6F7u,dim));}
static void plan_draw_padlock_symbol(int cx,int cy,int r,uint32_t c){
    if(r<2)r=2;if(r>5)r=5;
    plan_fill_px(cx-r,cy,cx+r+1,cy+r+2,c);
    line_px(cx-r+1,cy,cx-r+1,cy-r,c);line_px(cx+r-1,cy,cx+r-1,cy-r,c);line_px(cx-r+1,cy-r,cx+r-1,cy-r,c);
    putpx(cx,cy+1,0xFF4A3B12u);
}
static void plan_draw_underlay_marker(int cx,int cy,int r){
    if(r<2)r=2;if(r>6)r=6;
    plan_fill_px(cx-r,cy-r,cx+r+1,cy+r+1,0xFFF4F7F8u);
    line_px(cx-r,cy-r,cx+r,cy-r,0xFF4C626Cu);line_px(cx-r,cy+r,cx+r,cy+r,0xFF4C626Cu);
    line_px(cx-r,cy-r,cx-r,cy+r,0xFF4C626Cu);line_px(cx+r,cy-r,cx+r,cy+r,0xFF4C626Cu);
}
static int plan_in_selection(int x,int y){if(!A.planSelActive)return 0;int x0=A.planSelX0<A.planSelX1?A.planSelX0:A.planSelX1,x1=A.planSelX0>A.planSelX1?A.planSelX0:A.planSelX1,y0=A.planSelY0<A.planSelY1?A.planSelY0:A.planSelY1,y1=A.planSelY0>A.planSelY1?A.planSelY0:A.planSelY1;return x>=x0&&x<=x1&&y>=y0&&y<=y1;}
static uint32_t plan_dim_color(uint32_t c,int percent){if(percent>=100)return c;if(percent<0)percent=0;uint32_t a=c&0xFF000000u;int r=(int)((c>>16)&255),g=(int)((c>>8)&255),b=(int)(c&255);r=r*percent/100;g=g*percent/100;b=b*percent/100;return a|((uint32_t)r<<16)|((uint32_t)g<<8)|(uint32_t)b;}
static void plan_fill_diamond(int cx,int cy,int rx,int ry,uint32_t c){if(rx<1||ry<1)return;for(int dy=-ry;dy<=ry;dy++){int ad=dy<0?-dy:dy;int span=rx*(ry-ad)/ry;int y=cy+dy;if(y<0||y>=A.backH)continue;int x0=clampi(cx-span,0,A.backW-1),x1=clampi(cx+span,0,A.backW-1);for(int x=x0;x<=x1;x++)A.backbuf[(size_t)y*A.backW+x]=c;}}
static long long plan_edge_fn(int ax,int ay,int bx,int by,int px,int py){return (long long)(px-ax)*(by-ay)-(long long)(py-ay)*(bx-ax);}
static void plan_fill_triangle(int ax,int ay,int bx,int by,int cx,int cy,uint32_t color){int minx=ax<bx?(ax<cx?ax:cx):(bx<cx?bx:cx),maxx=ax>bx?(ax>cx?ax:cx):(bx>cx?bx:cx),miny=ay<by?(ay<cy?ay:cy):(by<cy?by:cy),maxy=ay>by?(ay>cy?ay:cy):(by>cy?by:cy);minx=clampi(minx,0,A.backW-1);maxx=clampi(maxx,0,A.backW-1);miny=clampi(miny,0,A.backH-1);maxy=clampi(maxy,0,A.backH-1);long long area=plan_edge_fn(ax,ay,bx,by,cx,cy);for(int y=miny;y<=maxy;y++)for(int x=minx;x<=maxx;x++){long long e1=plan_edge_fn(ax,ay,bx,by,x,y),e2=plan_edge_fn(bx,by,cx,cy,x,y),e3=plan_edge_fn(cx,cy,ax,ay,x,y);if(area>=0?(e1>=0&&e2>=0&&e3>=0):(e1<=0&&e2<=0&&e3<=0))putpx(x,y,color);}}
static void plan_fill_square_shape(int l,int t,int r,int b,int shape,uint32_t c){if(shape==PLAN_FSHAPE_FULL||shape==PLAN_FSHAPE_AUTO){plan_fill_px(l,t,r,b,c);return;}if(shape==PLAN_FSHAPE_TRI_NO)plan_fill_triangle(l,t,r,t,l,b,c);else if(shape==PLAN_FSHAPE_TRI_NE)plan_fill_triangle(l,t,r,t,r,b,c);else if(shape==PLAN_FSHAPE_TRI_SE)plan_fill_triangle(r,t,r,b,l,b,c);else if(shape==PLAN_FSHAPE_TRI_SO)plan_fill_triangle(l,t,l,b,r,b,c);}
static void plan_fill_iso_shape(int cx,int cy,int rx,int ry,int shape,uint32_t c){int tx=cx,ty=cy-ry,rxp=cx+rx,ryp=cy,bx=cx,by=cy+ry,lx=cx-rx,ly=cy;if(shape==PLAN_FSHAPE_FULL||shape==PLAN_FSHAPE_AUTO){plan_fill_diamond(cx,cy,rx,ry,c);return;}if(shape==PLAN_FSHAPE_TRI_NO)plan_fill_triangle(tx,ty,rxp,ryp,lx,ly,c);else if(shape==PLAN_FSHAPE_TRI_NE)plan_fill_triangle(tx,ty,rxp,ryp,bx,by,c);else if(shape==PLAN_FSHAPE_TRI_SE)plan_fill_triangle(rxp,ryp,bx,by,lx,ly,c);else if(shape==PLAN_FSHAPE_TRI_SO)plan_fill_triangle(tx,ty,lx,ly,bx,by,c);}
static void plan_iso_points(int x,int y,int z,int*tx,int*ty,int*rx,int*ry){int ox,oy,sx,sy;world_origin(&ox,&oy);project_tile(x,y,z,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;*rx=16*A.zoom;*ry=8*A.zoom;*tx=sx+16*A.zoom;*ty=sy+32*A.zoom;}
static int plan_cell_has_semantic_content(int x,int y,int z){PlanCell*p=plan_cell_at(x,y,z);if(!p)return 0;if(plan_floor_effective(x,y,z)!=PLAN_AUTO||plan_object_effective(x,y,z)!=PLAN_OBJ_AUTO||p->vlink||p->flags||plan_diag_effective(x,y,z))return 1;for(int e=1;e<=4;e++)if(plan_edge_effective(x,y,z,e))return 1;return 0;}
static void plan_draw_vlink_symbol(int cx,int cy,int d,uint8_t v,uint32_t vc){if(!v)return;if(d<2)d=2;line_px_thick(cx-d,cy,cx+d,cy,vc);line_px_thick(cx,cy-d,cx,cy+d,vc);if(v==1){line_px(cx,cy-d,cx-2,cy-d+3,vc);line_px(cx,cy-d,cx+2,cy-d+3,vc);}else if(v==2){line_px(cx,cy+d,cx-2,cy+d-3,vc);line_px(cx,cy+d,cx+2,cy+d-3,vc);}else if(v==3)line_px_thick(cx-d,cy+d,cx+d,cy-d,vc);else if(v==4){line_px(cx-d/2,cy-d,cx-d/2,cy+d,vc);line_px(cx+d/2,cy-d,cx+d/2,cy+d,vc);}else if(v==5){line_px(cx-d,cy-d,cx+d,cy+d,0xFFFF6B6Bu);line_px(cx+d,cy-d,cx-d,cy+d,0xFFFF6B6Bu);}else if(v>=6){line_px_thick(cx-d,cy-d,cx+d,cy-d,vc);line_px_thick(cx-d,cy+d,cx+d,cy+d,vc);}}
static void plan_draw_scene_macro_2d(int ox,int oy,int cs){
    if(!A.showGrid||!A.scene.active||!A.scene.macroFlags)return;
    for(int my=0;my<A.scene.macroH;my++)for(int mx=0;mx<A.scene.macroW;mx++){
        uint8_t f=A.scene.macroFlags[my*A.scene.macroW+mx];if(!(f&(SCENE_MACRO_CRAFT|SCENE_MACRO_USO)))continue;
        int x0=ox+mx*10*cs,y0=oy+my*10*cs,x1=ox+((mx+1)*10>A.scene.x?A.scene.x:(mx+1)*10)*cs,y1=oy+((my+1)*10>A.scene.y?A.scene.y:(my+1)*10)*cs;
        uint32_t c=(f&SCENE_MACRO_CRAFT)?0xFF55D77Au:0xFFFFB52Eu;
        line_px_thick(x0,y0,x1,y0,c);line_px_thick(x0,y1,x1,y1,c);line_px_thick(x0,y0,x0,y1,c);line_px_thick(x1,y0,x1,y1,c);
        if(f&SCENE_MACRO_UNDERLAY)plan_draw_underlay_marker(x0+clampi(cs,4,9),y0+clampi(cs,4,9),clampi(cs/3,2,5));
    }
}
static void plan_draw_scene_macro_iso(void){
    if(!A.showGrid||!A.scene.active||!A.scene.macroFlags)return;
    for(int my=0;my<A.scene.macroH;my++)for(int mx=0;mx<A.scene.macroW;mx++){
        uint8_t f=A.scene.macroFlags[my*A.scene.macroW+mx];if(!(f&(SCENE_MACRO_CRAFT|SCENE_MACRO_USO)))continue;
        uint32_t c=(f&SCENE_MACRO_CRAFT)?0xFF55D77Au:0xFFFFB52Eu;
        int x0=mx*10,y0=my*10,x1=(mx+1)*10-1,y1=(my+1)*10-1;if(x1>=A.scene.x)x1=A.scene.x-1;if(y1>=A.scene.y)y1=A.scene.y-1;
        for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
            if(x!=x0&&x!=x1&&y!=y0&&y!=y1)continue;
            int cx,cy,rx,ry;plan_iso_points(x,y,A.currentZ,&cx,&cy,&rx,&ry);
            line_px(cx,cy-ry,cx-rx,cy,c);line_px(cx,cy-ry,cx+rx,cy,c);line_px(cx-rx,cy,cx,cy+ry,c);line_px(cx+rx,cy,cx,cy+ry,c);
        }
        if(f&SCENE_MACRO_UNDERLAY){
            int cx,cy,rx,ry;plan_iso_points((x0+x1)/2,(y0+y1)/2,A.currentZ,&cx,&cy,&rx,&ry);
            (void)rx;(void)ry;plan_draw_underlay_marker(cx,cy,clampi(A.zoom+1,2,5));
        }
    }
}
static void draw_plan(void){
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(dx<=0||dy<=0)return;int ox,oy,cs;plan_geometry(&ox,&oy,&cs);int z=A.currentZ;uint8_t*renderSem=plan_build_render_floor_layer(z);
    for(int y=0;y<dy;y++)for(int x=0;x<dx;x++){
        PlanCell*p=plan_cell_at(x,y,z);int sem=renderSem?renderSem[y*dx+x]:plan_floor_effective(x,y,z),obj=plan_object_effective(x,y,z),shape=plan_floor_shape_effective(x,y,z);int l=ox+x*cs,t=oy+y*cs;
        if(A.planDisplayMode==0||A.planDisplayMode==1){uint32_t fill=PLAN_COLORS[(sem>=0&&sem<PLAN_SEMANTIC_COUNT)?sem:0];plan_fill_square_shape(l+1,t+1,l+cs,t+cs,shape,fill);}
        else plan_fill_px(l+1,t+1,l+cs,t+cs,0xFF11191Eu);
        if(A.planDisplayMode==0||A.planDisplayMode==3){uint32_t oc=plan_object_visual_color(x,y,z,obj);if(oc){int m=cs/4;if(m<2)m=2;plan_fill_px(l+m,t+m,l+cs-m,t+cs-m,oc);line_px(l+m,t+m,l+cs-m,t+m,0xFF10171Bu);line_px(l+m,t+cs-m,l+cs-m,t+cs-m,0xFF10171Bu);line_px(l+m,t+m,l+m,t+cs-m,0xFF10171Bu);line_px(l+cs-m,t+m,l+cs-m,t+cs-m,0xFF10171Bu);}}
        if(A.showGrid){line_px(l,t,l+cs,t,0xFF334954u);line_px(l,t,l,t+cs,0xFF334954u);line_px(l+cs,t,l+cs,t+cs,0xFF334954u);line_px(l,t+cs,l+cs,t+cs,0xFF334954u);}
        if(A.planDisplayMode==0||A.planDisplayMode==2){
            int d=plan_diag_effective(x,y,z),e;
            if((e=plan_edge_effective(x,y,z,1)))plan_line_edge_context_dim(l,t,l,t+cs,e,100,d!=0);
            if((e=plan_edge_effective(x,y,z,2)))plan_line_edge_context_dim(l,t,l+cs,t,e,100,d!=0);
            if((e=plan_edge_effective(x,y,z,3)))plan_line_edge_context_dim(l+cs,t,l+cs,t+cs,e,100,d!=0);
            if((e=plan_edge_effective(x,y,z,4)))plan_line_edge_context_dim(l,t+cs,l+cs,t+cs,e,100,d!=0);
            if(d==2)plan_line_diag_dim(l,t+cs,l+cs,t,100);else if(d==3)plan_line_diag_dim(l,t,l+cs,t+cs,100);
        }
        if(p&&p->vlink)plan_draw_vlink_symbol(l+cs/2,t+cs/2,cs/4,p->vlink,0xFF42E4FFu);
        if(p&&(p->flags&PLAN_FLAG_LOCKED)){int r=clampi(cs/8,2,4);plan_draw_padlock_symbol(l+cs-r-2,t+r+2,r,0xFFFFD54Au);}
        if(plan_in_selection(x,y)){line_px_thick(l+1,t+1,l+cs-1,t+1,0xFF35D7FFu);line_px_thick(l+1,t+cs-1,l+cs-1,t+cs-1,0xFF35D7FFu);line_px_thick(l+1,t+1,l+1,t+cs-1,0xFF35D7FFu);line_px_thick(l+cs-1,t+1,l+cs-1,t+cs-1,0xFF35D7FFu);}
    }
    free(renderSem);plan_draw_scene_macro_2d(ox,oy,cs);
    if(A.planPasteMode&&A.hoverValid&&A.planClip&&A.planClipW>0&&A.planClipH>0){int x0=A.hoverX,y0=A.hoverY;for(int y=0;y<A.planClipH;y++)for(int x=0;x<A.planClipW;x++){int tx=x0+x,ty=y0+y;if(tx<0||ty<0||tx>=dx||ty>=dy)continue;int l=ox+tx*cs,t=oy+ty*cs;line_px(l+2,t+2,l+cs-2,t+2,0xFFB86BFFu);line_px(l+2,t+cs-2,l+cs-2,t+cs-2,0xFFB86BFFu);line_px(l+2,t+2,l+2,t+cs-2,0xFFB86BFFu);line_px(l+cs-2,t+2,l+cs-2,t+cs-2,0xFFB86BFFu);}}
}
static void draw_plan_iso(void){
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(dx<=0||dy<=0)return;int zmin=A.showAllBelow?0:A.currentZ;
    for(int z=zmin;z<=A.currentZ;z++){
        int depth=A.currentZ-z;int dim=depth==0?100:(depth==1?72:(depth==2?58:46));uint8_t*renderSem=plan_build_render_floor_layer(z);
        for(int y=0;y<dy;y++)for(int x=0;x<dx;x++){
            if(z!=A.currentZ&&!plan_cell_has_semantic_content(x,y,z))continue;PlanCell*p=plan_cell_at(x,y,z);int sem=renderSem?renderSem[y*dx+x]:plan_floor_effective(x,y,z),obj=plan_object_effective(x,y,z),shape=plan_floor_shape_effective(x,y,z);int cx,cy,rx,ry;plan_iso_points(x,y,z,&cx,&cy,&rx,&ry);uint32_t fc=plan_dim_color(PLAN_COLORS[(sem>=0&&sem<PLAN_SEMANTIC_COUNT)?sem:0],dim);
            if((A.planDisplayMode==0||A.planDisplayMode==1)&&sem!=PLAN_AUTO)plan_fill_iso_shape(cx,cy,rx,ry,shape,fc);else if(A.planDisplayMode==2||A.planDisplayMode==3)plan_fill_diamond(cx,cy,rx,ry,plan_dim_color(0xFF11191Eu,dim));
            if(A.planDisplayMode==0||A.planDisplayMode==3){uint32_t oc=plan_object_visual_color(x,y,z,obj);if(oc)plan_fill_diamond(cx,cy,(rx*5)/10,(ry*5)/10,plan_dim_color(oc,dim));}
            uint32_t gc=plan_dim_color(0xFF334954u,dim);if(A.showGrid){line_px(cx,cy-ry,cx-rx,cy,gc);line_px(cx,cy-ry,cx+rx,cy,gc);line_px(cx-rx,cy,cx,cy+ry,gc);line_px(cx+rx,cy,cx,cy+ry,gc);}
            if(A.planDisplayMode==0||A.planDisplayMode==2){
                int d=plan_diag_effective(x,y,z),e;
                if((e=plan_edge_effective(x,y,z,1)))plan_line_edge_context_dim(cx,cy-ry,cx-rx,cy,e,dim,d!=0);
                if((e=plan_edge_effective(x,y,z,2)))plan_line_edge_context_dim(cx,cy-ry,cx+rx,cy,e,dim,d!=0);
                if((e=plan_edge_effective(x,y,z,3)))plan_line_edge_context_dim(cx+rx,cy,cx,cy+ry,e,dim,d!=0);
                if((e=plan_edge_effective(x,y,z,4)))plan_line_edge_context_dim(cx-rx,cy,cx,cy+ry,e,dim,d!=0);
                if(d==2)plan_line_diag_dim(cx-rx,cy,cx+rx,cy,dim);else if(d==3)plan_line_diag_dim(cx,cy-ry,cx,cy+ry,dim);
            }
            if(p&&p->vlink)plan_draw_vlink_symbol(cx,cy,(ry*3)/4,p->vlink,plan_dim_color(0xFF42E4FFu,dim));
            if(z==A.currentZ&&p&&(p->flags&PLAN_FLAG_LOCKED)){int r=clampi(ry/4,2,4);plan_draw_padlock_symbol(cx+rx/3,cy-ry/3,r,0xFFFFD54Au);}
            if(z==A.currentZ&&plan_in_selection(x,y)){uint32_t sc=0xFF35D7FFu;line_px_thick(cx,cy-ry,cx-rx,cy,sc);line_px_thick(cx,cy-ry,cx+rx,cy,sc);line_px_thick(cx-rx,cy,cx,cy+ry,sc);line_px_thick(cx+rx,cy,cx,cy+ry,sc);}
        }
        free(renderSem);
    }
    plan_draw_scene_macro_iso();
    if(A.planPasteMode&&A.hoverValid&&A.planClip&&A.planClipW>0&&A.planClipH>0){int x0=A.hoverX,y0=A.hoverY;for(int y=0;y<A.planClipH;y++)for(int x=0;x<A.planClipW;x++){int tx=x0+x,ty=y0+y;if(tx<0||ty<0||tx>=dx||ty>=dy)continue;int cx,cy,rx,ry;plan_iso_points(tx,ty,A.currentZ,&cx,&cy,&rx,&ry);uint32_t c=0xFFB86BFFu;line_px(cx,cy-ry,cx-rx,cy,c);line_px(cx,cy-ry,cx+rx,cy,c);line_px(cx-rx,cy,cx,cy+ry,c);line_px(cx+rx,cy,cx,cy+ry,c);}}
}
#include "workshop_realhd.h"

static void draw_scene(void){
    if(!A.scene.active||!A.scene.cells)return;int ox,oy;world_origin(&ox,&oy);int zmin=normal_z_min(),zmax=normal_z_max();
    for(int z=zmin;z<=zmax;z++)for(int y=0;y<A.scene.y;y++)for(int x=0;x<A.scene.x;x++){
        editorRenderOpacity=editor_opacity_for_z(z);int sx,sy;project_tile(x,y,z,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;SceneCell*c=scene_cell_at(x,y,z);if(!c)continue;
        if(A.scene.geoLookup&&A.scene.geoLookup[(z*A.scene.y+y)*A.scene.x+x]>=0){scene_geo_draw(x,y,z,sx,sy);}
        for(int part=0;part<4;part++){if(part==0&&A.scene.geoLookup&&A.scene.geoLookup[(z*A.scene.y+y)*A.scene.x+x]>=0)continue;int lib=c->lib[part],local=c->local[part];if(lib<0||local<0)continue;if(rh_draw_part(x,y,z,part,lib,local,sx,sy))continue;int fr=mcd_frame(lib,local),p=mcd_plevel(lib,local);draw_sprite_scaled(lib,local,fr,sx,sy-p*A.zoom,A.zoom);}
    }
    editorRenderOpacity=100;
    if(A.showGrid){uint32_t gc=0xFF31566Au;for(int y=0;y<A.scene.y;y++)for(int x=0;x<A.scene.x;x++){int sx,sy;project_tile(x,y,A.currentZ,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;int hw=16*A.zoom;int topx=sx+hw,topy=sy+24*A.zoom;line_px(topx,topy,sx+32*A.zoom,sy+32*A.zoom,gc);line_px(sx+32*A.zoom,sy+32*A.zoom,topx,sy+40*A.zoom,gc);line_px(topx,sy+40*A.zoom,sx,sy+32*A.zoom,gc);line_px(sx,sy+32*A.zoom,topx,topy,gc);}}
    draw_scene_macro_reservations();
    if(A.showGrid&&A.scene.usoSlotActive){
        draw_scene_slot_rect(A.scene.usoSlotX,A.scene.usoSlotY,A.scene.usoSlotW,A.scene.usoSlotH,0xFFFFB52Eu);
        int ox2,oy2;world_origin(&ox2,&oy2);int ax,ay,bx,by;project_tile(A.scene.usoSlotX,A.scene.usoSlotY,A.currentZ,&ax,&ay);project_tile(A.scene.usoSlotX+A.scene.usoSlotW-1,A.scene.usoSlotY+A.scene.usoSlotH-1,A.currentZ,&bx,&by);ax=ox2+ax*A.zoom+16*A.zoom;ay=oy2+ay*A.zoom+32*A.zoom;bx=ox2+bx*A.zoom+16*A.zoom;by=oy2+by*A.zoom+32*A.zoom;line_px(ax,ay,bx,by,0xFFFFB52Eu);
    }
    if(A.showGrid&&A.scene.craftW>0&&A.scene.craftH>0)draw_scene_slot_rect(A.scene.craftX,A.scene.craftY,A.scene.craftW,A.scene.craftH,0xFF55D77Au);
    draw_placement_preview();
}
static void draw_map(void){
    if(A.viewMode==1){draw_plan();return;}
    if(A.viewMode==2){draw_plan_iso();return;}
    if(A.scene.active){draw_scene();return;}
    if(!A.map.cells)return;int ox,oy;world_origin(&ox,&oy);int zmin=normal_z_min(),zmax=normal_z_max();
    for(int z=zmin;z<=zmax;z++)for(int y=0;y<A.map.y;y++)for(int x=0;x<A.map.x;x++){
        editorRenderOpacity=editor_opacity_for_z(z);int sx,sy;project_tile(x,y,z,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;MapCell*c=cell_at(x,y,z);
        for(int part=0;part<4;part++){int raw=c->part[part],lib,local;if(!active_resolve_raw(raw,&lib,&local))continue;if(rh_draw_part(x,y,z,part,lib,local,sx,sy))continue;int fr=mcd_frame(lib,local),p=mcd_plevel(lib,local);draw_sprite_scaled(lib,local,fr,sx,sy-p*A.zoom,A.zoom);}
    }
    editorRenderOpacity=100;
    if(A.showGrid){uint32_t gc=0xFF31566Au;for(int y=0;y<A.map.y;y++)for(int x=0;x<A.map.x;x++){int sx,sy;project_tile(x,y,A.currentZ,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;int hw=16*A.zoom;int topx=sx+hw,topy=sy+24*A.zoom;line_px(topx,topy,sx+32*A.zoom,sy+32*A.zoom,gc);line_px(sx+32*A.zoom,sy+32*A.zoom,topx,sy+40*A.zoom,gc);line_px(topx,sy+40*A.zoom,sx,sy+32*A.zoom,gc);line_px(sx,sy+32*A.zoom,topx,topy,gc);}}
    rmp_draw_overlay();
    draw_placement_preview();
}


static void overview_putpx(int x,int y,uint32_t c){
    if(A.overviewBuf&&x>=0&&y>=0&&x<A.overviewW&&y<A.overviewH)A.overviewBuf[(size_t)y*A.overviewW+x]=c;
}
static void overview_sprite(int lib,int local,int frame,int dx,int dy){
    if(A.assetRenderMode!=0){HDCacheEntry*e=hd_load_entry(lib,local);if(e&&e->pixels){
        for(int y=0;y<TILE_H;y++){int sy=y*e->h/TILE_H;for(int x=0;x<TILE_W;x++){int sx=x*e->w/TILE_W;uint32_t sp=e->pixels[(size_t)sy*e->w+sx];unsigned a=sp>>24;if(!a)continue;int ox=dx+x,oy=dy+y;if(ox>=0&&oy>=0&&ox<A.overviewW&&oy<A.overviewH){size_t di=(size_t)oy*A.overviewW+ox;A.overviewBuf[di]=alpha_over(A.overviewBuf[di],sp);}}}return;
    }}
    if(!library_load(lib))return;LibrarySet*s=&A.library[lib];if(!s->sprites||frame<0||frame>=s->frameCount)return;
    uint8_t*sp=s->sprites+(size_t)frame*TILE_W*TILE_H;
    for(int y=0;y<TILE_H;y++)for(int x=0;x<TILE_W;x++){uint8_t p=sp[y*TILE_W+x];if(p)overview_putpx(dx+x,dy+y,pal_color(p));}
}
static void build_overview(void){
    if(!A.map.cells){A.overviewDirty=0;return;}
    int w=(A.map.x+A.map.y)*16+128;
    int h=(A.map.x+A.map.y)*8+(A.map.z-1)*24+176;
    if(w<256)w=256;if(h<192)h=192;if(w>4096)w=4096;if(h>3072)h=3072;
    if(!A.overviewBuf||A.overviewW!=w||A.overviewH!=h){free(A.overviewBuf);A.overviewBuf=(uint32_t*)malloc((size_t)w*h*4);A.overviewW=w;A.overviewH=h;}
    if(!A.overviewBuf){A.overviewW=A.overviewH=0;A.overviewDirty=0;return;}
    for(size_t i=0,n=(size_t)w*h;i<n;i++)A.overviewBuf[i]=RGB32(8,14,18);
    int ox=(A.map.y-1)*16+64;
    int oy=(A.map.z-1)*24+72;
    for(int z=0;z<A.map.z;z++)for(int y=0;y<A.map.y;y++)for(int x=0;x<A.map.x;x++){
        int sx=ox+(x-y)*16,sy=oy+(x+y)*8-z*24;MapCell*c=cell_at(x,y,z);
        for(int part=0;part<4;part++){int raw=c->part[part],lib,local;if(!active_resolve_raw(raw,&lib,&local))continue;int fr=mcd_frame(lib,local),p=mcd_plevel(lib,local);overview_sprite(lib,local,fr,sx,sy-p);}
    }
    A.overviewDirty=0;
}
static void draw_overview(HDC hdc,int x,int y,int w,int h){
    fill_rect_color(hdc,x-1,y-1,x+w+1,y+h+1,RGB(47,70,80));
    fill_rect_color(hdc,x,y,x+w,y+h,RGB(8,14,18));
    if(!A.map.cells)return;if(A.overviewDirty)build_overview();if(!A.overviewBuf||A.overviewW<=0||A.overviewH<=0)return;
    double sx=(double)w/A.overviewW,sy=(double)h/A.overviewH,sc=sx<sy?sx:sy;int dw=(int)(A.overviewW*sc),dh=(int)(A.overviewH*sc);if(dw<1||dh<1)return;
    int dx=x+(w-dw)/2,dy=y+(h-dh)/2;BITMAPINFO bi;ZeroMemory(&bi,sizeof(bi));bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=A.overviewW;bi.bmiHeader.biHeight=-A.overviewH;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
    StretchDIBits(hdc,dx,dy,dw,dh,0,0,A.overviewW,A.overviewH,A.overviewBuf,&bi,DIB_RGB_COLORS,SRCCOPY);
}
static int inspector_z_button_y(void){return plan_view_active()?812:252;}
static int inspector_z_rows(void){int dz=doc_zmax();if(dz<=0)return 0;int cols=dz<8?dz:8;return (dz+cols-1)/cols;}
static void draw_inspector_z_buttons(HDC hdc,int l){
    int dz=doc_zmax();if(dz<=0)return;int cols=dz<8?dz:8;if(cols<1)return;int avail=A.inspectorW-28,bw=avail/cols,y=inspector_z_button_y();
    for(int z=0;z<dz;z++){int row=z/cols,col=z%cols;int x=l+14+col*bw;wchar_t t[24];_snwprintf(t,23,L"Z%d",z);draw_button(hdc,x,y+row*28,x+bw-3,y+row*28+24,t,z==A.currentZ);}
}
static int inspector_level_click(int mx,int my){
    int dz=doc_zmax();if(dz<=0)return 0;int l=A.clientW-A.inspectorW;if(mx<l+14||mx>=A.clientW-14)return 0;int cols=dz<8?dz:8;if(cols<1)return 0;int avail=A.inspectorW-28,bw=avail/cols,y=inspector_z_button_y();int rows=inspector_z_rows();if(my<y||my>=y+rows*28)return 0;int col=(mx-(l+14))/bw,row=(my-y)/28,z=row*cols+col;if(z>=0&&z<dz){set_z(z);return 1;}return 0;
}


static const wchar_t* source_name(int s){return s==SRC_TFTD?L"TFTD ORIGINAL":(s==SRC_OXCE?L"OXCE STANDARD":(s==SRC_MOD?L"MOD OXCE":tr(L"MANUEL")));}
static int contains_ci(const wchar_t *hay,const wchar_t *needle){
    if(!needle||!*needle)return 1;
    size_t hn=wcslen(hay),nn=wcslen(needle); if(nn>hn)return 0;
    for(size_t i=0;i+nn<=hn;i++){size_t k=0;for(;k<nn;k++)if(towlower(hay[i+k])!=towlower(needle[k]))break;if(k==nn)return 1;}
    return 0;
}
static int mcd_matches_filter(int lib,int local){
    if(A.resourceFilter<0)return 1;
    return mcd_u8(lib,local,53)==A.resourceFilter;
}
static int visible_mcd_count(int lib){if(lib<0||lib>=A.libraryCount)return 0;int n=0;for(int k=0;k<A.library[lib].mcdCount;k++)if(mcd_matches_filter(lib,k))n++;return n;}
static int visible_mcd_at(int lib,int vis){if(lib<0||lib>=A.libraryCount)return -1;int n=0;for(int k=0;k<A.library[lib].mcdCount;k++)if(mcd_matches_filter(lib,k)){if(n==vis)return k;n++;}return -1;}
static int library_visible(int i){ return i>=0&&i<A.libraryCount&&contains_ci(A.library[i].name,A.searchFilter)&&visible_mcd_count(i)>0; }


static int map_visible(int i){return i>=0&&i<A.mapCount&&(!A.mapFilterDormant||A.maps[i].legacyDormant)&&contains_ci(A.maps[i].name,A.searchFilter);}
static const wchar_t* map_kind_name(const MapEntry*m){
    if(m&&m->legacyDormant)return L"LEGACY DORMANT";
    if(!m)return L"MAP";
    if(m->profileIndex>=0&&m->profileIndex<A.profileCount){int k=A.profiles[m->profileIndex].kind;if(k==1)return L"CRAFT";if(k==2)return L"USO";if(k==0)return L"TERRAIN";}
    if(_wcsnicmp(m->name,L"UFO",3)==0)return L"USO";return L"MAP";
}
static int panel_source(int panel){return panel==0?SRC_TFTD:(panel==1?SRC_OXCE:SRC_MOD);}
static void ensure_panel_splits(void){
    int top=TREE_TOP,bottom=A.clientH-TREE_BOTTOM,avail=bottom-top;if(avail<180)return;
    if(A.panelSplit1==0||A.panelSplit2==0){A.panelSplit1=top+avail/3;A.panelSplit2=top+(avail*2)/3;}
    A.panelSplit1=clampi(A.panelSplit1,top+55,bottom-110);A.panelSplit2=clampi(A.panelSplit2,A.panelSplit1+55,bottom-55);
}
static void panel_bounds(int panel,int*top,int*bottom){ensure_panel_splits();int t=TREE_TOP,b=A.clientH-TREE_BOTTOM;if(panel==0){*top=t;*bottom=A.panelSplit1-3;}else if(panel==1){*top=A.panelSplit1+3;*bottom=A.panelSplit2-3;}else{*top=A.panelSplit2+3;*bottom=b;}}
static int map_panel_content_height(int panel){int source=panel_source(panel),h=0;wchar_t last[96]=L"";for(int i=0;i<A.mapCount;i++){MapEntry*m=&A.maps[i];if(m->source!=source||!map_visible(i))continue;if(source==SRC_MOD&&_wcsicmp(last,m->origin)!=0){h+=24;wcsncpy(last,m->origin,95);last[95]=0;}h+=27;}return h;}
static void draw_map_panel(HDC hdc,int panel){
    int top,bottom;panel_bounds(panel,&top,&bottom);if(bottom<=top)return;int source=panel_source(panel);const wchar_t*title=source==SRC_TFTD?L"TFTD ORIGINAL":(source==SRC_OXCE?L"OXCE STANDARD":L"MODS OXCE");
    COLORREF tc=source==SRC_TFTD?RGB(105,215,235):(source==SRC_OXCE?RGB(170,165,245):RGB(235,175,95));
    fill_rect_color(hdc,5,top,A.sidebarW-5,bottom,RGB(12,20,25));fill_rect_color(hdc,6,top+1,A.sidebarW-6,top+28,RGB(26,48,58));SetTextColor(hdc,tc);TextOutW(hdc,14,top+7,title,(int)wcslen(title));
    wchar_t count[48];int n=0;for(int i=0;i<A.mapCount;i++)if(A.maps[i].source==source&&map_visible(i))n++;_snwprintf(count,47,tr(L"%d MAP"),n);SetTextColor(hdc,RGB(145,165,175));TextOutW(hdc,A.sidebarW-76,top+7,count,(int)wcslen(count));
    int bodyTop=top+30,bodyBottom=bottom-2;int panelSaved=SaveDC(hdc);IntersectClipRect(hdc,6,bodyTop,A.sidebarW-6,bodyBottom);int y=bodyTop-A.sourceScroll[panel];wchar_t last[96]=L"";
    for(int i=0;i<A.mapCount;i++){MapEntry*m=&A.maps[i];if(m->source!=source||!map_visible(i))continue;
        if(source==SRC_MOD&&_wcsicmp(last,m->origin)!=0){if(y+24>=bodyTop&&y<bodyBottom){fill_rect_color(hdc,8,y,A.sidebarW-8,y+22,RGB(42,35,26));SetTextColor(hdc,RGB(235,190,115));wchar_t modline[128];_snwprintf(modline,127,tr(L"MOD : %ls"),m->origin[0]?m->origin:tr(L"(sans nom)"));TextOutW(hdc,14,y+4,modline,(int)wcslen(modline));}y+=24;wcsncpy(last,m->origin,95);last[95]=0;}
        if(y+27>=bodyTop&&y<bodyBottom){int sel=i==A.selectedMap;fill_rect_color(hdc,8,y,A.sidebarW-8,y+25,sel?RGB(45,83,96):RGB(24,33,40));SetTextColor(hdc,RGB(210,226,232));wchar_t line[220];_snwprintf(line,219,tr(L"%ls   %dx%dx%d   [%ls]"),m->name,m->x,m->y,m->z,map_kind_name(m));TextOutW(hdc,16,y+4,line,(int)wcslen(line));}y+=27;
    }
    RestoreDC(hdc,panelSaved);int content=map_panel_content_height(panel),visible=bodyBottom-bodyTop,max=content-visible;if(max<0)max=0;A.sourceScroll[panel]=clampi(A.sourceScroll[panel],0,max);
}
static void draw_tree_maps(HDC hdc,int top,int bottom){(void)top;(void)bottom;ensure_panel_splits();draw_map_panel(hdc,0);draw_map_panel(hdc,1);draw_map_panel(hdc,2);fill_rect_color(hdc,5,A.panelSplit1-3,A.sidebarW-5,A.panelSplit1+3,RGB(72,96,104));fill_rect_color(hdc,5,A.panelSplit2-3,A.sidebarW-5,A.panelSplit2+3,RGB(72,96,104));}
static void draw_thumb_hdc_scaled(HDC hdc,int lib,int local,int frame,int dx,int dy,int dw,int dh,uint32_t bg){
    for(int i=0;i<TILE_W*TILE_H;i++)gThumb[i]=bg;
    int usedHD=0;
    if(A.assetRenderMode!=0){HDCacheEntry*e=hd_load_entry(lib,local);if(e&&e->pixels){usedHD=1;
        for(int y=0;y<TILE_H;y++){int sy=y*e->h/TILE_H;for(int x=0;x<TILE_W;x++){int sx=x*e->w/TILE_W;uint32_t sp=e->pixels[(size_t)sy*e->w+sx];if(sp>>24)gThumb[y*TILE_W+x]=alpha_over(gThumb[y*TILE_W+x],sp);}}
    }}
    if(!usedHD&&library_load(lib)){LibrarySet*s=&A.library[lib];if(s->sprites&&frame>=0&&frame<s->frameCount){uint8_t*sp=s->sprites+(size_t)frame*TILE_W*TILE_H;for(int i=0;i<TILE_W*TILE_H;i++)if(sp[i])gThumb[i]=pal_color(sp[i]);}}
    BITMAPINFO bi;ZeroMemory(&bi,sizeof(bi));bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=TILE_W;bi.bmiHeader.biHeight=-TILE_H;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
    StretchDIBits(hdc,dx,dy,dw,dh,0,0,TILE_W,TILE_H,gThumb,&bi,DIB_RGB_COLORS,SRCCOPY);
}
static void fill_rect_color(HDC hdc,int l,int t,int r,int b,COLORREF c){RECT q={l,t,r,b};HBRUSH br=CreateSolidBrush(c);FillRect(hdc,&q,br);DeleteObject(br);}
static void draw_button(HDC hdc,int l,int t,int r,int b,const wchar_t*txt,int active){fill_rect_color(hdc,l,t,r,b,active?RGB(38,92,108):RGB(31,43,52));SetTextColor(hdc,active?RGB(230,250,255):RGB(205,220,226));RECT textRect={l+8,t+2,r-5,b-2};DrawTextW(hdc,txt,-1,&textRect,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);}

static int grid_rows_for(int count){return (count+GRID_COLS-1)/GRID_COLS;}
static int tree_content_height_library(void){
    int h=0;for(int i=0;i<A.libraryCount;i++){if(!library_visible(i))continue;h+=30;if(i==A.expandedLib)h+=grid_rows_for(visible_mcd_count(i))*GRID_CELL_H;}return h;
}
static int tree_content_height_active(void){
    int h=0;for(int i=0;i<A.activeCount;i++){int lib=A.active[i].libIndex;if(!library_visible(lib))continue;h+=32;if(i==A.expandedActive)h+=grid_rows_for(visible_mcd_count(lib))*GRID_CELL_H;}return h;
}
static void draw_grid_entries(HDC hdc,int lib,int base,int y0,int top,int bottom,int showRaw){
    if(!library_load(lib))return;
    int avail=A.sidebarW-20, cw=avail/GRID_COLS;int vc=visible_mcd_count(lib);
    for(int vi=0;vi<vc;vi++){
        int k=visible_mcd_at(lib,vi);if(k<0)continue;
        int row=vi/GRID_COLS,col=vi%GRID_COLS;int x=10+col*cw,y=y0+row*GRID_CELL_H;
        if(y+GRID_CELL_H<top||y>bottom)continue;
        int sel=A.selectedLib==lib&&A.selectedLocal==k;
        fill_rect_color(hdc,x+2,y+2,x+cw-2,y+GRID_CELL_H-3,sel?RGB(45,83,96):RGB(27,37,44));
        int fr=mcd_frame(lib,k);
        draw_thumb_hdc_scaled(hdc,lib,k,fr,x+(cw-40)/2,y+5,40,50,sel?0xFF604F30u:0xFF30271Fu);
        wchar_t label[64];SetTextColor(hdc,sel?RGB(235,250,255):RGB(190,210,217));
        if(showRaw)_snwprintf(label,63,L"%03d/%03d",base+k,k);else _snwprintf(label,63,tr(L"MCD %03d"),k);
        int tx=x+5;TextOutW(hdc,tx,y+58,label,(int)wcslen(label));
    }
}
static void draw_tree_library(HDC hdc,int top,int bottom){
    int y=top-A.treeScroll;
    for(int i=0;i<A.libraryCount;i++){
        if(!library_visible(i))continue;LibrarySet*s=&A.library[i];
        if(y+30>=top&&y<bottom){
            int open=(i==A.expandedLib);fill_rect_color(hdc,6,y,A.sidebarW-6,y+28,open?RGB(36,59,70):RGB(25,35,43));
            SetTextColor(hdc,s->source==0?RGB(100,210,235):RGB(170,150,245));
            wchar_t line[300];if(s->source==SRC_MOD)_snwprintf(line,299,tr(L"%ls  [MOD: %ls]  (%d)"),s->name,s->origin,s->mcdCount);else _snwprintf(line,299,tr(L"%ls  [%ls]  (%d)"),s->name,source_name(s->source),s->mcdCount);TextOutW(hdc,14,y+6,line,(int)wcslen(line));
            SetTextColor(hdc,RGB(110,220,235));TextOutW(hdc,A.sidebarW-58,y+6,L"+MAP",4);
        }
        y+=30;
        if(i==A.expandedLib){int gh=grid_rows_for(visible_mcd_count(i))*GRID_CELL_H;draw_grid_entries(hdc,i,0,y,top,bottom,0);y+=gh;}
    }
}
static void draw_tree_active(HDC hdc,int top,int bottom,int showRaw){
    int y=top-A.treeScroll;
    for(int i=0;i<A.activeCount;i++){
        int lib=A.active[i].libIndex;if(!library_visible(lib))continue;LibrarySet*s=&A.library[lib];int base=A.active[i].baseIndex;
        if(y+32>=top&&y<bottom){
            int open=(i==A.expandedActive);fill_rect_color(hdc,6,y,A.sidebarW-6,y+30,open?RGB(50,63,45):RGB(31,40,34));
            SetTextColor(hdc,RGB(205,235,180));wchar_t line[220];
            if(showRaw){if(s->source==SRC_MOD)_snwprintf(line,219,tr(L"%03d-%03d  %ls [MOD:%ls]"),base,base+s->mcdCount-1,s->name,s->origin);else _snwprintf(line,219,tr(L"%03d-%03d  %ls"),base,base+s->mcdCount-1,s->name);}else{if(s->source==SRC_MOD)_snwprintf(line,219,tr(L"%ls [MOD:%ls] (%d)"),s->name,s->origin,s->mcdCount);else _snwprintf(line,219,tr(L"%ls  (%d)"),s->name,s->mcdCount);}
            TextOutW(hdc,14,y+7,line,(int)wcslen(line));
        }
        y+=32;
        if(i==A.expandedActive){int gh=grid_rows_for(visible_mcd_count(lib))*GRID_CELL_H;draw_grid_entries(hdc,lib,base,y,top,bottom,showRaw);y+=gh;}
    }
}
static void draw_sidebar(HDC hdc){
    fill_rect_color(hdc,0,0,A.sidebarW,A.clientH,RGB(17,24,30));SetBkMode(hdc,TRANSPARENT);
    SetTextColor(hdc,RGB(230,240,243));TextOutW(hdc,12,8,L"TFTD WORKSHOP V2.12.7",(int)wcslen(L"TFTD WORKSHOP V2.12.7"));
    SetTextColor(hdc,RGB(95,215,235));TextOutW(hdc,12,27,tr(L"Benjamin et GPT-6 edition"),(int)wcslen(tr(L"Benjamin et GPT-6 edition")));
    wchar_t sum[260];SetTextColor(hdc,RGB(145,165,175));_snwprintf(sum,259,tr(L"Datasets : TFTD %d | OXCE %d | MODS %d   |   MAP %d"),A.libraryCountTFTD,A.libraryCountOXCE,A.libraryCountMods,A.mapCount);TextOutW(hdc,12,54,sum,(int)wcslen(sum));
    SetTextColor(hdc,RGB(105,125,135));{
        const wchar_t*rm=render_mode_label(A.assetRenderMode);
        TextOutW(hdc,12,75,rm,(int)wcslen(rm));
    }
    int tabw=(A.sidebarW-16)/3;draw_button(hdc,8,102,8+tabw-2,130,tr(L"Cartes"),A.browserMode==0);draw_button(hdc,8+tabw,102,8+2*tabw-2,130,tr(L"Bibliotheque"),A.browserMode==1);draw_button(hdc,8+2*tabw,102,A.sidebarW-8,130,tr(L"Datasets MAP"),A.browserMode==2);
    SetTextColor(hdc,RGB(145,165,175));TextOutW(hdc,10,142,tr(L"Recherche :"),10);
    if(A.browserMode==0){SetTextColor(hdc,RGB(110,135,145));TextOutW(hdc,10,171,tr(L"3 sources separees - tirez les barres pour redimensionner"),52);}else{
        const wchar_t*labs[5]={tr(L"TOUS"),tr(L"SOL"),tr(L"OUEST"),tr(L"NORD"),tr(L"OBJET")};int bw=(A.sidebarW-16)/5;for(int i=0;i<5;i++){int f=i-1;draw_button(hdc,8+i*bw,168,8+(i+1)*bw-2,190,labs[i],A.resourceFilter==f);}
    }
    int bottom=A.clientH-TREE_BOTTOM;
    if(A.browserMode==0)draw_tree_maps(hdc,TREE_TOP,bottom);else{int saved=SaveDC(hdc);IntersectClipRect(hdc,0,TREE_TOP,A.sidebarW,bottom);if(A.browserMode==1)draw_tree_library(hdc,TREE_TOP,bottom);else draw_tree_active(hdc,TREE_TOP,bottom,1);RestoreDC(hdc,saved);}
    A.treeContentH=A.browserMode==0?0:(A.browserMode==1?tree_content_height_library():tree_content_height_active());if(A.browserMode!=0){int visible=bottom-TREE_TOP,maxScroll=A.treeContentH-visible;if(maxScroll<0)maxScroll=0;if(A.treeScroll>maxScroll)A.treeScroll=maxScroll;}
    fill_rect_color(hdc,0,bottom,A.sidebarW,A.clientH,RGB(14,20,25));SetTextColor(hdc,RGB(135,155,165));
    TextOutW(hdc,10,bottom+5,tr(L"Molette : Z | Ctrl+molette : zoom"),33);
    TextOutW(hdc,10,bottom+22,tr(L"Clic droit / Echap : lacher la piece"),34);
    TextOutW(hdc,10,bottom+39,tr(L"Tirez les separateurs des panneaux"),33);
    fill_rect_color(hdc,A.sidebarW-3,0,A.sidebarW,A.clientH,RGB(70,98,108));
}

static void raw_desc(int raw,wchar_t*out,int cap){
    if(raw<=0){wcsncpy(out,tr(L"vide"),cap-1);out[cap-1]=0;return;}
    int lib,local;if(!active_resolve_raw(raw,&lib,&local)){_snwprintf(out,cap-1,L"%d (non resolu)",raw);out[cap-1]=0;return;}
    _snwprintf(out,cap-1,L"%d = %ls/MCD %d",raw,A.library[lib].name,local);out[cap-1]=0;
}
static void draw_inspector(HDC hdc){
    int l=A.clientW-A.inspectorW;if(l<A.sidebarW+200)return;if(plan_view_active()){draw_plan_panel(hdc,l);return;}
    fill_rect_color(hdc,l,0,A.clientW,A.clientH,RGB(16,22,27));SetBkMode(hdc,TRANSPARENT);
    SetTextColor(hdc,RGB(105,215,230));TextOutW(hdc,l+14,12,tr(L"INSPECTEUR"),10);
    wchar_t line[300];int y=40;
    if(A.scene.active){
        SetTextColor(hdc,RGB(225,235,238));TextOutW(hdc,l+14,y,tr(L"SCENE COMPOSITEUR"),18);y+=24;SetTextColor(hdc,RGB(110,220,235));TextOutW(hdc,l+14,y,A.scene.title,(int)wcslen(A.scene.title));y+=24;
        _snwprintf(line,299,tr(L"Taille : %dx%dx%d"),A.scene.x,A.scene.y,A.scene.z);SetTextColor(hdc,RGB(170,190,198));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=20;
        if(A.scene.craftW>0){int res=0,under=0;for(int i=0;i<A.scene.macroW*A.scene.macroH;i++)if(A.scene.macroFlags[i]&SCENE_MACRO_CRAFT){res++;if(A.scene.macroFlags[i]&SCENE_MACRO_UNDERLAY)under++;}SetTextColor(hdc,RGB(85,215,122));_snwprintf(line,299,tr(L"Reservation X-COM : X%d Y%d %dx%d"),A.scene.craftX,A.scene.craftY,A.scene.craftW,A.scene.craftH);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;_snwprintf(line,299,tr(L"Macro-cases : %d | sol sous reservation : %d/%d"),res,under,res);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;}
        if(A.scene.usoSlotActive){int res=0,under=0;for(int i=0;i<A.scene.macroW*A.scene.macroH;i++)if(A.scene.macroFlags[i]&SCENE_MACRO_USO){res++;if(A.scene.macroFlags[i]&SCENE_MACRO_UNDERLAY)under++;}_snwprintf(line,299,tr(L"Reservation USO : %ls"),A.scene.usoSlotName);SetTextColor(hdc,RGB(245,190,90));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;_snwprintf(line,299,tr(L"X%d Y%d %dx%d | sous-sol %d/%d"),A.scene.usoSlotX,A.scene.usoSlotY,A.scene.usoSlotW,A.scene.usoSlotH,under,res);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=22;}
        SetTextColor(hdc,RGB(225,235,238));TextOutW(hdc,l+14,220,tr(L"NIVEAUX"),7);draw_inspector_z_buttons(hdc,l);
        y=inspector_z_button_y()+inspector_z_rows()*28+16;SetTextColor(hdc,RGB(150,175,184));TextOutW(hdc,l+14,y,normal_visibility_label(),(int)wcslen(normal_visibility_label()));y+=22;
        if(A.hoverValid){
            _snwprintf(line,299,tr(L"Case X%d Y%d Z%d"),A.hoverX,A.hoverY,A.currentZ);SetTextColor(hdc,RGB(110,220,235));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=20;
            SceneCell*c=scene_cell_at(A.hoverX,A.hoverY,A.currentZ);const wchar_t*pn[4]={tr(L"SOL"),tr(L"OUEST"),tr(L"NORD"),tr(L"OBJET")};
            for(int p=0;p<4;p++){if(!c||c->lib[p]<0){_snwprintf(line,299,tr(L"%ls : vide"),pn[p]);}else{_snwprintf(line,299,tr(L"%ls : %ls/MCD %d"),pn[p],A.library[c->lib[p]].name,c->local[p]);}SetTextColor(hdc,p==A.selectedLayer?RGB(235,245,190):RGB(165,185,192));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;}
        }
        if(A.scene.dirty){SetTextColor(hdc,RGB(245,190,90));TextOutW(hdc,l+14,A.clientH-42,tr(L"Scene modifiee - pensez a enregistrer le projet."),45);}
        return;
    }
    if(A.autoProfileIndex>=0&&A.autoProfileIndex<A.profileCount){MapProfile*q=&A.profiles[A.autoProfileIndex];int mi=A.map.path[0]?map_index_by_path(A.map.path):-1;SetTextColor(hdc,RGB(220,235,238));if(mi>=0&&A.maps[mi].profileInferred){const LegacyDormantInfo*di=legacy_dormant_info(A.maps[mi].name);_snwprintf(line,299,tr(L"Profil familial : %ls (via %ls)"),di?di->family:tr(L"?"),q->mapName);}else _snwprintf(line,299,tr(L"Profil MAP : %ls"),q->mapName);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=20;SetTextColor(hdc,RGB(145,165,175));if(q->source==SRC_MOD)_snwprintf(line,299,tr(L"Profil source : MOD / %ls"),q->origin);else _snwprintf(line,299,tr(L"Profil source : %ls"),source_name(q->source));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;_snwprintf(line,299,L"%d datasets automatiques",q->dataSetCount);TextOutW(hdc,l+14,y,line,(int)wcslen(line));}
    else{SetTextColor(hdc,RGB(150,165,172));TextOutW(hdc,l+14,y,tr(L"Profil MAP : non identifie"),25);}

    SetTextColor(hdc,RGB(225,235,238));TextOutW(hdc,l+14,100,tr(L"VUE GLOBALE MULTI-Z"),20);
    draw_overview(hdc,l+14,120,A.inspectorW-28,122);
    draw_inspector_z_buttons(hdc,l);
    y=inspector_z_button_y()+inspector_z_rows()*28+10;
    fill_rect_color(hdc,l+10,y,l+A.inspectorW-10,y+1,RGB(45,61,68));y+=12;
    if(A.map.cells){int cms=current_map_source();_snwprintf(line,299,tr(L"Source : %ls%ls"),source_name(cms),(cms==SRC_TFTD||cms==SRC_OXCE)?tr(L" [LECTURE SEULE]"):(cms==SRC_MOD?tr(L" [EDITABLE + BACKUP]"):tr(L" [NOUVEAU]")));SetTextColor(hdc,(cms==SRC_TFTD||cms==SRC_OXCE)?RGB(245,165,105):RGB(120,220,170));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=20;}
    if(A.map.cells&&A.map.path[0]){int mi=map_index_by_path(A.map.path);if(mi>=0&&A.maps[mi].legacyDormant){const LegacyDormantInfo*di=legacy_dormant_info(A.maps[mi].name);SetTextColor(hdc,RGB(255,200,90));TextOutW(hdc,l+14,y,L"LEGACY DORMANT / OXCE UNREACHABLE",33);y+=18;if(di){SetTextColor(hdc,RGB(175,195,205));_snwprintf(line,299,tr(L"Famille : %ls | %ls"),di->family,di->status);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;}if(A.maps[mi].profileInferred){SetTextColor(hdc,RGB(130,205,255));TextOutW(hdc,l+14,y,tr(L"Profil familial OXCE infere"),27);y+=18;}SetTextColor(hdc,RGB(175,195,205));if(A.maps[mi].rmpPresent&&A.maps[mi].rmpNodeCount>=0)_snwprintf(line,299,tr(L"RMP Legacy : %d nodes"),A.maps[mi].rmpNodeCount);else if(A.maps[mi].rmpPresent)_snwprintf(line,299,tr(L"RMP Legacy : present / format a verifier"));else _snwprintf(line,299,tr(L"RMP Legacy : absent"));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=20;}}
    if(A.rmp.show){
        SetTextColor(hdc,RGB(130,205,255));_snwprintf(line,299,tr(L"RMP : %d nodes | %ls"),A.rmp.count,A.rmp.loaded?A.rmp.sourceLabel:tr(L"aucun fichier"));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;
        if(A.rmp.proposalCount>0){SetTextColor(hdc,RGB(80,220,255));_snwprintf(line,299,tr(L"Propositions auto : %d"),A.rmp.proposalCount);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;}
        if(A.rmp.editMode){SetTextColor(hdc,RGB(255,190,90));TextOutW(hdc,l+14,y,tr(L"MODE EDITION RMP ACTIF"),22);y+=18;}
        if(A.rmp.selected>=0&&A.rmp.selected<A.rmp.count){RmpNode*n=&A.rmp.nodes[A.rmp.selected];SetTextColor(hdc,RGB(255,235,100));_snwprintf(line,299,tr(L"Node #%d  X%d Y%d Z%d"),A.rmp.selected,n->x,n->y,n->z);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;SetTextColor(hdc,RGB(175,195,205));_snwprintf(line,299,tr(L"Type %ls (%d) | Rang %ls (%d)"),rmp_type_name(n->raw[19]),n->raw[19],rmp_rank_name(n->raw[20]),n->raw[20]);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;_snwprintf(line,299,tr(L"Patrouille %d | Spawn %d | Cible %ls | res %d"),n->raw[21],n->raw[23],n->raw[22]==5?tr(L"OUI"):tr(L"NON"),n->raw[22]);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;}
        fill_rect_color(hdc,l+10,y,l+A.inspectorW-10,y+1,RGB(45,61,68));y+=10;
    }
    SetTextColor(hdc,RGB(225,235,238));TextOutW(hdc,l+14,y,tr(L"Tuile active"),12);y+=22;
    if(A.selectedLib>=0&&A.selectedLib<A.libraryCount){
        LibrarySet*s=&A.library[A.selectedLib];int k=A.selectedLocal;int raw=selected_raw_index(0);
        SetTextColor(hdc,RGB(110,220,235));_snwprintf(line,299,tr(L"%ls / MCD %d"),s->name,k);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=20;
        SetTextColor(hdc,RGB(170,190,198));_snwprintf(line,299,tr(L"Frame %d   P %d   T %d"),mcd_frame(A.selectedLib,k),mcd_plevel(A.selectedLib,k),mcd_tlevel(A.selectedLib,k));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;
        _snwprintf(line,299,tr(L"Indice MAP : %ls"),raw>=0?tr(L"actif"):tr(L"hors palette"));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;
        _snwprintf(line,299,tr(L"TU marche/vol/glisse : %d / %d / %d"),mcd_u8(A.selectedLib,k,39),mcd_u8(A.selectedLib,k,41),mcd_u8(A.selectedLib,k,40));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;
        _snwprintf(line,299,tr(L"Armure %d  Die %d  Alt %d"),mcd_u8(A.selectedLib,k,42),mcd_u8(A.selectedLib,k,44),mcd_u8(A.selectedLib,k,46));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;
        _snwprintf(line,299,tr(L"Porte %d  UFO %d  NoFloor %d  BigWall %d"),mcd_u8(A.selectedLib,k,35),mcd_u8(A.selectedLib,k,30),mcd_u8(A.selectedLib,k,32),mcd_bigwall_effective(A.selectedLib,k));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=20;
        int bpm[32],bpn=blueprint_find_matches_for(A.selectedLib,k,bpm,32);if(bpn>0){const BlueprintDef*bp=&BLUEPRINTS_RC15[A.blueprintIndex>=0?A.blueprintIndex:bpm[0]];SetTextColor(hdc,RGB(95,220,205));_snwprintf(line,299,tr(L"Blueprint : %d assemblage(s) | %d pieces | %dx%dx%d"),bpn,bp->partCount,bp->spanX,bp->spanY,bp->spanZ);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;SetTextColor(hdc,RGB(135,170,178));_snwprintf(line,299,tr(L"Observe %d fois | mode %ls"),bp->support,A.blueprintMode?tr(L"ASSEMBLAGE"):tr(L"PIECE SEULE"));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=22;}else y+=4;
    }else{SetTextColor(hdc,RGB(135,150,158));TextOutW(hdc,l+14,y,tr(L"Aucune tuile selectionnee."),25);y+=28;}

    fill_rect_color(hdc,l+10,y,l+A.inspectorW-10,y+1,RGB(45,61,68));y+=12;
    SetTextColor(hdc,RGB(225,235,238));TextOutW(hdc,l+14,y,tr(L"Case sous le curseur"),20);y+=22;
    if(A.hoverValid&&A.map.cells){
        _snwprintf(line,299,tr(L"X %d   Y %d   Z %d"),A.hoverX,A.hoverY,A.currentZ);SetTextColor(hdc,RGB(110,220,235));TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=22;
        MapCell*c=cell_at(A.hoverX,A.hoverY,A.currentZ);const wchar_t*pn[4]={tr(L"Sol"),tr(L"Mur ouest"),tr(L"Mur nord"),tr(L"Objet")};
        for(int p=0;p<4;p++){wchar_t d[220];raw_desc(c?c->part[p]:0,d,220);SetTextColor(hdc,p==A.selectedLayer?RGB(235,245,190):RGB(165,185,192));_snwprintf(line,299,tr(L"%ls : %ls"),pn[p],d);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=19;}
        if(A.showAllBelow&&A.currentZ>0){y+=4;SetTextColor(hdc,RGB(125,175,188));TextOutW(hdc,l+14,y,tr(L"Sous la case (niveau inferieur) :"),31);y+=18;MapCell*b=cell_at(A.hoverX,A.hoverY,A.currentZ-1);for(int p=0;p<4;p++){if(!b||!b->part[p])continue;wchar_t d[220];raw_desc(b->part[p],d,220);_snwprintf(line,299,tr(L"Z%d %ls : %ls"),A.currentZ-1,pn[p],d);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=17;}}
    }else{SetTextColor(hdc,RGB(135,150,158));TextOutW(hdc,l+14,y,tr(L"Survolez une case de la MAP."),27);y+=28;}

    y+=8;fill_rect_color(hdc,l+10,y,l+A.inspectorW-10,y+1,RGB(45,61,68));y+=12;
    SetTextColor(hdc,RGB(145,165,175));_snwprintf(line,299,tr(L"Datasets MAP : %d datasets / %d indices cumules (1..255 adressables)"),A.activeCount,A.activeTotalMcd);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;
    _snwprintf(line,299,tr(L"Zoom x%d   Niveau Z %d/%d"),A.zoom,A.currentZ,A.map.cells?A.map.z-1:0);TextOutW(hdc,l+14,y,line,(int)wcslen(line));y+=18;
    SetTextColor(hdc,RGB(110,190,205));TextOutW(hdc,l+14,y,normal_visibility_label(),(int)wcslen(normal_visibility_label()));
}
static void draw_level_bar(HDC hdc){
    int dz=doc_zmax();if(dz<=0)return;int x=A.sidebarW+12,y=38;
    draw_button(hdc,x,y,x+34,y+28,L"Z-",0);x+=40;
    wchar_t ztxt[96];_snwprintf(ztxt,95,tr(L"Z %d / %d"),A.currentZ,dz-1);fill_rect_color(hdc,x,y,x+84,y+28,RGB(23,37,46));SetBkMode(hdc,TRANSPARENT);SetTextColor(hdc,RGB(225,240,244));TextOutW(hdc,x+10,y+6,ztxt,(int)wcslen(ztxt));x+=90;
    draw_button(hdc,x,y,x+34,y+28,L"Z+",0);x+=42;
    draw_button(hdc,x,y,x+148,y+28,A.viewMode==0?normal_visibility_label():(A.showAllBelow?tr(L"Z actif + dessous >"):tr(L"Z actif seul >")),A.viewMode==0?A.showComplete:A.showAllBelow);x+=154;
    draw_button(hdc,x,y,x+64,y+28,A.showGrid?tr(L"Grille +"):tr(L"Grille -"),A.showGrid);
}
static int level_bar_click(int mx,int my){
    if(doc_zmax()<=0||my<38||my>=66||mx<A.sidebarW+12)return 0;int x=A.sidebarW+12;
    if(mx>=x&&mx<x+34){set_z(A.currentZ-1);return 1;}x+=40+84+6;
    if(mx>=x&&mx<x+34){set_z(A.currentZ+1);return 1;}x+=42;
    if(mx>=x&&mx<x+148){
        if(A.viewMode==0){HMENU menu=CreatePopupMenu();
            AppendMenuW(menu,MF_STRING|(!A.showComplete&&!A.showAllBelow?MF_CHECKED:0),IDM_Z_SINGLE,tr(L"Niveau actif seul"));
            AppendMenuW(menu,MF_STRING|(!A.showComplete&&A.showAllBelow?MF_CHECKED:0),IDM_Z_BELOW,tr(L"Niveau actif + niveaux inferieurs"));
            AppendMenuW(menu,MF_STRING|(A.showComplete?MF_CHECKED:0),IDM_Z_COMPLETE,tr(L"Vue complete : tous les niveaux Z"));
            POINT pt={x,66};ClientToScreen(A.hwnd,&pt);int id=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_LEFTALIGN,pt.x,pt.y,0,A.hwnd,NULL);DestroyMenu(menu);
            if(id)set_normal_visibility(id==IDM_Z_COMPLETE?2:(id==IDM_Z_BELOW?1:0));
        }else{A.showAllBelow=!A.showAllBelow;InvalidateRect(A.hwnd,NULL,FALSE);}return 1;}x+=154;
    if(mx>=x&&mx<x+64){A.showGrid=!A.showGrid;CheckMenuItem(GetMenu(A.hwnd),IDM_GRID_TOGGLE,MF_BYCOMMAND|(A.showGrid?MF_CHECKED:MF_UNCHECKED));set_status(A.showGrid?tr(L"Grille du niveau Z affichee."):tr(L"Grille du niveau Z masquee : mode capture propre."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}return 0;
}

static const wchar_t* rmp_rank_name(int rank){
    static const wchar_t* names[10]={L"Scout",L"XCOM",L"Soldier",L"Navigator",L"Leader",L"Engineer",L"Misc1",L"Medic",L"Misc2",L"Legacy9"};
    return (rank>=0&&rank<=9)?tr(names[rank]):L"RAW";
}
static const wchar_t* rmp_type_name(int type){
    switch(type&7){case 1:return tr(L"VOL");case 2:return L"1x1";case 3:return L"1x1+VOL";case 4:return tr(L"DANGER");case 5:return tr(L"VOL+D");case 6:return L"1x1+D";case 7:return L"1x1+VOL+D";default:return tr(L"TOUS");}
}
static const wchar_t* rmp_tool_desc(void){
    switch(A.rmpTool){
    case 1:return tr(L"RELIER : cliquez le node de depart puis le node d'arrivee.");
    case 2:return tr(L"SORTIE N : raccord vers le bloc voisin au Nord.");
    case 3:return tr(L"SORTIE E : raccord vers le bloc voisin a l'Est.");
    case 4:return tr(L"SORTIE S : raccord vers le bloc voisin au Sud.");
    case 5:return tr(L"SORTIE O : raccord vers le bloc voisin a l'Ouest.");
    case 6:return tr(L"SUPPRIMER : cliquez un node pour le retirer.");
    default:return tr(L"POSER : clic case vide = nouveau node ; clic node = selection.");
    }
}
static void rmp_template_from_selected(void){
    if(A.rmp.selected<0||A.rmp.selected>=A.rmp.count)return;RmpNode*n=&A.rmp.nodes[A.rmp.selected];
    A.rmpNewType=n->raw[19]&3;A.rmpNewRank=n->raw[20];A.rmpNewFlags=n->raw[21];A.rmpNewTarget=(n->raw[22]==5);A.rmpNewPriority=n->raw[23];
}
static void rmp_apply_editor_values_to_selected(int what){
    if(A.rmp.selected<0||A.rmp.selected>=A.rmp.count||current_map_source()!=SRC_MOD)return;RmpNode*n=&A.rmp.nodes[A.rmp.selected];
    if(what==0)n->raw[19]=(uint8_t)((n->raw[19]&4)|(A.rmpNewType&3));
    else if(what==1)n->raw[20]=(uint8_t)clampi(A.rmpNewRank,0,255);
    else if(what==2)n->raw[21]=(uint8_t)clampi(A.rmpNewFlags,0,255);
    else if(what==3)n->raw[23]=(uint8_t)clampi(A.rmpNewPriority,0,255);
    else if(what==4)n->raw[22]=(uint8_t)(A.rmpNewTarget?5:0);
    A.rmp.dirty=1;
}
static void rmp_set_simple_route_template(int applySelected){
    A.rmpNewType=0;A.rmpNewRank=0;A.rmpNewFlags=1;A.rmpNewTarget=0;A.rmpNewPriority=0;
    if(applySelected&&A.rmp.selected>=0&&A.rmp.selected<A.rmp.count&&current_map_source()==SRC_MOD){
        RmpNode*n=&A.rmp.nodes[A.rmp.selected];n->raw[19]=(uint8_t)(n->raw[19]&4);n->raw[20]=0;n->raw[21]=1;n->raw[22]=0;n->raw[23]=0;A.rmp.dirty=1;
    }
}
static int rmp_template_is_simple(void){
    return (A.rmpNewType&3)==0&&A.rmpNewRank==0&&A.rmpNewFlags==1&&!A.rmpNewTarget&&A.rmpNewPriority==0;
}
static void draw_rmp_toolbar(HDC hdc){
    if(!A.rmp.editMode||!A.rmp.show||A.scene.active)return;
    int x0=A.sidebarW+12,right=A.clientW-A.inspectorW-12;if(right-x0<620)return;int y=106;
    int bottom=A.rmpAdvanced?246:194;
    fill_rect_color(hdc,x0-4,y-4,right,bottom,RGB(11,24,31));
    SetBkMode(hdc,TRANSPARENT);SetTextColor(hdc,RGB(105,220,230));TextOutW(hdc,x0,y+6,tr(L"ROUTES RMP"),10);
    int x=x0+82;
    draw_button(hdc,x,y,x+62,y+26,tr(L"POSER"),A.rmpTool==0);x+=66;
    draw_button(hdc,x,y,x+66,y+26,tr(L"RELIER"),A.rmpTool==1);x+=70;
    draw_button(hdc,x,y,x+34,y+26,L"N",A.rmpTool==2);x+=38;
    draw_button(hdc,x,y,x+34,y+26,L"E",A.rmpTool==3);x+=38;
    draw_button(hdc,x,y,x+34,y+26,L"S",A.rmpTool==4);x+=38;
    draw_button(hdc,x,y,x+34,y+26,L"O",A.rmpTool==5);x+=38;
    draw_button(hdc,x,y,x+72,y+26,tr(L"SUPPR"),A.rmpTool==6);x+=80;
    SetTextColor(hdc,RGB(155,182,190));if(x<right-20)TextOutW(hdc,x,y+6,rmp_tool_desc(),(int)wcslen(rmp_tool_desc()));

    y+=32;x=x0;SetTextColor(hdc,RGB(155,180,188));TextOutW(hdc,x,y+6,tr(L"Profil :"),8);x+=62;
    draw_button(hdc,x,y,x+104,y+26,tr(L"ROUTE SIMPLE"),rmp_template_is_simple());x+=110;
    SetTextColor(hdc,RGB(155,180,188));TextOutW(hdc,x,y+6,tr(L"Acces :"),7);x+=52;
    draw_button(hdc,x,y,x+50,y+26,tr(L"TOUS"),(A.rmpNewType&3)==0);x+=54;
    draw_button(hdc,x,y,x+48,y+26,L"1x1",(A.rmpNewType&3)==2);x+=52;
    draw_button(hdc,x,y,x+48,y+26,tr(L"VOL"),(A.rmpNewType&3)==1);x+=52;
    draw_button(hdc,x,y,x+78,y+26,L"1x1+VOL",(A.rmpNewType&3)==3);x+=86;
    draw_button(hdc,x,y,x+86,y+26,A.rmpAdvanced?tr(L"AVANCE -"):tr(L"AVANCE +"),A.rmpAdvanced);x+=92;
    SetTextColor(hdc,RGB(120,155,164));if(x<right-20){const wchar_t*msg=tr(L"Route simple suffit dans la plupart des cas.");TextOutW(hdc,x,y+6,msg,(int)wcslen(msg));}

    y+=32;SetTextColor(hdc,RGB(115,150,160));{
        const wchar_t*msg=tr(L"Acces TOUS = aucune restriction | 1x1 = exclut les grosses unites | VOL = reserve aux unites volantes.");
        TextOutW(hdc,x0,y+4,msg,(int)wcslen(msg));
    }
    if(!A.rmpAdvanced)return;

    y+=28;x=x0;SetTextColor(hdc,RGB(155,180,188));TextOutW(hdc,x,y+6,tr(L"Avance :"),8);x+=62;
    draw_button(hdc,x,y,x+26,y+26,L"<",0);x+=30;
    wchar_t rb[128];if(A.rmpNewRank>=0&&A.rmpNewRank<=9)_snwprintf(rb,127,L"Rang:%ls",rmp_rank_name(A.rmpNewRank));else _snwprintf(rb,127,L"Rang:RAW%d",A.rmpNewRank);draw_button(hdc,x,y,x+116,y+26,rb,0);x+=120;
    draw_button(hdc,x,y,x+26,y+26,L">",0);x+=34;
    draw_button(hdc,x,y,x+26,y+26,L"-",0);x+=30;wchar_t pb[64];_snwprintf(pb,63,L"Pat:%d",A.rmpNewFlags);draw_button(hdc,x,y,x+62,y+26,pb,0);x+=66;draw_button(hdc,x,y,x+26,y+26,L"+",0);x+=34;
    draw_button(hdc,x,y,x+26,y+26,L"-",0);x+=30;wchar_t sb[64];_snwprintf(sb,63,L"Spawn:%d",A.rmpNewPriority);draw_button(hdc,x,y,x+76,y+26,sb,0);x+=80;draw_button(hdc,x,y,x+26,y+26,L"+",0);x+=34;
    draw_button(hdc,x,y,x+76,y+26,A.rmpNewTarget?tr(L"CIBLE:OUI"):tr(L"CIBLE:NON"),A.rmpNewTarget);x+=84;
    SetTextColor(hdc,RGB(120,155,164));if(x<right-20){const wchar_t*msg=tr(L"Cas speciaux seulement.");TextOutW(hdc,x,y+6,msg,(int)wcslen(msg));}
    SetTextColor(hdc,RGB(115,150,160));{const wchar_t*msg=tr(L"Rang = categorie RMP/spawn | Pat = desirabilite IA | Spawn 0 = aucun spawn | Cible = defense de base.");TextOutW(hdc,x0,y+34,msg,(int)wcslen(msg));}
}
static int rmp_toolbar_click(int mx,int my){
    if(!A.rmp.editMode||!A.rmp.show||A.scene.active)return 0;int x0=A.sidebarW+12,right=A.clientW-A.inspectorW-12,y0=102;int bottom=A.rmpAdvanced?246:194;if(mx<x0-4||mx>=right||my<y0||my>=bottom)return 0;
    int y=106,x=x0+82;
    int widths[7]={62,66,34,34,34,34,72};
    for(int i=0;i<7;i++){if(mx>=x&&mx<x+widths[i]&&my>=y&&my<y+26){A.rmpTool=i;set_status(rmp_tool_desc());InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=widths[i]+4;}
    y=138;x=x0+62;
    if(mx>=x&&mx<x+104&&my>=y&&my<y+26){rmp_set_simple_route_template(1);set_status(tr(L"Profil ROUTE SIMPLE : navigation standard, aucune restriction, pas de spawn."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=110;
    x+=52;
    int typevals[4]={0,2,1,3};int typew[4]={50,48,48,78};
    for(int i=0;i<4;i++){if(mx>=x&&mx<x+typew[i]&&my>=y&&my<y+26){A.rmpNewType=typevals[i];rmp_apply_editor_values_to_selected(0);set_status(i==0?tr(L"Acces TOUS : aucune restriction de taille ou de vol."):i==1?tr(L"Acces 1x1 : grandes unites exclues de ce node."):i==2?tr(L"Acces VOL : seules les unites volantes peuvent utiliser ce node."):tr(L"Acces 1x1+VOL : reserve aux petites unites volantes."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=typew[i]+4;}
    x+=4;
    if(mx>=x&&mx<x+86&&my>=y&&my<y+26){A.rmpAdvanced=!A.rmpAdvanced;set_status(A.rmpAdvanced?tr(L"Parametres RMP avances affiches."):tr(L"Parametres RMP avances masques. Le profil simple reste actif."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}
    if(!A.rmpAdvanced)return 1;
    y=198;x=x0+62;
    if(mx>=x&&mx<x+26&&my>=y&&my<y+26){A.rmpNewRank=(A.rmpNewRank<=0?9:A.rmpNewRank-1);rmp_apply_editor_values_to_selected(1);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=30;
    if(mx>=x&&mx<x+116&&my>=y&&my<y+26){A.rmpNewRank=(A.rmpNewRank>=9?0:A.rmpNewRank+1);rmp_apply_editor_values_to_selected(1);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=120;
    if(mx>=x&&mx<x+26&&my>=y&&my<y+26){A.rmpNewRank=(A.rmpNewRank>=9?0:A.rmpNewRank+1);rmp_apply_editor_values_to_selected(1);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=34;
    if(mx>=x&&mx<x+26&&my>=y&&my<y+26){A.rmpNewFlags=clampi(A.rmpNewFlags-1,0,255);rmp_apply_editor_values_to_selected(2);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=30+62+4;
    if(mx>=x&&mx<x+26&&my>=y&&my<y+26){A.rmpNewFlags=clampi(A.rmpNewFlags+1,0,255);rmp_apply_editor_values_to_selected(2);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=34;
    if(mx>=x&&mx<x+26&&my>=y&&my<y+26){A.rmpNewPriority=clampi(A.rmpNewPriority-1,0,255);rmp_apply_editor_values_to_selected(3);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=30+76+4;
    if(mx>=x&&mx<x+26&&my>=y&&my<y+26){A.rmpNewPriority=clampi(A.rmpNewPriority+1,0,255);rmp_apply_editor_values_to_selected(3);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=34;
    if(mx>=x&&mx<x+76&&my>=y&&my<y+26){A.rmpNewTarget=!A.rmpNewTarget;rmp_apply_editor_values_to_selected(4);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}
    return 1;
}
static void blueprint_capture_cancel(void){A.blueprintCaptureMode=0;A.blueprintCaptureCount=0;set_status(tr(L"Capture blueprint perso annulee."));InvalidateRect(A.hwnd,NULL,FALSE);}
static int blueprint_capture_sprite_hit(int lib,int local,int dx,int dy,int mx,int my,int scale){
    if(lib<0||lib>=A.libraryCount||local<0||scale<=0)return 0;
    int dw=TILE_W*scale,dh=TILE_H*scale;if(mx<dx||my<dy||mx>=dx+dw||my>=dy+dh)return 0;
    int rx=mx-dx,ry=my-dy;
    if(A.assetRenderMode!=0){HDCacheEntry*e=hd_load_entry(lib,local);if(e&&e->pixels){int sx=(int)((int64_t)rx*e->w/dw),sy=(int)((int64_t)ry*e->h/dh);if(sx<0||sy<0||sx>=e->w||sy>=e->h)return 0;return (int)((e->pixels[(size_t)sy*e->w+sx]>>24)&255)>8;}}
    if(!library_load(lib))return 0;LibrarySet*s=&A.library[lib];int frame=mcd_frame(lib,local);if(!s->sprites||frame<0||frame>=s->frameCount)return 0;int sx=rx/scale,sy=ry/scale;if(sx<0||sy<0||sx>=TILE_W||sy>=TILE_H)return 0;return s->sprites[(size_t)frame*TILE_W*TILE_H+sy*TILE_W+sx]!=0;
}
static int hit_piece_at_levels(int mx,int my,int*hx,int*hy,int*hz,int*hlayer,int*hlib,int*hlocal,int zmin,int zmax){
    int dxmax=A.scene.active?A.scene.x:A.map.x,dymax=A.scene.active?A.scene.y:A.map.y;if(dxmax<=0||dymax<=0)return 0;
    int ox,oy;world_origin(&ox,&oy);
    /* Reverse exact draw order: first opaque sprite found is the visually topmost piece. */
    for(int z=zmax;z>=zmin;z--)for(int y=dymax-1;y>=0;y--)for(int x=dxmax-1;x>=0;x--){
        int sx,sy;project_tile(x,y,z,&sx,&sy);sx=ox+sx*A.zoom;sy=oy+sy*A.zoom;
        for(int layer=3;layer>=0;layer--){int lib=-1,local=-1;
            if(A.scene.active){SceneCell*c=scene_cell_at(x,y,z);if(c){lib=c->lib[layer];local=c->local[layer];}}
            else{MapCell*c=cell_at(x,y,z);if(c&&c->part[layer])active_resolve_raw(c->part[layer],&lib,&local);}
            if(lib<0||local<0)continue;int py=sy-mcd_plevel(lib,local)*A.zoom;
            int hit=rh_hit(x,y,z,layer,lib,local,sx,sy,mx,my);if(hit>0||(hit<0&&blueprint_capture_sprite_hit(lib,local,sx,py,mx,my,A.zoom))){*hx=x;*hy=y;*hz=z;*hlayer=layer;*hlib=lib;*hlocal=local;return 1;}
        }
    }
    /* Fallback ergonomique : case courante, toutes couches, sans imposer la couche d'edition. */
    int x,y;if(!mouse_to_tile(mx,my,&x,&y))return 0;int z=A.currentZ;
    for(int layer=3;layer>=0;layer--){int lib=-1,local=-1;
        if(A.scene.active){SceneCell*c=scene_cell_at(x,y,z);if(c){lib=c->lib[layer];local=c->local[layer];}}
        else{MapCell*c=cell_at(x,y,z);if(c&&c->part[layer])active_resolve_raw(c->part[layer],&lib,&local);}
        if(lib>=0&&local>=0){*hx=x;*hy=y;*hz=z;*hlayer=layer;*hlib=lib;*hlocal=local;return 1;}
    }
    return 0;
}
static int blueprint_capture_hit_piece(int mx,int my,int*hx,int*hy,int*hz,int*hlayer,int*hlib,int*hlocal){return hit_piece_at_levels(mx,my,hx,hy,hz,hlayer,hlib,hlocal,normal_z_min(),normal_z_max());}
static int blueprint_capture_click(int mx,int my){
    if(!A.blueprintCaptureMode)return 0;int x=-1,y=-1,z=-1,layer=-1,lib=-1,local=-1;
    if(!blueprint_capture_hit_piece(mx,my,&x,&y,&z,&layer,&lib,&local)){set_status(tr(L"Capture perso : aucune piece visible sous le curseur."));return 1;}
    for(int i=0;i<A.blueprintCaptureCount;i++){BlueprintCapturePart*p=&A.blueprintCapture[i];if(p->x==x&&p->y==y&&p->z==z&&p->layer==layer){memmove(&A.blueprintCapture[i],&A.blueprintCapture[i+1],sizeof(BlueprintCapturePart)*(A.blueprintCaptureCount-i-1));A.blueprintCaptureCount--;wchar_t st[180];_snwprintf(st,179,tr(L"Capture perso : piece retiree. %d piece(s) selectionnee(s)."),A.blueprintCaptureCount);set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);if(BUI.hwnd){blueprint_hangar_refresh();}return 1;}}
    if(A.blueprintCaptureCount>=MAX_BLUEPRINT_CAPTURE){set_status(tr(L"Capture perso : limite de pieces atteinte."));return 1;}BlueprintCapturePart*p=&A.blueprintCapture[A.blueprintCaptureCount++];p->x=x;p->y=y;p->z=z;p->layer=layer;p->lib=lib;p->local=local;wchar_t st[300];_snwprintf(st,299,tr(L"Capture perso : %ls/MCD%d ajoute (%ls X%d Y%d Z%d). Selection visuelle toutes couches. %d piece(s)."),A.library[lib].name,local,layer_name(layer),x,y,z,A.blueprintCaptureCount);set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);if(BUI.hwnd){blueprint_hangar_refresh();}return 1;
}
static void custom_blueprint_unique_name(const wchar_t*requested,wchar_t*out,int cap){
    wchar_t base[96];wcsncpy(base,(requested&&requested[0])?requested:tr(L"PERSO"),95);base[95]=0;
    for(int pass=0;pass<1000;pass++){
        wchar_t cand[96];if(pass==0)wcsncpy(cand,base,95);else _snwprintf(cand,95,L"%ls_%d",base,pass+1);cand[95]=0;
        int used=0;for(int i=0;i<A.customBlueprintCount;i++)if(_wcsicmp(A.customBlueprints[i].name,cand)==0){used=1;break;}
        if(!used){wcsncpy(out,cand,cap-1);out[cap-1]=0;return;}
    }
    _snwprintf(out,cap-1,L"PERSO_%03d",A.customBlueprintCount+1);out[cap-1]=0;
}
static int blueprint_capture_save_named(const wchar_t*name){
    if(!A.blueprintCaptureMode)return -1;if(A.blueprintCaptureCount<2){set_status(tr(L"Blueprint perso : selectionnez au moins 2 pieces avant d'enregistrer."));return -1;}if(A.customBlueprintCount>=MAX_CUSTOM_BLUEPRINTS||A.customBlueprintPartCount+A.blueprintCaptureCount>MAX_CUSTOM_BLUEPRINT_PARTS){set_status(tr(L"Blueprint perso : capacite de stockage atteinte."));return -1;}
    int minx=999999,miny=999999,minz=999999,maxx=-999999,maxy=-999999,maxz=-999999;for(int i=0;i<A.blueprintCaptureCount;i++){BlueprintCapturePart*p=&A.blueprintCapture[i];if(p->x<minx)minx=p->x;if(p->y<miny)miny=p->y;if(p->z<minz)minz=p->z;if(p->x>maxx)maxx=p->x;if(p->y>maxy)maxy=p->y;if(p->z>maxz)maxz=p->z;}
    int oldBc=A.customBlueprintCount,oldPc=A.customBlueprintPartCount;int bi=A.customBlueprintCount++;CustomBlueprintDef*b=&A.customBlueprints[bi];ZeroMemory(b,sizeof(*b));custom_blueprint_unique_name(name,b->name,96);b->firstPart=A.customBlueprintPartCount;b->partCount=A.blueprintCaptureCount;b->spanX=maxx-minx+1;b->spanY=maxy-miny+1;b->spanZ=maxz-minz+1;
    for(int i=0;i<A.blueprintCaptureCount;i++){BlueprintCapturePart*c=&A.blueprintCapture[i];CustomBlueprintPart*p=&A.customBlueprintParts[A.customBlueprintPartCount++];ZeroMemory(p,sizeof(*p));wcsncpy(p->dataset,A.library[c->lib].name,95);p->dataset[95]=0;p->mcd=c->local;p->layer=c->layer;p->dx=c->x-minx;p->dy=c->y-miny;p->dz=c->z-minz;}
    if(!custom_blueprints_save()){A.customBlueprintCount=oldBc;A.customBlueprintPartCount=oldPc;MessageBoxW(A.hwnd,tr(L"Echec de l'enregistrement du blueprint dans le Hangar.\\n\\nAucune modification du Hangar n'a ete conservee ; votre capture reste active pour pouvoir reessayer."),APP_TITLE,MB_ICONERROR);set_status(tr(L"Blueprint perso : echec d'enregistrement, capture conservee."));return -1;}
    A.selectedLib=A.blueprintCapture[0].lib;A.selectedLocal=A.blueprintCapture[0].local;A.customBlueprintIndex=bi;A.blueprintIndex=-1;A.blueprintMode=2;E.tool=EDIT_PLACE;A.blueprintCaptureMode=0;A.blueprintCaptureCount=0;wchar_t st[360];_snwprintf(st,359,tr(L"Blueprint perso %ls enregistre dans le Hangar : %d pieces, etendue %dx%dx%d."),b->name,b->partCount,b->spanX,b->spanY,b->spanZ);set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);return bi;
}

static void blueprint_capture_save(void){
    wchar_t autoName[96];_snwprintf(autoName,95,L"PERSO_%03d",A.customBlueprintCount+1);autoName[95]=0;blueprint_capture_save_named(autoName);
}
static int blueprint_from_map_entry(int mi){
    if(mi<0||mi>=A.mapCount)return -1;MapEntry*m=&A.maps[mi];int pi=m->profileIndex;
    if(pi<0)pi=profile_find_best_ctx(m->name,m->source,m->origin);
    if(pi<0){MessageBoxW(A.hwnd,tr(L"Impossible de copier cette MAP comme blueprint : aucun profil de datasets n'a ete identifie.\n\nLe Workshop ne peut pas convertir les indices MAP en dataset/MCD sans ce profil."),APP_TITLE,MB_ICONERROR);return -1;}
    MapProfile*q=&A.profiles[pi];for(int k=0;k<q->dataSetCount;k++)if(library_find_name_ctx(q->dataSets[k],m->source,m->origin)<0){wchar_t msg[520];_snwprintf(msg,519,tr(L"Copie annulee : le dataset %ls du profil de cette MAP est introuvable dans la chaine de sources autorisee.\n\nAucun fallback vers un autre mod et aucun blueprint partiel ne seront utilises."),q->dataSets[k]);MessageBoxW(A.hwnd,msg,APP_TITLE,MB_ICONERROR);return -1;}
    DWORD sz=0;uint8_t*raw=read_all(m->path,&sz);if(!raw||sz<3){free(raw);MessageBoxW(A.hwnd,tr(L"Impossible de lire cette MAP."),APP_TITLE,MB_ICONERROR);return -1;}
    int sy=raw[0],sx=raw[1],szm=raw[2];uint32_t need=3u+(uint32_t)sx*sy*szm*4u;if(!sx||!sy||!szm||need>sz){free(raw);MessageBoxW(A.hwnd,tr(L"MAP invalide ou tronquee."),APP_TITLE,MB_ICONERROR);return -1;}
    int count=0;for(uint32_t off=3;off<need;off++)if(raw[off])count++;
    if(count<=0){free(raw);MessageBoxW(A.hwnd,tr(L"Cette MAP ne contient aucune piece a copier."),APP_TITLE,MB_ICONINFORMATION);return -1;}
    if(A.customBlueprintCount>=MAX_CUSTOM_BLUEPRINTS||A.customBlueprintPartCount+count>MAX_CUSTOM_BLUEPRINT_PARTS){free(raw);wchar_t msg[420];_snwprintf(msg,419,tr(L"Impossible de copier toute la MAP : elle contient %d pieces et le Hangar n'a plus assez de capacite.\n\nCapacite restante : %d pieces."),count,MAX_CUSTOM_BLUEPRINT_PARTS-A.customBlueprintPartCount);MessageBoxW(A.hwnd,msg,APP_TITLE,MB_ICONERROR);return -1;}
    CustomBlueprintPart*tmp=(CustomBlueprintPart*)calloc((size_t)count,sizeof(CustomBlueprintPart));if(!tmp){free(raw);MessageBoxW(A.hwnd,tr(L"Memoire insuffisante pour construire le blueprint."),APP_TITLE,MB_ICONERROR);return -1;}
    uint32_t off=3;int n=0,minx=sx,miny=sy,minz=szm,maxx=-1,maxy=-1,maxz=-1,unresolved=0;
    for(int fz=0;fz<szm;fz++){int z=szm-1-fz;for(int y=0;y<sy;y++)for(int x=0;x<sx;x++)for(int layer=0;layer<4;layer++){
        int rv=raw[off++];if(rv<=0)continue;int lib=-1,local=-1;if(!profile_resolve_raw(pi,rv,m->source,m->origin,&lib,&local)){unresolved++;continue;}
        CustomBlueprintPart*p=&tmp[n++];wcsncpy(p->dataset,A.library[lib].name,95);p->dataset[95]=0;p->mcd=local;p->layer=layer;p->dx=x;p->dy=y;p->dz=z;
        if(x<minx)minx=x;if(y<miny)miny=y;if(z<minz)minz=z;if(x>maxx)maxx=x;if(y>maxy)maxy=y;if(z>maxz)maxz=z;
    }}
    free(raw);
    if(unresolved>0||n!=count){free(tmp);wchar_t msg[520];_snwprintf(msg,519,tr(L"Copie annulee : %d piece(s) de la MAP ne peuvent pas etre resolues vers un dataset/MCD avec le profil strict de cette source.\n\nAucun blueprint partiel n'a ete cree."),unresolved);MessageBoxW(A.hwnd,msg,APP_TITLE,MB_ICONERROR);return -1;}
    int oldBc=A.customBlueprintCount,oldPc=A.customBlueprintPartCount;int bi=A.customBlueprintCount++;CustomBlueprintDef*b=&A.customBlueprints[bi];ZeroMemory(b,sizeof(*b));wchar_t requested[96];_snwprintf(requested,95,L"MAP_%ls",m->name);requested[95]=0;custom_blueprint_unique_name(requested,b->name,96);b->firstPart=A.customBlueprintPartCount;b->partCount=n;b->spanX=maxx-minx+1;b->spanY=maxy-miny+1;b->spanZ=maxz-minz+1;
    for(int i=0;i<n;i++){CustomBlueprintPart*p=&A.customBlueprintParts[A.customBlueprintPartCount++];*p=tmp[i];p->dx-=minx;p->dy-=miny;p->dz-=minz;}free(tmp);if(!custom_blueprints_save()){A.customBlueprintCount=oldBc;A.customBlueprintPartCount=oldPc;MessageBoxW(A.hwnd,tr(L"Impossible d'enregistrer cette MAP dans le Hangar.\n\nLe stockage du Hangar n'a pas ete modifie et aucun blueprint partiel n'a ete conserve."),APP_TITLE,MB_ICONERROR);return -1;}
    A.customBlueprintIndex=bi;A.blueprintIndex=-1;A.blueprintMode=2;
    open_blueprint_hangar();if(BUI.hwnd){BUI.selected=bi;blueprint_hangar_refresh();if(BUI.nameEdit)SetWindowTextW(BUI.nameEdit,b->name);}
    wchar_t st[420];_snwprintf(st,419,tr(L"MAP %ls copiee dans le Hangar comme blueprint %ls : %d pieces, etendue %dx%dx%d."),m->name,b->name,b->partCount,b->spanX,b->spanY,b->spanZ);set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);return bi;
}
static int custom_blueprint_delete(int bi){
    if(bi<0||bi>=A.customBlueprintCount)return 0;CustomBlueprintDef*b=&A.customBlueprints[bi];int first=b->firstPart,pc=b->partCount;
    if(pc>0&&first>=0&&first+pc<=A.customBlueprintPartCount){memmove(&A.customBlueprintParts[first],&A.customBlueprintParts[first+pc],sizeof(CustomBlueprintPart)*(A.customBlueprintPartCount-first-pc));A.customBlueprintPartCount-=pc;}
    for(int i=bi+1;i<A.customBlueprintCount;i++)A.customBlueprints[i].firstPart-=pc;
    memmove(&A.customBlueprints[bi],&A.customBlueprints[bi+1],sizeof(CustomBlueprintDef)*(A.customBlueprintCount-bi-1));A.customBlueprintCount--;
    if(A.customBlueprintIndex==bi){A.customBlueprintIndex=-1;A.blueprintMode=0;}else if(A.customBlueprintIndex>bi)A.customBlueprintIndex--;
    custom_blueprints_save();return 1;
}
static void draw_blueprint_toolbar(HDC hdc){
    if(A.rmp.editMode||(!A.map.cells&&!A.scene.active))return;int x=A.sidebarW+12,y=106;fill_rect_color(hdc,x-4,y-4,A.clientW-A.inspectorW-8,170,RGB(11,24,31));
    if(A.blueprintCaptureMode){draw_button(hdc,x,y,x+116,y+26,tr(L"Enregistrer"),0);draw_button(hdc,x+122,y,x+222,y+26,tr(L"Annuler"),0);}
    else{draw_button(hdc,x,y,x+100,y+26,tr(L"Piece seule"),A.blueprintMode==0);draw_button(hdc,x+104,y,x+218,y+26,tr(L"Assemblage >"),A.blueprintMode!=0);draw_button(hdc,x+222,y,x+300,y+26,L"Hangar",0);draw_button(hdc,x+304,y,x+392,y+26,tr(L"Capturer"),0);}
    wchar_t hint[240];if(A.blueprintCaptureMode)_snwprintf(hint,239,tr(L"Capture : %d pieces. Cliquez les pieces visibles."),A.blueprintCaptureCount);else _snwprintf(hint,239,tr(L"Clic droit / Echap : lacher la piece ou l'assemblage."));SetTextColor(hdc,RGB(160,190,200));TextOutW(hdc,x,144,hint,(int)wcslen(hint));
}
static int blueprint_toolbar_click(int mx,int my){
    if(A.rmp.editMode||(!A.map.cells&&!A.scene.active))return 0;int x=A.sidebarW+12;
    if(mx<x-4||mx>=A.clientW-A.inspectorW-8||my<102||my>=170)return 0;
    if(my<106||my>=132)return 1;
    if(A.blueprintCaptureMode){if(mx<x+116){blueprint_capture_save();blueprint_hangar_refresh();}else if(mx>=x+122&&mx<x+222){blueprint_capture_cancel();blueprint_hangar_refresh();}return 1;}
    if(mx<x+100){A.blueprintMode=0;InvalidateRect(A.hwnd,NULL,FALSE);return 1;}
    if(mx>=x+104&&mx<x+218){
        int hm[64],cm[64],hn=0,cn=0;if(A.selectedLib>=0&&A.selectedLib<A.libraryCount){hn=blueprint_find_matches_for(A.selectedLib,A.selectedLocal,hm,64);cn=custom_blueprint_find_matches_for(A.selectedLib,A.selectedLocal,cm,64);}if(hn>64)hn=64;if(cn>64)cn=64;
        HMENU menu=CreatePopupMenu();for(int i=0;i<hn;i++){wchar_t text[240];_snwprintf(text,239,tr(L"Habituel : %ls"),BLUEPRINTS_RC15[hm[i]].id);AppendMenuW(menu,MF_STRING,1+i,text);}for(int i=0;i<cn;i++){wchar_t text[240];_snwprintf(text,239,tr(L"Personnel : %ls"),A.customBlueprints[cm[i]].name);AppendMenuW(menu,MF_STRING,101+i,text);}AppendMenuW(menu,MF_SEPARATOR,0,NULL);AppendMenuW(menu,MF_STRING,200,tr(L"Tous mes assemblages : ouvrir le Hangar..."));
        POINT pt={x+104,132};ClientToScreen(A.hwnd,&pt);int id=TrackPopupMenu(menu,TPM_RETURNCMD,pt.x,pt.y,0,A.hwnd,NULL);DestroyMenu(menu);
        if(id>=1&&id<=hn){A.blueprintMode=1;A.blueprintIndex=hm[id-1];}else if(id>=101&&id<101+cn){A.blueprintMode=2;A.customBlueprintIndex=cm[id-101];}else if(id==200)open_blueprint_hangar();InvalidateRect(A.hwnd,NULL,FALSE);return 1;
    }
    if(mx>=x+222&&mx<x+300){open_blueprint_hangar();return 1;}
    if(mx>=x+304&&mx<x+392){A.blueprintCaptureMode=1;A.blueprintCaptureCount=0;A.blueprintMode=0;open_blueprint_hangar();if(BUI.nameEdit){wchar_t nm[96];_snwprintf(nm,95,L"Blueprint_%03d",A.customBlueprintCount+1);SetWindowTextW(BUI.nameEdit,nm);}blueprint_hangar_refresh();set_status(tr(L"Capture : cliquez les pieces visibles, puis Enregistrer."));InvalidateRect(A.hwnd,NULL,FALSE);}return 1;
}

static const wchar_t* plan_tool_name(void){const wchar_t*n[]={tr(L"CASE"),tr(L"MUR"),tr(L"PORTE"),tr(L"LIEN Z"),tr(L"SELECTION"),tr(L"VERROU")};return (A.planTool>=0&&A.planTool<6)?n[A.planTool]:tr(L"?");}
static const wchar_t* plan_vlink_name(int v){const wchar_t*n[]={tr(L"AUCUN"),tr(L"ESCALIER +"),tr(L"ESCALIER -"),tr(L"RAMPE"),tr(L"ECHELLE"),tr(L"TROU"),tr(L"PUITS"),tr(L"ASCENSEUR")};return (v>=0&&v<8)?n[v]:tr(L"?");}
static void draw_plan_toolbar(HDC dc){
 int l=A.sidebarW+12,r=A.clientW-A.inspectorW-12,w=(r-l-20)/6;
 fill_rect_color(dc,l-6,68,r+6,140,RGB(17,28,35));
 const wchar_t*tools[]={tr(L"Case"),tr(L"Mur"),tr(L"Porte"),tr(L"Lien Z"),L"Zone",tr(L"Verrou")};
 const wchar_t*actions[]={tr(L"Copier"),tr(L"Coller"),tr(L"Gomme"),tr(L"Pieces"),tr(L"Plan 2D"),tr(L"Plan ISO")};
 for(int i=0;i<6;i++){int x=l+i*(w+4);draw_button(dc,x,72,x+w,100,tools[i],A.planTool==i);draw_button(dc,x,106,x+w,134,actions[i],(i==1&&A.planPasteMode)||(i==2&&E.planErase)||(i==4&&A.viewMode==1)||(i==5&&A.viewMode==2));}
}
static int plan_toolbar_click(int mx,int my){
 if(!plan_view_active()||my<68||my>=140)return 0;int l=A.sidebarW+12,r=A.clientW-A.inspectorW-12,w=(r-l-20)/6;if(mx<l||mx>=r)return 1;int i=(mx-l)/(w+4);if(i>5)return 1;
 if(my>=72&&my<100){A.planTool=i;A.planSelecting=0;set_status(plan_tool_name());}
 else if(my>=106&&my<134){if(i==0)plan_copy_selection();else if(i==1){E.planErase=0;plan_begin_paste();}else if(i==2){E.planErase=!E.planErase;A.planPasteMode=0;set_status(E.planErase?tr(L"Gomme PLAN : clic gauche efface le type choisi. Clic droit termine."):tr(L"Edition du plan active."));}else handle_command(i==3?IDM_VIEW_NORMAL:i==4?IDM_VIEW_TOP:IDM_VIEW_ISO);}
 InvalidateRect(A.hwnd,NULL,FALSE);return 1;
}
static int plan_panel_click(int mx,int my){
    if(!plan_view_active())return 0;int l=A.clientW-A.inspectorW;if(mx<l)return 0;
    if(my>=60&&my<86){if(mx>=l+14&&mx<l+158){A.planSemanticLayer=0;A.planTool=0;set_status(tr(L"PLAN : edition SOL / ZONE."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}if(mx>=l+166&&mx<l+326){A.planSemanticLayer=1;A.planTool=0;set_status(tr(L"PLAN : edition OBJET / DECOR."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}}
    int dw=(A.inspectorW-36)/4;for(int i=0;i<4;i++){int x=l+14+i*dw;if(mx>=x&&mx<x+dw-4&&my>=92&&my<118){A.planDisplayMode=i;if(i==1){A.planTool=0;A.planSemanticLayer=0;}else if(i==2){A.planTool=1;}else if(i==3){A.planTool=0;A.planSemanticLayer=1;}wchar_t st[220];_snwprintf(st,219,tr(L"PLAN : affichage %ls%ls"),tr(PLAN_DISPLAY_NAMES[i]),i==0?tr(L" ; utilisez SOL, MUR, PORTE ou OBJET pour choisir ce que vous editez."):(i==1?tr(L" ; edition SOL active."):(i==2?tr(L" ; edition MURS active."):tr(L" ; edition OBJETS active."))));set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}}
    int bw=(A.inspectorW-36)/4;for(int i=0;i<4;i++){int x=l+14+i*bw;if(mx>=x&&mx<x+bw-4&&my>=126&&my<152){A.brushMode=i;set_status(tr(L"Pinceau PLAN configure."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}}
    if(my>=158&&my<184){int x=l+14;if(mx>=x&&mx<x+28){A.brushSize=clampi(A.brushSize-1,1,16);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x=l+98;if(mx>=x&&mx<x+28){A.brushSize=clampi(A.brushSize+1,1,16);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x=l+140;for(int i=0;i<3;i++){int w=60;if(mx>=x&&mx<x+w){A.brushShape=i;InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=w+4;}}
    int startY=210,sw=(A.inspectorW-42)/2,sh=25,count=A.planSemanticLayer?PLAN_OBJECT_COUNT:PLAN_SEMANTIC_COUNT;for(int i=0;i<count;i++){int col=i%2,row=i/2,x=l+14+col*(sw+8),y=startY+row*(sh+3);if(mx>=x&&mx<x+sw&&my>=y&&my<y+sh){if(A.planSemanticLayer)A.planObjectSemantic=i;else A.planSemantic=i;A.planTool=0;wchar_t st[220];_snwprintf(st,219,tr(L"PLAN : pinceau %ls = %ls."),A.planSemanticLayer?tr(L"OBJET"):tr(L"SOL/ZONE"),A.planSemanticLayer?plan_object_name(i):plan_semantic_name(i));set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}}
    int ctlY=554;
    if(my>=ctlY&&my<580){int x=l+14;int ww[3]={74,108,108};for(int i=0;i<3;i++){if(mx>=x&&mx<x+ww[i]){A.planWallMode=i==0?0:(i+1);A.planTool=1;set_status(i==0?tr(L"PLAN : murs sur bords de case."):(i==1?tr(L"PLAN : mur diagonal NE-SO."):tr(L"PLAN : mur diagonal NO-SE.")));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}x+=ww[i]+4;}}
    if(my>=604&&my<658){for(int i=0;i<6;i++){int row=i/3,col=i%3,w=(A.inspectorW-36)/3,yy=604+row*29,xx=l+14+col*w;if(mx>=xx&&mx<xx+w-4&&my>=yy&&my<yy+25){A.planFloorShape=i;A.planTool=0;A.planSemanticLayer=0;set_status(tr(L"PLAN : forme de sol selectionnee."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}}}
    int zy=682;for(int i=0;i<7;i++){int col=i%4,row=i/4,x=l+14+col*bw,y=zy+row*29;if(mx>=x&&mx<x+bw-4&&my>=y&&my<y+25){A.planLinkType=i+1;A.planTool=3;wchar_t st[200];_snwprintf(st,199,tr(L"PLAN : liaison verticale = %ls."),plan_vlink_name(A.planLinkType));set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}}
    return inspector_level_click(mx,my);
}
static void draw_plan_panel(HDC hdc,int l){
    fill_rect_color(hdc,l,0,A.clientW,A.clientH,RGB(16,22,27));SetBkMode(hdc,TRANSPARENT);SetTextColor(hdc,RGB(105,215,230));TextOutW(hdc,l+14,12,A.viewMode==2?tr(L"MODE PLAN ISO"):tr(L"MODE PLAN 2D"),A.viewMode==2?13:12);SetTextColor(hdc,RGB(180,200,206));wchar_t h[300];_snwprintf(h,299,tr(L"Outil : %ls   Couche : %ls"),plan_tool_name(),A.planSemanticLayer?tr(L"OBJET / DECOR"):tr(L"SOL / ZONE"));TextOutW(hdc,l+14,38,h,(int)wcslen(h));
    draw_button(hdc,l+14,60,l+158,86,tr(L"SOL / ZONE"),A.planSemanticLayer==0);draw_button(hdc,l+166,60,l+326,86,tr(L"OBJET / DECOR"),A.planSemanticLayer==1);
    int dw=(A.inspectorW-36)/4;for(int i=0;i<4;i++){int x=l+14+i*dw;draw_button(hdc,x,92,x+dw-4,118,tr(PLAN_DISPLAY_NAMES[i]),A.planDisplayMode==i);}
    int bw=(A.inspectorW-36)/4;for(int i=0;i<4;i++){int x=l+14+i*bw;draw_button(hdc,x,126,x+bw-4,152,tr(BRUSH_MODE_NAMES[i]),A.brushMode==i);}
    draw_button(hdc,l+14,158,l+42,184,L"-",0);wchar_t bs[48];_snwprintf(bs,47,L"%d",A.brushSize);draw_button(hdc,l+46,158,l+94,184,bs,0);draw_button(hdc,l+98,158,l+126,184,L"+",0);int sx=l+140;for(int i=0;i<3;i++){draw_button(hdc,sx,158,sx+60,184,tr(BRUSH_SHAPE_NAMES[i]),A.brushShape==i);sx+=64;}
    SetTextColor(hdc,RGB(165,190,198));const wchar_t*sub=A.planSemanticLayer?tr(L"OBJET : petit carre au-dessus du sol"):tr(L"SOL / ZONE : forme et couleur de la case");TextOutW(hdc,l+14,190,sub,(int)wcslen(sub));
    int startY=210,sw=(A.inspectorW-42)/2,sh=25,count=A.planSemanticLayer?PLAN_OBJECT_COUNT:PLAN_SEMANTIC_COUNT;for(int i=0;i<count;i++){int col=i%2,row=i/2,x=l+14+col*(sw+8),y=startY+row*(sh+3);uint32_t cc=A.planSemanticLayer?PLAN_OBJECT_COLORS[i]:PLAN_COLORS[i];COLORREF c=RGB((cc>>16)&255,(cc>>8)&255,cc&255);int active=A.planSemanticLayer?(i==A.planObjectSemantic):(i==A.planSemantic);fill_rect_color(hdc,x,y,x+sw,y+sh,active?RGB(55,115,132):RGB(34,43,49));fill_rect_color(hdc,x+3,y+3,x+22,y+sh-3,c);SetTextColor(hdc,RGB(225,235,238));const wchar_t*n=A.planSemanticLayer?tr(PLAN_OBJECT_NAMES[i]):tr(PLAN_NAMES[i]);TextOutW(hdc,x+27,y+5,n,(int)wcslen(n));}
    SetTextColor(hdc,RGB(225,235,238));TextOutW(hdc,l+14,536,tr(L"MURS"),4);int x=l+14;draw_button(hdc,x,554,x+74,580,tr(L"BORD"),A.planWallMode==0);x+=78;draw_button(hdc,x,554,x+108,580,tr(L"DIAG NE-SO"),A.planWallMode==2);x+=112;draw_button(hdc,x,554,x+108,580,tr(L"DIAG NO-SE"),A.planWallMode==3);
    TextOutW(hdc,l+14,584,tr(L"FORME DU SOL"),12);for(int i=0;i<6;i++){int row=i/3,col=i%3,w=(A.inspectorW-36)/3,yy=604+row*29,xx=l+14+col*w;draw_button(hdc,xx,yy,xx+w-4,yy+25,tr(PLAN_FSHAPE_NAMES[i]),A.planFloorShape==i);}
    TextOutW(hdc,l+14,662,tr(L"LIAISON VERTICALE"),17);const wchar_t*labs[7]={L"ESC+",L"ESC-",tr(L"RAMPE"),L"ECH",tr(L"TROU"),tr(L"PUITS"),L"ASC"};int zy=682;for(int i=0;i<7;i++){int col=i%4,row=i/4,xx=l+14+col*bw,yy=zy+row*29;draw_button(hdc,xx,yy,xx+bw-4,yy+25,labs[i],A.planTool==3&&A.planLinkType==i+1);}
    int infoY=748;if(A.hoverValid){PlanCell*p=plan_cell_at(A.hoverX,A.hoverY,A.currentZ);int fs=plan_floor_display_effective(A.hoverX,A.hoverY,A.currentZ),os=plan_object_effective(A.hoverX,A.hoverY,A.currentZ),dg=plan_diag_effective(A.hoverX,A.hoverY,A.currentZ),sh=plan_floor_shape_effective(A.hoverX,A.hoverY,A.currentZ);_snwprintf(h,299,tr(L"X%d Y%d Z%d | %ls | %ls | sol %ls%ls"),A.hoverX,A.hoverY,A.currentZ,plan_semantic_name(fs),plan_object_effective_name(A.hoverX,A.hoverY,A.currentZ,os),tr(PLAN_FSHAPE_NAMES[sh]),dg==2?tr(L" | diag NE-SO"):(dg==3?tr(L" | diag NO-SE"):L""));SetTextColor(hdc,RGB(210,225,230));TextOutW(hdc,l+14,infoY,h,(int)wcslen(h));if(p&&p->vlink){_snwprintf(h,299,tr(L"Lien Z : %ls"),plan_vlink_name(p->vlink));TextOutW(hdc,l+14,infoY+18,h,(int)wcslen(h));}}
    SetTextColor(hdc,RGB(225,235,238));TextOutW(hdc,l+14,792,tr(L"NIVEAUX"),7);draw_inspector_z_buttons(hdc,l);
}
static int choose_popup(HMENU menu,int x,int y){POINT pt={x,y};ClientToScreen(A.hwnd,&pt);int choice=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_LEFTALIGN,pt.x,pt.y,0,A.hwnd,NULL);DestroyMenu(menu);return choice;}
static void draw_layer_bar(HDC hdc){
    int x=A.sidebarW+12,y=72;wchar_t label[80];
    const wchar_t*layers[4]={tr(L"Sol"),tr(L"Mur O"),tr(L"Mur N"),tr(L"Objet")};_snwprintf(label,79,tr(L"Couche: %ls"),layers[A.selectedLayer]);draw_button(hdc,x,y,x+112,y+28,label,0);x+=116;
    const wchar_t*tools[4]={tr(L"Pinceau >"),tr(L"Contour >"),tr(L"Forme pleine >"),tr(L"Remplissage >")};draw_button(hdc,x,y,x+108,y+28,tools[A.brushMode],0);x+=112;
    _snwprintf(label,79,tr(L"Taille %d"),A.brushSize);draw_button(hdc,x,y,x+68,y+28,label,0);x+=72;
    draw_button(hdc,x,y,x+92,y+28,tr(BRUSH_SHAPE_NAMES[A.brushShape]),0);
}
static int layer_bar_click(int mx,int my){
    if(my<72||my>=100)return 0;int x=A.sidebarW+12,group=-1;int widths[4]={112,108,68,92};
    for(int i=0;i<4;i++){if(mx>=x&&mx<x+widths[i]){group=i;break;}x+=widths[i]+4;}if(group<0)return 0;
    HMENU menu=CreatePopupMenu();int count=group==0?4:(group==1?4:(group==2?16:3));
    const wchar_t*tools[4]={tr(L"Pinceau case par case"),tr(L"Contour de forme"),tr(L"Forme pleine"),tr(L"Remplissage de zone")};
    for(int i=0;i<count;i++){wchar_t text[80];int active=group==0?A.selectedLayer:(group==1?A.brushMode:(group==2?A.brushSize-1:A.brushShape));
        if(group==2)_snwprintf(text,79,L"%d case(s)",i+1);else _snwprintf(text,79,L"%ls",group==0?layer_name(i):(group==1?tools[i]:tr(BRUSH_SHAPE_NAMES[i])));
        AppendMenuW(menu,MF_STRING|(i==active?MF_CHECKED:0),i+1,text);}
    int value=choose_popup(menu,x,100)-1;if(value>=0){if(group==0)A.selectedLayer=value;else if(group==1)A.brushMode=value;else if(group==2)A.brushSize=value+1;else A.brushShape=value;InvalidateRect(A.hwnd,NULL,FALSE);}return 1;
}
static void paint(HWND hwnd,HDC hdc){
    RECT rc;GetClientRect(hwnd,&rc);A.clientW=rc.right;A.clientH=rc.bottom;ensure_backbuf(A.clientW,A.clientH);clear_back(RGB32(9,15,20));draw_map();editor_draw_overlay();
    HDC mem=CreateCompatibleDC(hdc);HBITMAP bmp=CreateCompatibleBitmap(hdc,A.clientW,A.clientH);HGDIOBJ old=SelectObject(mem,bmp);SelectObject(mem,GetStockObject(DEFAULT_GUI_FONT));
    StretchDIBits(mem,0,0,A.clientW,A.clientH,0,0,A.clientW,A.clientH,A.backbuf,&A.bmi,DIB_RGB_COLORS,SRCCOPY);
    int panelClip=SaveDC(mem);IntersectClipRect(mem,0,0,A.sidebarW,A.clientH);draw_sidebar(mem);RestoreDC(mem,panelClip);panelClip=SaveDC(mem);IntersectClipRect(mem,A.clientW-A.inspectorW,0,A.clientW,A.clientH);editor_draw_inspector(mem);RestoreDC(mem,panelClip);int clip=SaveDC(mem);IntersectClipRect(mem,A.sidebarW+5,0,A.clientW-A.inspectorW-5,A.clientH);if(plan_view_active()||A.rmp.editMode){draw_level_bar(mem);if(plan_view_active())draw_plan_toolbar(mem);else{draw_layer_bar(mem);draw_rmp_toolbar(mem);}}else editor_draw_header(mem);
    SetBkMode(mem,TRANSPARENT);SetTextColor(mem,RGB(215,228,232));wchar_t s[PATH_CAP+160];
    if(A.scene.active){_snwprintf(s,PATH_CAP+159,tr(L"COMPOSITEUR : %ls   %dx%dx%d   Z=%d  zoom x%d%ls"),A.scene.title,A.scene.x,A.scene.y,A.scene.z,A.currentZ,A.zoom,A.scene.dirty?tr(L"   *modifiee*"):L"");}
    else if(A.map.cells){_snwprintf(s,PATH_CAP+159,tr(L"%ls   %dx%dx%d   Z=%d  zoom x%d%ls"),A.map.path[0]?A.map.path:tr(L"<nouvelle MAP>"),A.map.x,A.map.y,A.map.z,A.currentZ,A.zoom,A.map.dirty?tr(L"   *modifiee*"):L"");}
    else{_snwprintf(s,PATH_CAP+159,tr(L"Ouvrez ou creez une MAP. %ls"),A.activeCount?tr(L"Palette MAP prete."):tr(L"Les datasets seront charges automatiquement si un profil OXCE est trouve."));}
    RECT titleRect={A.sidebarW+12,8,A.clientW-A.inspectorW-12,30};DrawTextW(mem,s,-1,&titleRect,DT_SINGLELINE|DT_PATH_ELLIPSIS|DT_NOPREFIX);
    SetTextColor(mem,RGB(125,160,172));TextOutW(mem,A.sidebarW+12,A.clientH-23,A.status,(int)wcslen(A.status));
    RestoreDC(mem,clip);fill_rect_color(mem,A.sidebarW-3,0,A.sidebarW+3,A.clientH,RGB(72,96,104));fill_rect_color(mem,A.clientW-A.inspectorW-3,0,A.clientW-A.inspectorW+3,A.clientH,RGB(72,96,104));BitBlt(hdc,0,0,A.clientW,A.clientH,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bmp);DeleteDC(mem);
}

static int mouse_to_tile(int mx,int my,int*tx,int*ty){
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(dx<=0||dy<=0||mx<A.sidebarW||mx>=A.clientW-A.inspectorW)return 0;
    if(A.viewMode==1){int ox,oy,cs;plan_geometry(&ox,&oy,&cs);int x=(mx-ox)/cs,y=(my-oy)/cs;if(mx<ox||my<oy||x<0||y<0||x>=dx||y>=dy)return 0;*tx=x;*ty=y;return 1;}
    int ox,oy;world_origin(&ox,&oy);double ux=(double)(mx-ox)/A.zoom,uy=(double)(my-oy)/A.zoom;uy+=-16.0+(double)A.currentZ*24.0;
    double yf=(-ux+2.0*uy)/32.0;double xf=(uy-((-ux+2.0*uy)/4.0)-8.0)/8.0;int x=(int)xf,y=(int)yf;if(xf<0)x--;if(yf<0)y--;
    if(x<0||y<0||x>=dx||y>=dy)return 0;*tx=x;*ty=y;return 1;
}
static void apply_edit_value(int x,int y,int z,int layer,uint8_t val,int record){
    MapCell*c=cell_at(x,y,z);if(!c)return;uint8_t before=c->part[layer];if(before==val)return;if(record)push_edit(x,y,z,layer,before,val);c->part[layer]=val;A.map.dirty=1;A.overviewDirty=1;InvalidateRect(A.hwnd,NULL,FALSE);
}
static void apply_scene_edit_value(int x,int y,int z,int layer,int lib,int local,int record){
    SceneCell*c=scene_cell_at(x,y,z);if(!c)return;int bl=c->lib[layer],bo=c->local[layer];if(bl==lib&&bo==local)return;if(record)push_scene_edit(x,y,z,layer,bl,bo,lib,local);c->lib[layer]=lib;c->local[layer]=local;A.scene.dirty=1;InvalidateRect(A.hwnd,NULL,FALSE);
}
static int blueprint_place_at(int x,int y){
    if(!blueprint_current_matches_selection())return 0;int dxmax=A.scene.active?A.scene.x:A.map.x,dymax=A.scene.active?A.scene.y:A.map.y,dzmax=A.scene.active?A.scene.z:A.map.z;int libs[1024],raws[1024],count=0,anchor=-1;const wchar_t*bpname=tr(L"?");
    if(A.blueprintMode==1){const BlueprintDef*b=&BLUEPRINTS_RC15[A.blueprintIndex];count=b->partCount;anchor=blueprint_anchor_part(A.blueprintIndex);bpname=b->id;if(count>1024||anchor<0)return 0;const BlueprintPartDef*a=&BLUEPRINT_PARTS_RC15[b->firstPart+anchor];for(int j=0;j<count;j++){const BlueprintPartDef*p=&BLUEPRINT_PARTS_RC15[b->firstPart+j];int tx=x+(p->dx-a->dx),ty=y+(p->dy-a->dy),tz=A.currentZ+(p->dz-a->dz);if(tx<0||ty<0||tz<0||tx>=dxmax||ty>=dymax||tz>=dzmax){set_status(tr(L"Blueprint refuse : une piece sortirait de la MAP."));return 1;}int lib=blueprint_resolve_lib(p->dataset);if(lib<0||p->mcd<0||p->mcd>=A.library[lib].mcdCount){set_status(tr(L"Blueprint refuse : dataset/MCD introuvable dans la chaine de sources autorisee."));return 1;}libs[j]=lib;raws[j]=-1;}if(!A.scene.active)for(int j=0;j<count;j++){const BlueprintPartDef*p=&BLUEPRINT_PARTS_RC15[b->firstPart+j];raws[j]=raw_index_for_lib_local(libs[j],p->mcd,1);if(raws[j]<0)return 1;}A.currentEditGroup=++A.nextEditGroup;for(int j=0;j<count;j++){const BlueprintPartDef*p=&BLUEPRINT_PARTS_RC15[b->firstPart+j];int tx=x+(p->dx-a->dx),ty=y+(p->dy-a->dy),tz=A.currentZ+(p->dz-a->dz);if(A.scene.active)apply_scene_edit_value(tx,ty,tz,p->layer,libs[j],p->mcd,1);else apply_edit_value(tx,ty,tz,p->layer,(uint8_t)raws[j],1);}A.currentEditGroup=0;}
    else if(A.blueprintMode==2){CustomBlueprintDef*b=&A.customBlueprints[A.customBlueprintIndex];count=b->partCount;anchor=custom_blueprint_anchor_part(A.customBlueprintIndex);bpname=b->name;if(count>1024||anchor<0)return 0;CustomBlueprintPart*a=&A.customBlueprintParts[b->firstPart+anchor];for(int j=0;j<count;j++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+j];int tx=x+(p->dx-a->dx),ty=y+(p->dy-a->dy),tz=A.currentZ+(p->dz-a->dz);if(tx<0||ty<0||tz<0||tx>=dxmax||ty>=dymax||tz>=dzmax){set_status(tr(L"Blueprint perso refuse : une piece sortirait de la MAP."));return 1;}int lib=blueprint_resolve_lib(p->dataset);if(lib<0||p->mcd<0||p->mcd>=A.library[lib].mcdCount){set_status(tr(L"Blueprint perso refuse : dataset/MCD introuvable dans la chaine de sources autorisee."));return 1;}libs[j]=lib;raws[j]=-1;}if(!A.scene.active)for(int j=0;j<count;j++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+j];raws[j]=raw_index_for_lib_local(libs[j],p->mcd,1);if(raws[j]<0)return 1;}A.currentEditGroup=++A.nextEditGroup;for(int j=0;j<count;j++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+j];int tx=x+(p->dx-a->dx),ty=y+(p->dy-a->dy),tz=A.currentZ+(p->dz-a->dz);if(A.scene.active)apply_scene_edit_value(tx,ty,tz,p->layer,libs[j],p->mcd,1);else apply_edit_value(tx,ty,tz,p->layer,(uint8_t)raws[j],1);}A.currentEditGroup=0;}
    wchar_t st[300];_snwprintf(st,299,tr(L"Blueprint %ls pose : %d pieces (Undo/Redo groupe)."),bpname,count);set_status(st);A.overviewDirty=1;InvalidateRect(A.hwnd,NULL,FALSE);return 1;
}

static int erase_visible_piece_at(int mx,int my){
    int x=-1,y=-1,z=-1,layer=-1,lib=-1,local=-1;
    if(!hit_piece_at_levels(mx,my,&x,&y,&z,&layer,&lib,&local,A.currentZ,A.currentZ))return 0;
    if(A.scene.active)apply_scene_edit_value(x,y,z,layer,-1,-1,1);
    else apply_edit_value(x,y,z,layer,0,1);
    int tx=x,ty=y;mouse_to_tile(mx,my,&tx,&ty);A.lastPaintX=tx;A.lastPaintY=ty;
    wchar_t st[260];_snwprintf(st,259,tr(L"Gomme universelle : %ls/MCD%d efface (%ls X%d Y%d Z%d)."),(lib>=0&&lib<A.libraryCount)?A.library[lib].name:tr(L"?"),local,layer_name(layer),x,y,z);set_status(st);
    return 1;
}

static int brush_contains(int ix,int iy,int n,int shape){
    if(n<=1)return 1;if(shape==0)return 1;int cx2=n-1,cy2=n-1,dx=abs(ix*2-cx2),dy=abs(iy*2-cy2);if(shape==2)return dx+dy<=n;long long rr=(long long)n*n,dd=(long long)dx*dx+(long long)dy*dy;return dd<=rr;
}
static int brush_is_border(int ix,int iy,int n,int shape){if(!brush_contains(ix,iy,n,shape))return 0;if(n<=1)return 1;static const int d[4][2]={{-1,0},{1,0},{0,-1},{0,1}};for(int k=0;k<4;k++){int nx=ix+d[k][0],ny=iy+d[k][1];if(nx<0||ny<0||nx>=n||ny>=n||!brush_contains(nx,ny,n,shape))return 1;}return 0;}
static int normal_apply_one(int x,int y,int erase,int record){
    if(editor_locked(x,y,A.currentZ))return 0;
    if(!erase&&!E.replace){if(A.scene.active){SceneCell*c=scene_cell_at(x,y,A.currentZ);if(c&&c->lib[A.selectedLayer]>=0)return 0;}else{MapCell*c=cell_at(x,y,A.currentZ);if(c&&c->part[A.selectedLayer])return 0;}}
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(x<0||y<0||x>=dx||y>=dy)return 0;
    if(A.scene.active){int lib=-1,local=-1;if(!erase){if(A.selectedLib<0||A.selectedLib>=A.libraryCount||A.selectedLocal<0)return 0;lib=A.selectedLib;local=A.selectedLocal;}apply_scene_edit_value(x,y,A.currentZ,A.selectedLayer,lib,local,record);return 1;}
    int raw=erase?0:selected_raw_index(1);if(!erase&&raw<0)return 0;apply_edit_value(x,y,A.currentZ,A.selectedLayer,(uint8_t)raw,record);return 1;
}
static void normal_flood_fill(int sx,int sy,int erase){
    int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(sx<0||sy<0||sx>=dx||sy>=dy)return;int n=dx*dy,cap=n*4+8,*qx=(int*)malloc((size_t)cap*sizeof(int)),*qy=(int*)malloc((size_t)cap*sizeof(int));uint8_t*seen=(uint8_t*)calloc((size_t)n,1);if(!qx||!qy||!seen){free(qx);free(qy);free(seen);return;}int h=0,t=0;
    int sl=-1,so=-1;uint8_t sr=0;if(A.scene.active){SceneCell*c=scene_cell_at(sx,sy,A.currentZ);if(!c){free(qx);free(qy);free(seen);return;}sl=c->lib[A.selectedLayer];so=c->local[A.selectedLayer];}else{MapCell*c=cell_at(sx,sy,A.currentZ);if(!c){free(qx);free(qy);free(seen);return;}sr=c->part[A.selectedLayer];}
    qx[t]=sx;qy[t++]=sy;A.currentEditGroup=++A.nextEditGroup;while(h<t){int x=qx[h],y=qy[h++],id=y*dx+x;if(seen[id])continue;seen[id]=1;int same=0;if(A.scene.active){SceneCell*c=scene_cell_at(x,y,A.currentZ);same=c&&c->lib[A.selectedLayer]==sl&&c->local[A.selectedLayer]==so;}else{MapCell*c=cell_at(x,y,A.currentZ);same=c&&c->part[A.selectedLayer]==sr;}if(!same)continue;normal_apply_one(x,y,erase,1);if(x>0&&t<cap){qx[t]=x-1;qy[t++]=y;}if(x+1<dx&&t<cap){qx[t]=x+1;qy[t++]=y;}if(y>0&&t<cap){qx[t]=x;qy[t++]=y-1;}if(y+1<dy&&t<cap){qx[t]=x;qy[t++]=y+1;}}
    A.currentEditGroup=0;free(qx);free(qy);free(seen);
}
static int plan_context_has_diag(int diag){
    if(A.scene.active){size_t n=(size_t)A.scene.x*A.scene.y*A.scene.z;for(size_t i=0;i<n;i++)for(int p=0;p<4;p++){int lib=A.scene.cells[i].lib[p],loc=A.scene.cells[i].local[p];if(lib>=0&&loc>=0&&mcd_u8(lib,loc,53)==3&&mcd_bigwall_effective(lib,loc)==diag)return 1;}}
    else for(int a=0;a<A.activeCount;a++){int lib=A.active[a].libIndex;if(lib<0||lib>=A.libraryCount)continue;for(int loc=0;loc<A.library[lib].mcdCount;loc++)if(mcd_u8(lib,loc,53)==3&&mcd_bigwall_effective(lib,loc)==diag)return 1;}return 0;
}

static int plan_find_diag_candidate(int diag,int*outLib,int*outLocal){
    if(outLib)*outLib=-1;if(outLocal)*outLocal=-1;
    if(A.scene.active){uint8_t seen[MAX_LIBRARY];ZeroMemory(seen,sizeof(seen));size_t n=(size_t)A.scene.x*A.scene.y*A.scene.z;for(size_t i=0;i<n;i++)for(int p=0;p<4;p++){int lib=A.scene.cells[i].lib[p];if(lib<0||lib>=A.libraryCount||seen[lib])continue;seen[lib]=1;for(int loc=0;loc<A.library[lib].mcdCount;loc++)if(mcd_u8(lib,loc,53)==3&&mcd_bigwall_effective(lib,loc)==diag){if(outLib)*outLib=lib;if(outLocal)*outLocal=loc;return 1;}}}
    else for(int a=0;a<A.activeCount;a++){int lib=A.active[a].libIndex;if(lib<0||lib>=A.libraryCount)continue;for(int loc=0;loc<A.library[lib].mcdCount;loc++)if(mcd_u8(lib,loc,53)==3&&mcd_bigwall_effective(lib,loc)==diag){if(outLib)*outLib=lib;if(outLocal)*outLocal=loc;return 1;}}
    return 0;
}
static int plan_physical_editable(void){return A.scene.active||!A.map.path[0]||current_map_source()==SRC_MOD;}
static void plan_materialize_diag(int x,int y,int diag,int erase){
    if(!plan_physical_editable())return;int oldLib=-1,oldLocal=-1,has=plan_actual_layer(x,y,A.currentZ,3,&oldLib,&oldLocal),oldDiag=has?mcd_bigwall_effective(oldLib,oldLocal):0;
    if(erase){if(has&&(oldDiag==2||oldDiag==3)){if(A.scene.active)apply_scene_edit_value(x,y,A.currentZ,3,-1,-1,1);else apply_edit_value(x,y,A.currentZ,3,0,1);}return;}
    int lib=-1,local=-1;if(!plan_find_diag_candidate(diag,&lib,&local))return;
    if(has&&oldDiag!=2&&oldDiag!=3){set_status(tr(L"PLAN : mur diagonal logique pose, mais la couche OBJET de cette case est deja occupee ; aucune piece reelle n'a ete ecrasee."));return;}
    if(A.scene.active)apply_scene_edit_value(x,y,A.currentZ,3,lib,local,1);else{int raw=raw_index_for_lib_local(lib,local,1);if(raw>0)apply_edit_value(x,y,A.currentZ,3,(uint8_t)raw,1);}
}
static void plan_warn_missing_diag(int diag){
    int bit=diag==2?1:2;if(A.planDiagWarnMask&bit)return;if(plan_context_has_diag(diag))return;A.planDiagWarnMask|=bit;wchar_t msg[1600];_snwprintf(msg,1599,tr(L"Le mur diagonal %ls est conserve dans le plan, mais aucun objet MCD bigWall=%d compatible n'est present dans les datasets actuellement utilises.\n\nLe Workshop ne bloque pas l'edition. Il faudra ajouter une tuile compatible au profil/dataset avant export final.\n\nCandidats deja indexes :"),diag==2?tr(L"NE-SO"):tr(L"NO-SE"),diag);int found=0;for(int lib=0;lib<A.libraryCount&&found<10;lib++)for(int loc=0;loc<A.library[lib].mcdCount&&found<10;loc++)if(mcd_u8(lib,loc,53)==3&&mcd_bigwall_effective(lib,loc)==diag){wchar_t line[180];_snwprintf(line,179,tr(L"\n  - %ls / MCD %d [%ls]"),A.library[lib].name,loc,source_name(A.library[lib].source));if(wcslen(msg)+wcslen(line)<1550)wcscat(msg,line);found++;}if(!found)wcscat(msg,tr(L"\n  - aucun candidat indexe : creer une entree OBJECT bigWall correspondante."));MessageBoxW(A.hwnd,msg,tr(L"PLAN - tuile de mur diagonal manquante"),MB_OK|MB_ICONWARNING);
}
static int plan_shape_from_click(int mx,int my,int x,int y,int diag){
    if(diag!=2&&diag!=3)return PLAN_FSHAPE_FULL;if(A.viewMode==1){int ox,oy,cs;plan_geometry(&ox,&oy,&cs);int lx=clampi(mx-(ox+x*cs),0,cs),ly=clampi(my-(oy+y*cs),0,cs);if(diag==2)return (ly<cs-lx)?PLAN_FSHAPE_TRI_NO:PLAN_FSHAPE_TRI_SE;return (ly<lx)?PLAN_FSHAPE_TRI_NE:PLAN_FSHAPE_TRI_SO;}
    int cx,cy,rx,ry;plan_iso_points(x,y,A.currentZ,&cx,&cy,&rx,&ry);double dx=(double)(mx-cx)/(double)(rx?rx:1),dy=(double)(my-cy)/(double)(ry?ry:1);double u=(dx+dy+1.0)/2.0,v=(dy-dx+1.0)/2.0;if(diag==2)return (u+v<1.0)?PLAN_FSHAPE_TRI_NO:PLAN_FSHAPE_TRI_SE;return (u>v)?PLAN_FSHAPE_TRI_NE:PLAN_FSHAPE_TRI_SO;
}
static void plan_apply_semantic_one(int x,int y,int erase,int record,int shapeOverride){PlanCell*p=plan_cell_at(x,y,A.currentZ);if(!p||(p->flags&PLAN_FLAG_LOCKED))return;PlanCell n=*p;if(A.planSemanticLayer)n.objectSemantic=(uint8_t)(erase?PLAN_OBJ_AUTO:A.planObjectSemantic);else{n.semantic=(uint8_t)(erase?PLAN_AUTO:A.planSemantic);if(erase)n.floorShape=PLAN_FSHAPE_AUTO;else if(A.planFloorShape!=PLAN_FSHAPE_AUTO)n.floorShape=(uint8_t)A.planFloorShape;else if(shapeOverride>PLAN_FSHAPE_AUTO)n.floorShape=(uint8_t)shapeOverride;}plan_apply_cell(x,y,A.currentZ,n,record);}
static void plan_flood_fill(int sx,int sy,int erase){int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(sx<0||sy<0||sx>=dx||sy>=dy)return;int target=A.planSemanticLayer?plan_object_effective(sx,sy,A.currentZ):plan_floor_effective(sx,sy,A.currentZ),n=dx*dy,cap=n*4+8,*q=(int*)malloc((size_t)cap*sizeof(int));uint8_t*seen=(uint8_t*)calloc((size_t)n,1);if(!q||!seen){free(q);free(seen);return;}int h=0,t=0;q[t++]=sy*dx+sx;A.currentEditGroup=++A.nextEditGroup;while(h<t){int id=q[h++];if(id<0||id>=n||seen[id])continue;seen[id]=1;int x=id%dx,y=id/dx,v=A.planSemanticLayer?plan_object_effective(x,y,A.currentZ):plan_floor_effective(x,y,A.currentZ);if(v!=target)continue;plan_apply_semantic_one(x,y,erase,1,0);if(x>0&&t<cap)q[t++]=id-1;if(x+1<dx&&t<cap)q[t++]=id+1;if(y>0&&t<cap)q[t++]=id-dx;if(y+1<dy&&t<cap)q[t++]=id+dx;}A.currentEditGroup=0;free(q);free(seen);}

static void edit_at(int mx,int my,int erase){
    if(!A.scene.active&&current_map_is_protected()){int r=MessageBoxW(A.hwnd,tr(L"ATTENTION : ce macrobloc appartient a TFTD ORIGINAL ou OXCE STANDARD.\n\nIl est protege en lecture seule.\n\nCreer maintenant une copie editable dans un mod ?"),APP_TITLE,MB_YESNO|MB_ICONWARNING);if(r==IDYES)open_export_wizard();return;}
    if(erase&&A.selectedLib<0&&A.brushMode==0){erase_visible_piece_at(mx,my);return;}int x,y;if(!mouse_to_tile(mx,my,&x,&y))return;
    if(!erase&&A.brushMode==0&&blueprint_current_matches_selection()){if(blueprint_place_at(x,y))return;}
    if(A.brushMode==3){normal_flood_fill(x,y,erase);A.lastPaintX=x;A.lastPaintY=y;InvalidateRect(A.hwnd,NULL,FALSE);return;}
    int n=A.brushMode==0?1:clampi(A.brushSize,1,16),x0=x-(n-1)/2,y0=y-(n-1)/2;A.currentEditGroup=++A.nextEditGroup;for(int iy=0;iy<n;iy++)for(int ix=0;ix<n;ix++){int ok=A.brushMode==1?brush_is_border(ix,iy,n,A.brushShape):brush_contains(ix,iy,n,A.brushShape);if(ok)normal_apply_one(x0+ix,y0+iy,erase,1);}A.currentEditGroup=0;A.lastPaintX=x;A.lastPaintY=y;InvalidateRect(A.hwnd,NULL,FALSE);
}
static void plan_mark_dirty(void){if(A.scene.active)A.scene.dirty=1;else A.map.dirty=1;}
static void plan_apply_cell(int x,int y,int z,PlanCell v,int record){
    PlanCell*p=plan_cell_at(x,y,z);if(!p)return;if((p->flags&PLAN_FLAG_LOCKED)&&A.planTool!=5)return;PlanCell old=*p;if(plan_equal(old,v))return;if(record)push_plan_edit(x,y,z,old,v);*p=v;plan_mark_dirty();
}
static void plan_set_edge_one(int x,int y,int z,int edge,int val){
    PlanCell*p=plan_cell_at(x,y,z);if(!p||(p->flags&PLAN_FLAG_LOCKED))return;PlanCell n=*p;if(edge==1)n.west=(uint8_t)val;else if(edge==2)n.north=(uint8_t)val;else if(edge==3)n.east=(uint8_t)val;else n.south=(uint8_t)val;plan_apply_cell(x,y,z,n,1);
}
static void plan_set_edge_pair(int x,int y,int z,int edge,int val){
    A.currentEditGroup=++A.nextEditGroup;plan_set_edge_one(x,y,z,edge,val);
    if(edge==1)plan_set_edge_one(x-1,y,z,3,val);else if(edge==2)plan_set_edge_one(x,y-1,z,4,val);else if(edge==3)plan_set_edge_one(x+1,y,z,1,val);else plan_set_edge_one(x,y+1,z,2,val);A.currentEditGroup=0;
}
static long long plan_seg_dist2(int px,int py,int ax,int ay,int bx,int by){long long vx=bx-ax,vy=by-ay,wx=px-ax,wy=py-ay,c1=wx*vx+wy*vy;if(c1<=0){long long dx=px-ax,dy=py-ay;return dx*dx+dy*dy;}long long c2=vx*vx+vy*vy;if(c2<=c1){long long dx=px-bx,dy=py-by;return dx*dx+dy*dy;}double t=(double)c1/(double)c2;double qx=ax+t*vx,qy=ay+t*vy,dx=px-qx,dy=py-qy;return (long long)(dx*dx+dy*dy);}
static int plan_edge_from_mouse(int mx,int my,int x,int y){if(A.viewMode==2){int cx,cy,rx,ry;plan_iso_points(x,y,A.currentZ,&cx,&cy,&rx,&ry);long long d[4]={plan_seg_dist2(mx,my,cx,cy-ry,cx-rx,cy),plan_seg_dist2(mx,my,cx,cy-ry,cx+rx,cy),plan_seg_dist2(mx,my,cx+rx,cy,cx,cy+ry),plan_seg_dist2(mx,my,cx-rx,cy,cx,cy+ry)};int e=0;for(int i=1;i<4;i++)if(d[i]<d[e])e=i;return e+1;}int ox,oy,cs;plan_geometry(&ox,&oy,&cs);int lx=mx-(ox+x*cs),ly=my-(oy+y*cs);int d1=lx,d2=ly,d3=cs-lx,d4=cs-ly,edge=1,b=d1;if(d2<b){b=d2;edge=2;}if(d3<b){b=d3;edge=3;}if(d4<b){edge=4;}return edge;}
static void plan_edit_at(int mx,int my,int erase){
    int x,y;if(!mouse_to_tile(mx,my,&x,&y))return;if(A.planPasteMode&&!erase){plan_commit_paste(x,y);return;}PlanCell*p=plan_cell_at(x,y,A.currentZ);if(!p)return;if(A.planTool==4)return;
    if(A.planTool==5){PlanCell n=*p;n.flags^=PLAN_FLAG_LOCKED;A.currentEditGroup=++A.nextEditGroup;push_plan_edit(x,y,A.currentZ,*p,n);*p=n;A.currentEditGroup=0;plan_mark_dirty();set_status((n.flags&PLAN_FLAG_LOCKED)?tr(L"PLAN : case verrouillee."):tr(L"PLAN : case deverrouillee."));InvalidateRect(A.hwnd,NULL,FALSE);return;}
    if(A.planTool==0){if(A.brushMode==3){plan_flood_fill(x,y,erase);A.lastPaintX=x;A.lastPaintY=y;InvalidateRect(A.hwnd,NULL,FALSE);return;}int n=A.brushMode==0?1:clampi(A.brushSize,1,16),x0=x-(n-1)/2,y0=y-(n-1)/2;A.currentEditGroup=++A.nextEditGroup;for(int iy=0;iy<n;iy++)for(int ix=0;ix<n;ix++){int ok=A.brushMode==1?brush_is_border(ix,iy,n,A.brushShape):brush_contains(ix,iy,n,A.brushShape);if(!ok)continue;int tx=x0+ix,ty=y0+iy,sh=0;if(n==1&&!erase&&!A.planSemanticLayer&&A.planFloorShape==PLAN_FSHAPE_AUTO){int dg=plan_diag_effective(tx,ty,A.currentZ);if(dg)sh=plan_shape_from_click(mx,my,tx,ty,dg);}plan_apply_semantic_one(tx,ty,erase,1,sh);}A.currentEditGroup=0;A.lastPaintX=x;A.lastPaintY=y;InvalidateRect(A.hwnd,NULL,FALSE);return;}
    if(A.planTool==3){PlanCell n=*p;if(n.flags&PLAN_FLAG_LOCKED)return;n.vlink=(uint8_t)(erase?0:A.planLinkType);plan_apply_cell(x,y,A.currentZ,n,1);A.lastPaintX=x;A.lastPaintY=y;InvalidateRect(A.hwnd,NULL,FALSE);return;}
    if(A.planTool==1&&A.planWallMode){PlanCell n=*p;if(n.flags&PLAN_FLAG_LOCKED)return;if(erase)n.diag=(uint8_t)(plan_actual_diag(x,y,A.currentZ)?PLAN_DIAG_FORCE_NONE:0);else n.diag=(uint8_t)A.planWallMode;A.currentEditGroup=++A.nextEditGroup;plan_apply_cell(x,y,A.currentZ,n,1);plan_materialize_diag(x,y,A.planWallMode,erase);A.currentEditGroup=0;if(!erase)plan_warn_missing_diag(A.planWallMode);A.lastPaintX=x;A.lastPaintY=y;InvalidateRect(A.hwnd,NULL,FALSE);return;}
    int edge=plan_edge_from_mouse(mx,my,x,y);int val;if(erase)val=plan_actual_wall(x,y,A.currentZ,edge)?3:0;else val=A.planTool==2?2:1;plan_set_edge_pair(x,y,A.currentZ,edge,val);A.lastPaintX=x;A.lastPaintY=y;InvalidateRect(A.hwnd,NULL,FALSE);
}
static void plan_copy_selection(void){
    if(!plan_view_active())return;int x0,y0,x1,y1;if(A.planSelActive){x0=A.planSelX0<A.planSelX1?A.planSelX0:A.planSelX1;x1=A.planSelX0>A.planSelX1?A.planSelX0:A.planSelX1;y0=A.planSelY0<A.planSelY1?A.planSelY0:A.planSelY1;y1=A.planSelY0>A.planSelY1?A.planSelY0:A.planSelY1;}else if(A.hoverValid){x0=x1=A.hoverX;y0=y1=A.hoverY;}else{set_status(tr(L"PLAN : selectionnez une zone avant Ctrl+C."));return;}
    int w=x1-x0+1,h=y1-y0+1;PlanClipboardCell*nb=(PlanClipboardCell*)realloc(A.planClip,(size_t)w*h*sizeof(PlanClipboardCell));if(!nb){set_status(tr(L"PLAN : memoire insuffisante pour le presse-papiers."));return;}A.planClip=nb;A.planClipW=w;A.planClipH=h;
    for(int y=0;y<h;y++)for(int x=0;x<w;x++){PlanClipboardCell*q=&A.planClip[y*w+x];ZeroMemory(q,sizeof(*q));PlanCell*p=plan_cell_at(x0+x,y0+y,A.currentZ);if(p)q->plan=*p;for(int part=0;part<4;part++){q->lib[part]=q->local[part]=-1;if(A.scene.active){SceneCell*c=scene_cell_at(x0+x,y0+y,A.currentZ);if(c){q->lib[part]=c->lib[part];q->local[part]=c->local[part];}}else{MapCell*c=cell_at(x0+x,y0+y,A.currentZ);int lib,local;if(c&&active_resolve_raw(c->part[part],&lib,&local)){q->lib[part]=lib;q->local[part]=local;}}}}
    wchar_t st[200];_snwprintf(st,199,tr(L"PLAN : selection %dx%d copiee. Ctrl+V puis clic pour la poser."),w,h);set_status(st);
}
static void plan_begin_paste(void){if(!plan_view_active()||!A.planClip||A.planClipW<=0||A.planClipH<=0){set_status(tr(L"PLAN : presse-papiers vide."));return;}A.planPasteMode=1;set_status(tr(L"PLAN : collage fantome actif. Cliquez la case d'ancrage ; Echap annule."));InvalidateRect(A.hwnd,NULL,FALSE);}
static void plan_cancel_paste(void){if(!A.planPasteMode)return;A.planPasteMode=0;set_status(tr(L"PLAN : collage annule."));InvalidateRect(A.hwnd,NULL,FALSE);}
static void plan_commit_paste(int x0,int y0){
    if(!A.planPasteMode||!A.planClip)return;int dx=A.scene.active?A.scene.x:A.map.x,dy=A.scene.active?A.scene.y:A.map.y;if(x0<0||y0<0||x0+A.planClipW>dx||y0+A.planClipH>dy){set_status(tr(L"PLAN : collage refuse, la selection sortirait de la carte."));return;}
    if(!A.scene.active&&current_map_is_protected()){set_status(tr(L"PLAN : copie TFTD/OXCE protegee. Exportez d'abord vers un mod pour modifier les pieces."));return;}
    A.currentEditGroup=++A.nextEditGroup;
    for(int y=0;y<A.planClipH;y++)for(int x=0;x<A.planClipW;x++){int tx=x0+x,ty=y0+y;PlanCell*dst=plan_cell_at(tx,ty,A.currentZ);if(!dst||(dst->flags&PLAN_FLAG_LOCKED))continue;PlanClipboardCell*q=&A.planClip[y*A.planClipW+x];plan_apply_cell(tx,ty,A.currentZ,q->plan,1);for(int part=0;part<4;part++){if(A.scene.active)apply_scene_edit_value(tx,ty,A.currentZ,part,q->lib[part],q->local[part],1);else{int raw=0;if(q->lib[part]>=0&&q->local[part]>=0){raw=raw_index_for_lib_local(q->lib[part],q->local[part],1);if(raw<0)continue;}apply_edit_value(tx,ty,A.currentZ,part,(uint8_t)raw,1);}}}
    A.currentEditGroup=0;A.planPasteMode=0;A.planSelActive=1;A.planSelX0=x0;A.planSelY0=y0;A.planSelX1=x0+A.planClipW-1;A.planSelY1=y0+A.planClipH-1;set_status(tr(L"PLAN : collage termine (donnees logiques + contenu des 4 couches). Undo restaure l'ensemble."));InvalidateRect(A.hwnd,NULL,FALSE);
}
static void apply_history_action(EditAction*e,int redo){
    if(e->kind==1){PlanCell*p=plan_cell_at(e->x,e->y,e->z);if(p){*p=redo?e->planAfter:e->planBefore;if(A.scene.active)A.scene.dirty=1;else A.map.dirty=1;}return;}
    if(e->scene){if(!A.scene.active)return;SceneCell*c=scene_cell_at(e->x,e->y,e->z);if(c){c->lib[e->layer]=redo?e->afterLib:e->beforeLib;c->local[e->layer]=redo?e->afterLocal:e->beforeLocal;A.scene.dirty=1;}}
    else{if(!A.map.cells)return;MapCell*c=cell_at(e->x,e->y,e->z);if(c){c->part[e->layer]=redo?e->after:e->before;A.map.dirty=1;A.overviewDirty=1;}}
}
static void do_undo(void){editor_selection_clear();
    if(!A.scene.active&&current_map_is_protected())return;if(A.undoPos<=0)return;int group=A.undo[A.undoPos-1].group;do{EditAction*e=&A.undo[--A.undoPos];apply_history_action(e,0);}while(A.undoPos>0&&A.undo[A.undoPos-1].group==group);set_status(tr(L"Modification annulee."));InvalidateRect(A.hwnd,NULL,FALSE);
}
static void do_redo(void){editor_selection_clear();
    if(!A.scene.active&&current_map_is_protected())return;if(A.undoPos>=A.undoCount)return;int group=A.undo[A.undoPos].group;do{EditAction*e=&A.undo[A.undoPos++];apply_history_action(e,1);}while(A.undoPos<A.undoCount&&A.undo[A.undoPos].group==group);set_status(tr(L"Modification retablie."));InvalidateRect(A.hwnd,NULL,FALSE);
}
static void pipette_at(int mx,int my){
    int x,y;if(!mouse_to_tile(mx,my,&x,&y))return;
    if(A.scene.active){SceneCell*c=scene_cell_at(x,y,A.currentZ);if(!c||c->lib[A.selectedLayer]<0){set_status(tr(L"Pipette : case vide."));return;}A.selectedLib=c->lib[A.selectedLayer];A.selectedLocal=c->local[A.selectedLayer];blueprint_refresh_for_selection();A.browserMode=1;A.expandedLib=A.selectedLib;A.treeScroll=0;wchar_t st[220];_snwprintf(st,219,tr(L"Pipette scene : %ls / MCD %d (%ls)."),A.library[A.selectedLib].name,A.selectedLocal,layer_name(A.selectedLayer));set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);return;}
    MapCell*c=cell_at(x,y,A.currentZ);if(!c)return;int raw=c->part[A.selectedLayer],lib,local;if(!active_resolve_raw(raw,&lib,&local)){set_status(tr(L"Pipette : case vide ou indice non resolu."));return;}
    A.selectedLib=lib;A.selectedLocal=local;blueprint_refresh_for_selection();int ai=active_find_lib(lib);if(ai>=0)A.expandedActive=ai;A.browserMode=2;A.treeScroll=0;
    wchar_t st[220];_snwprintf(st,219,tr(L"Pipette : %ls / MCD %d (%ls)."),A.library[lib].name,local,layer_name(A.selectedLayer));set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);
}
static void select_library_tile(int lib,int local){
    if(lib<0||lib>=A.libraryCount||local<0||local>=A.library[lib].mcdCount)return;
    A.selectedLib=lib;A.selectedLocal=local;
    /* Selection d'une nouvelle piece => couche MCD native automatiquement.
       L'utilisateur peut ensuite cliquer FLOOR/WEST/NORTH/OBJECT pour la
       surcharger manuellement tant que cette meme piece reste selectionnee. */
    int nativeLayer=mcd_u8(lib,local,53);
    if(nativeLayer>=0&&nativeLayer<4)A.selectedLayer=nativeLayer;
    blueprint_refresh_for_selection();editor_palette_selected();
    InvalidateRect(A.hwnd,NULL,FALSE);
}

static int tree_grid_hit(int mx,int cy,int *y,int lib,int base,int showRaw){
    LibrarySet*s=&A.library[lib];int vc=visible_mcd_count(lib);int gh=grid_rows_for(vc)*GRID_CELL_H;if(cy>=*y&&cy<*y+gh){
        int avail=A.sidebarW-20,cw=avail/GRID_COLS;int relx=mx-10,rely=cy-*y;if(relx>=0&&relx<avail){int col=relx/cw,row=rely/GRID_CELL_H,vi=row*GRID_COLS+col;int k=visible_mcd_at(lib,vi);if(k>=0&&k<s->mcdCount){select_library_tile(lib,k);wchar_t st[320];int nl=mcd_u8(lib,k,53);if(showRaw)_snwprintf(st,319,tr(L"Selection : %ls / MCD %d / indice MAP %d / couche auto %ls."),s->name,k,base+k,layer_name(nl));else _snwprintf(st,319,tr(L"Selection : %ls / MCD %d / couche auto %ls."),s->name,k,layer_name(nl));set_status(st);return 1;}}}
    *y+=gh;return 0;
}
static int confirm_map_change(void){
    if(!proc_can_replace(A.hwnd))return 0;
    if(A.rmp.dirty){int rr=MessageBoxW(A.hwnd,tr(L"Le RMP du mod a ete modifie. L'enregistrer avant de changer de carte ?"),APP_TITLE,MB_ICONQUESTION|MB_YESNOCANCEL);if(rr==IDCANCEL)return 0;if(rr==IDYES&&!rmp_save_to_mod())return 0;A.rmp.dirty=0;}
    if(!A.map.cells||!A.map.dirty)return 1;int src=current_map_source();
    if(src==SRC_MOD){int r=MessageBoxW(A.hwnd,tr(L"La MAP du mod a ete modifiee. Sauvegarder avec backup automatique avant de changer de carte ?"),APP_TITLE,MB_ICONQUESTION|MB_YESNOCANCEL);if(r==IDCANCEL)return 0;if(r==IDYES)return safe_save_current_mod();return 1;}
    if(src==SRC_TFTD||src==SRC_OXCE){A.map.dirty=0;return 1;}
    int r=MessageBoxW(A.hwnd,tr(L"Cette nouvelle MAP n'a pas encore ete exportee vers un mod.\n\nOUI = abandonner cette MAP sans l'enregistrer\nNON = rester ici pour l'exporter vers un mod"),APP_TITLE,MB_YESNO|MB_ICONWARNING);return r==IDYES;
}
static void open_map_index(int idx){
    if(idx<0||idx>=A.mapCount)return;if(!confirm_map_change())return;A.selectedMap=idx;if(!load_map_file(A.maps[idx].path))MessageBoxW(A.hwnd,tr(L"MAP invalide ou non prise en charge."),APP_TITLE,MB_ICONERROR);
}
static int maps_tree_index_at(int mx,int my){
    (void)mx;if(my<TREE_TOP||my>=A.clientH-TREE_BOTTOM)return -1;ensure_panel_splits();int panel=my<A.panelSplit1?0:(my<A.panelSplit2?1:2);int top,bottom;panel_bounds(panel,&top,&bottom);int bodyTop=top+30;if(my<bodyTop||my>=bottom)return -1;int source=panel_source(panel),cy=my-bodyTop+A.sourceScroll[panel],y=0;wchar_t last[96]=L"";
    for(int i=0;i<A.mapCount;i++){MapEntry*m=&A.maps[i];if(m->source!=source||!map_visible(i))continue;if(source==SRC_MOD&&_wcsicmp(last,m->origin)!=0){y+=24;wcsncpy(last,m->origin,95);last[95]=0;}if(cy>=y&&cy<y+27)return i;y+=27;}return -1;
}
static void maps_tree_hit(int mx,int my){int i=maps_tree_index_at(mx,my);if(i>=0)open_map_index(i);}
static void maps_tree_context(int mx,int my){
    int i=maps_tree_index_at(mx,my);if(i<0)return;MapEntry*m=&A.maps[i];HMENU menu=CreatePopupMenu();if(!menu)return;
    AppendMenuW(menu,MF_STRING,7101,tr(L"Ouvrir cette MAP"));AppendMenuW(menu,MF_SEPARATOR,0,NULL);AppendMenuW(menu,MF_STRING,7102,tr(L"Copier tout le contenu dans le Hangar comme blueprint"));
    POINT pt={mx,my};ClientToScreen(A.hwnd,&pt);int cmd=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,pt.x,pt.y,0,A.hwnd,NULL);DestroyMenu(menu);
    if(cmd==7101)open_map_index(i);else if(cmd==7102){wchar_t q[420];_snwprintf(q,419,tr(L"Copier toute la MAP %ls (%dx%dx%d) dans le Hangar courant ?\n\nToutes les pieces, couches et niveaux Z seront conserves avec leurs positions relatives. La MAP actuellement ouverte ne sera pas fermee."),m->name,m->x,m->y,m->z);if(MessageBoxW(A.hwnd,q,APP_TITLE,MB_YESNO|MB_ICONQUESTION)==IDYES)blueprint_from_map_entry(i);}
}
static void tree_hit(int mx,int my){
    if(A.browserMode==0){maps_tree_hit(mx,my);return;}
    if(my<TREE_TOP||my>=A.clientH-TREE_BOTTOM)return;int cy=my-TREE_TOP+A.treeScroll,y=0;
    if(A.browserMode==1){
        for(int i=0;i<A.libraryCount;i++){if(!library_visible(i))continue;
            if(cy>=y&&cy<y+30){if(mx>A.sidebarW-78)active_add_lib(i,1);else{A.expandedLib=(A.expandedLib==i?-1:i);if(A.expandedLib==i)library_load(i);}InvalidateRect(A.hwnd,NULL,FALSE);return;}y+=30;
            if(i==A.expandedLib&&tree_grid_hit(mx,cy,&y,i,0,0))return;
        }
    }else{
        int showRaw=A.browserMode==2;
        for(int i=0;i<A.activeCount;i++){int lib=A.active[i].libIndex;if(!library_visible(lib))continue;
            if(cy>=y&&cy<y+32){A.expandedActive=(A.expandedActive==i?-1:i);if(A.expandedActive==i)library_load(lib);InvalidateRect(A.hwnd,NULL,FALSE);return;}y+=32;
            if(i==A.expandedActive&&tree_grid_hit(mx,cy,&y,lib,A.active[i].baseIndex,showRaw))return;
        }
    }
}
static void sidebar_click(int mx,int my){
    if(my>=102&&my<130){int tabw=(A.sidebarW-16)/3;int idx=(mx-8)/tabw;if(idx>=0&&idx<3){A.browserMode=idx;A.treeScroll=0;InvalidateRect(A.hwnd,NULL,FALSE);}return;}
    if(my>=168&&my<190&&A.browserMode!=0){int bw=(A.sidebarW-16)/5;int idx=(mx-8)/bw;if(idx>=0&&idx<5){A.resourceFilter=idx-1;if(A.resourceFilter>=0)A.selectedLayer=A.resourceFilter;A.treeScroll=0;A.expandedLib=A.expandedActive=-1;InvalidateRect(A.hwnd,NULL,FALSE);}return;}
    tree_hit(mx,my);
}

static void center_view(void){A.panX=A.panY=0;InvalidateRect(A.hwnd,NULL,FALSE);}static void set_zoom(int z){A.zoom=clampi(z,1,8);InvalidateRect(A.hwnd,NULL,FALSE);}static void set_z(int z){int dz=doc_zmax();if(dz>0){A.currentZ=clampi(z,0,dz-1);InvalidateRect(A.hwnd,NULL,FALSE);}}

static void do_select_root(int which){
    if(!confirm_map_change())return;
    wchar_t out[PATH_CAP];const wchar_t*initial=which==0?A.tftdRoot:(which==1?A.oxceRoot:A.modsRoot);
    const wchar_t*title=which==0?tr(L"TFTD ORIGINAL - selectionnez le dossier racine de Terror From The Deep"):(which==1?tr(L"OXCE STANDARD - selectionnez la racine OpenXcom/OXCE (les mods utilisateur seront ignores)"):tr(L"MODS OXCE - selectionnez le dossier user\\mods"));
    if(!browse_folder(A.hwnd,out,PATH_CAP,title,initial))return;
    if(which==0){wcsncpy(A.tftdRoot,out,PATH_CAP-1);A.tftdRoot[PATH_CAP-1]=0;}
    else if(which==1){wcsncpy(A.oxceRoot,out,PATH_CAP-1);A.oxceRoot[PATH_CAP-1]=0;}
    else{wcsncpy(A.modsRoot,out,PATH_CAP-1);A.modsRoot[PATH_CAP-1]=0;}
    config_save();reindex_resources();
}
static const wchar_t*render_mode_label(int mode){const wchar_t*names[]={L"Legacy",tr(L"PNG Remastered"),tr(L"Gabarits universels"),tr(L"REAL HD - apercu SAND/DEBRIS"),tr(L"REAL HD debug - geometrie")};return tr(names[clampi(mode,0,4)]);}
static void asset_render_mode_set(int mode){
    mode=clampi(mode,0,4);if(A.assetRenderMode!=mode)hd_cache_clear();A.assetRenderMode=mode;if(mode==1||mode==2)A.hdOverlayProvider=mode;A.overviewDirty=1;
    CheckMenuRadioItem(GetMenu(A.hwnd),12320,12321,A.hdOverlayProvider==2?12321:12320,MF_BYCOMMAND);
    int ids[5]={IDM_RENDER_LEGACY,IDM_RENDER_HD,IDM_RENDER_HD_CUSTOM,IDM_RENDER_REAL,IDM_RENDER_DEBUG};HMENU menu=GetMenu(A.hwnd);if(menu)for(int i=0;i<5;i++)CheckMenuItem(menu,ids[i],MF_BYCOMMAND|(i==mode?MF_CHECKED:MF_UNCHECKED));
    wchar_t st[512];_snwprintf(st,511,tr(L"%ls | F6 : changer de rendu. %ls"),render_mode_label(mode),mode>=3?tr(L"Apercu geometrie P2ZJ ; sans shaders, eclairage ni effets du jeu. Autres pieces : PNG puis Legacy."):tr(L"Un PNG absent utilise le sprite Legacy ; la carte logique reste identique."));set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);
}
static int do_select_hd_custom_root(void){
    wchar_t out[PATH_CAP];
    if(!browse_folder(A.hwnd,out,PATH_CAP,tr(L"MOD HD MANUEL - selectionnez la racine du mod de gabarit universel (ou Resources/TFTD_HD/Terrain)"),A.hdCustomRoot))return 0;
    wcsncpy(A.hdCustomRoot,out,PATH_CAP-1);A.hdCustomRoot[PATH_CAP-1]=0;config_save();hd_cache_clear();A.overviewDirty=1;
    asset_render_mode_set(2);set_status(tr(L"MOD HD manuel configure et active. Les PNG absents utilisent automatiquement le rendu Legacy."));
    return 1;
}

static void do_select_map_root(int which){
    if(!confirm_map_change())return;
    wchar_t out[PATH_CAP];const wchar_t*initial=which==0?A.tftdMapRoot:A.oxceMapRoot;const wchar_t*title=which==0?tr(L"Selectionnez le dossier contenant les MAP TFTD"):tr(L"Selectionnez le dossier MAPS OXCE / mods");
    if(!browse_folder(A.hwnd,out,PATH_CAP,title,initial))return;
    if(which==0){wcsncpy(A.tftdMapRoot,out,PATH_CAP-1);A.tftdMapRoot[PATH_CAP-1]=0;}else{wcsncpy(A.oxceMapRoot,out,PATH_CAP-1);A.oxceMapRoot[PATH_CAP-1]=0;}config_save();reindex_resources();A.browserMode=0;A.treeScroll=0;
}
static void do_open_map(void){if(!confirm_map_change())return;wchar_t p[PATH_CAP];if(open_file_dialog(A.hwnd,p,PATH_CAP,tr(L"MAP TFTD (*.MAP)\0*.MAP\0Tous les fichiers\0*.*\0\0"),tr(L"Ouvrir une MAP TFTD"))){if(!load_map_file(p))MessageBoxW(A.hwnd,tr(L"MAP invalide ou non prise en charge."),APP_TITLE,MB_ICONERROR);}}
static void do_manual_mcd(void){wchar_t p[PATH_CAP];if(open_file_dialog(A.hwnd,p,PATH_CAP,tr(L"MCD TFTD (*.MCD)\0*.MCD\0Tous les fichiers\0*.*\0\0"),tr(L"Ajouter un dataset MCD/PCK/TAB"))){int old=A.libraryCount;if(library_add_mcd(p,SRC_MANUAL,tr(L"MANUEL"))){int lib=A.libraryCount-1;active_add_lib(lib,1);A.expandedLib=lib;A.browserMode=1;}else if(A.libraryCount==old)MessageBoxW(A.hwnd,tr(L"Le MCD n'a pas pu etre ajoute (PCK/TAB manquant, doublon ou format invalide)."),APP_TITLE,MB_ICONWARNING);InvalidateRect(A.hwnd,NULL,FALSE);}}
static void do_load_palette(void){wchar_t p[PATH_CAP];if(open_file_dialog(A.hwnd,p,PATH_CAP,tr(L"Palette LBM (*.LBM)\0*.LBM\0Tous les fichiers\0*.*\0\0"),tr(L"Charger une palette TFTD"))){if(!load_lbm_palette(p))MessageBoxW(A.hwnd,tr(L"Bloc CMAP de palette introuvable."),APP_TITLE,MB_ICONERROR);else set_status(tr(L"Palette chargee manuellement."));}}

static void map_prefix(const wchar_t*name,wchar_t*out,size_t cap){name_family(name,out,cap);}
static int combo_has_text(HWND c,const wchar_t*txt){int n=(int)SendMessageW(c,CB_GETCOUNT,0,0);wchar_t b[128];for(int i=0;i<n;i++){SendMessageW(c,CB_GETLBTEXT,i,(LPARAM)b);if(_wcsicmp(b,txt)==0)return 1;}return 0;}
static void combo_map_label(int mi,wchar_t*out,int cap){MapEntry*m=&A.maps[mi];if(m->source==SRC_MOD)_snwprintf(out,cap-1,tr(L"[MOD: %ls] %ls"),m->origin,m->name);else _snwprintf(out,cap-1,tr(L"[%ls] %ls"),source_name(m->source),m->name);out[cap-1]=0;}
static void combo_add_map_by_kind(HWND c,int kind,const wchar_t*noneText){
    SendMessageW(c,CB_RESETCONTENT,0,0);if(noneText){int j=(int)SendMessageW(c,CB_ADDSTRING,0,(LPARAM)noneText);SendMessageW(c,CB_SETITEMDATA,j,(LPARAM)-1);}
    for(int i=0;i<A.mapCount;i++){int pi=A.maps[i].profileIndex;if(pi<0||pi>=A.profileCount||A.profiles[pi].kind!=kind)continue;wchar_t lab[260];combo_map_label(i,lab,260);int j=(int)SendMessageW(c,CB_ADDSTRING,0,(LPARAM)lab);SendMessageW(c,CB_SETITEMDATA,j,(LPARAM)i);}
    SendMessageW(c,CB_SETCURSEL,0,0);
}
static const wchar_t* composer_script_friendly(const wchar_t*n){
    if(!n)return tr(L"Recette inconnue");if(_wcsicmp(n,L"DEFAULT")==0)return tr(L"Terrain standard");if(_wcsicmp(n,L"THREEBIG")==0)return tr(L"Terrain avec 3 gros blocs");if(_wcsicmp(n,L"PLANE")==0)return tr(L"Avion");if(_wcsicmp(n,L"SHIP_P1")==0)return tr(L"Navire - partie 1");if(_wcsicmp(n,L"SHIP_P2")==0)return tr(L"Navire - partie 2");if(_wcsicmp(n,L"ARTIFACT_P1")==0)return tr(L"Site artefact - partie 1");if(_wcsicmp(n,L"ARTIFACT_P2")==0)return tr(L"Site artefact - partie 2");if(_wcsicmp(n,L"BASE_DRILL")==0)return tr(L"Base - raccords");if(_wcsicmp(n,L"ALIEN_COLONY_P1")==0)return tr(L"Colonie alien - partie 1");if(_wcsicmp(n,L"ALIEN_COLONY_P2")==0)return tr(L"Colonie alien - partie 2");if(_wcsicmp(n,L"PORT_TERROR")==0)return tr(L"Port - mission de terreur");if(_wcsicmp(n,L"ISLAND_TERROR")==0)return tr(L"Ile - mission de terreur");if(_wcsicmp(n,L"TLETH_P1")==0)return tr(L"T'leth - partie 1");if(_wcsicmp(n,L"TLETH_P2_P3")==0)return tr(L"T'leth - parties 2/3");if(_wcsicmp(n,L"STR_CRAFT_DEPLOYMENT_PREVIEW")==0)return tr(L"Apercu du vaisseau X-COM");return n;
}
static void composer_script_recommended_size(const OxcMapScriptDef*sc,int*outW,int*outH){
    int w=0,h=0;if(!sc){*outW=*outH=0;return;}const wchar_t*n=sc->name;
    if(_wcsicmp(n,L"PLANE")==0){w=h=50;}else if(_wcsicmp(n,L"SHIP_P1")==0||_wcsicmp(n,L"SHIP_P2")==0){w=30;h=70;}else if(_wcsicmp(n,L"ALIEN_COLONY_P1")==0||_wcsicmp(n,L"ALIEN_COLONY_P2")==0||_wcsicmp(n,L"TLETH_P1")==0||_wcsicmp(n,L"TLETH_P2_P3")==0){w=h=60;}else if(_wcsicmp(n,L"PORT_TERROR")==0||_wcsicmp(n,L"ISLAND_TERROR")==0){w=h=50;}
    if(!w||!h){int maxx=0,maxy=0,seenPlacement=0;for(int i=0;i<sc->commandCount;i++){OxcCommandDef*c=&gOxcCommands[sc->firstCommand+i];if(c->type==OXC_CMD_RESIZE&&!seenPlacement){if(c->sizeX>0)w=c->sizeX*10;if(c->sizeY>0)h=c->sizeY*10;}else if(c->type!=OXC_CMD_RESIZE)seenPlacement=1;for(int r=0;r<c->rectCount;r++){int ex=c->rects[r][0]+c->rects[r][2],ey=c->rects[r][1]+c->rects[r][3];if(ex>maxx)maxx=ex;if(ey>maxy)maxy=ey;}}if(!w&&maxx>0)w=maxx*10;if(!h&&maxy>0)h=maxy*10;}
    *outW=w;*outH=h;
}
static int composerPresetMap[4096];
static const wchar_t* composer_family_label(const wchar_t*name){
    if(!_wcsicmp(name,L"LINERT"))return tr(L"Bateau de croisiere - partie 1 (LINERT)");
    if(!_wcsicmp(name,L"LINERB"))return tr(L"Bateau de croisiere - partie 2 (LINERB)");
    if(!_wcsicmp(name,L"CARGO"))return tr(L"Cargo (CARGO)");
    return name;
}
static void composer_apply_preset(void){
    int ss=(int)SendMessageW(CUI.scriptCombo,CB_GETCURSEL,0,0);
    if(ss<0||ss>=4096||composerPresetMap[ss]<0)return;
    int mi=composerPresetMap[ss],n=(int)SendMessageW(CUI.terrainCombo,CB_GETCOUNT,0,0);
    wchar_t pref[96];map_prefix(A.maps[mi].name,pref,96);
    for(int j=0;j<n;j++){int cm=(int)SendMessageW(CUI.terrainCombo,CB_GETITEMDATA,j,0);if(cm<0||cm>=A.mapCount)continue;wchar_t cp[96];map_prefix(A.maps[cm].name,cp,96);
        if(!_wcsicmp(pref,cp)&&A.maps[cm].source==A.maps[mi].source&&!_wcsicmp(A.maps[cm].origin,A.maps[mi].origin)){SendMessageW(CUI.terrainCombo,CB_SETCURSEL,j,0);return;}}
}
static int composer_add_ship_choices(int si){
    OxcMapScriptDef*sc=&gOxcScripts[si];int phase=!_wcsicmp(sc->name,L"SHIP_P1")?1:(!_wcsicmp(sc->name,L"SHIP_P2")?2:0);if(!phase)return 0;int added=0;
    const wchar_t*families[2]={phase==1?L"LINERT":L"LINERB",L"CARGO"};
    for(int f=0;f<2;f++){int best=-1,score=-999999;
        for(int mi=0;mi<A.mapCount;mi++){wchar_t pref[96];map_prefix(A.maps[mi].name,pref,96);if(_wcsicmp(pref,families[f]))continue;int pi=A.maps[mi].profileIndex;if(pi<0||A.profiles[pi].kind!=0)continue;int rank=exact_source_score(A.maps[mi].source,A.maps[mi].origin,sc->source,sc->origin);if(rank>score){score=rank;best=mi;}}
        if(best<0)continue;wchar_t label[360];_snwprintf(label,359,tr(L"%ls - partie %d [%ls%ls%ls]"),f==0?tr(L"Bateau de croisiere"):L"Cargo",phase,source_name(sc->source),sc->source==SRC_MOD?L" : ":L"",sc->source==SRC_MOD?sc->origin:L"");
        int j=(int)SendMessageW(CUI.scriptCombo,CB_ADDSTRING,0,(LPARAM)label);SendMessageW(CUI.scriptCombo,CB_SETITEMDATA,j,si);if(j>=0&&j<4096)composerPresetMap[j]=best;added++;
    }return added;
}
static int composer_selected_script_index(void){int ss=CUI.scriptCombo?(int)SendMessageW(CUI.scriptCombo,CB_GETCURSEL,0,0):-1;return ss>=0?(int)SendMessageW(CUI.scriptCombo,CB_GETITEMDATA,ss,0):-1;}
static void composer_update_info(int applySize){
    if(!CUI.hwnd||!CUI.infoText)return;int si=composer_selected_script_index();wchar_t txt[1200];wcscpy(txt,tr(L"Aucune recette indexee. Verifiez les dossiers dans Ressources."));
    int preset=(int)SendMessageW(CUI.scriptCombo,CB_GETCURSEL,0,0),mode=(int)SendMessageW(CUI.modeCombo,CB_GETCURSEL,0,0);
    if(si>=0&&si<gOxcScriptCount){OxcMapScriptDef*sc=&gOxcScripts[si];int rw=0,rh=0;composer_script_recommended_size(sc,&rw,&rh);wchar_t why[300];int ok=exact_script_supported(sc,why,300);
        if(applySize&&rw>0&&rh>0){wchar_t b[32];_snwprintf(b,31,L"%d",rw);SetWindowTextW(CUI.widthEdit,b);_snwprintf(b,31,L"%d",rh);SetWindowTextW(CUI.lengthEdit,b);}
        if(applySize&&ok){composer_apply_preset();if(preset<0||preset>=4096||composerPresetMap[preset]<0)composer_auto_select_compatible_terrain(sc);}
        if(ok){int row=(int)SendMessageW(CUI.terrainCombo,CB_GETCURSEL,0,0),mi=row>=0?(int)SendMessageW(CUI.terrainCombo,CB_GETITEMDATA,row,0):-1;
            if(mi>=0&&mi<A.mapCount){wchar_t pref[96],compat[300];map_prefix(A.maps[mi].name,pref,96);int ti=exact_terrain_for_family(pref,A.maps[mi].source,A.maps[mi].origin);int available=exact_script_terrain_compatible(sc,ti,compat,300);
                _snwprintf(txt,1199,tr(L"Terrain : %ls.\r\n%ls\r\n%ls"),composer_family_label(pref),mode==0?tr(L"Recette officielle : les etapes sans bloc optionnel peuvent etre sautees."):tr(L"Assemblage libre : choisissez le terrain et les dimensions."),available?tr(L"Blocs disponibles. La carte sera controlee a la generation."):compat);
            }else wcscpy(txt,tr(L"Aucun terrain disponible. Verifiez les dossiers de ressources."));
        }else _snwprintf(txt,1199,tr(L"Recette non prise en charge : %ls"),why);
    }
    EnableWindow(CUI.terrainCombo,mode!=0||preset<0||preset>=4096||composerPresetMap[preset]<0);
    SendMessageW(CUI.scriptCombo,CB_SETDROPPEDWIDTH,540,0);SendMessageW(CUI.terrainCombo,CB_SETDROPPEDWIDTH,540,0);
    SetWindowTextW(CUI.infoText,txt);
}
static void populate_composer_combos(void){
    if(!CUI.hwnd)return;
    SendMessageW(CUI.modeCombo,CB_RESETCONTENT,0,0);SendMessageW(CUI.modeCombo,CB_ADDSTRING,0,(LPARAM)tr(L"FIDELE OPENXCOM - recette officielle"));SendMessageW(CUI.modeCombo,CB_ADDSTRING,0,(LPARAM)tr(L"LIBRE - assemblage par famille"));SendMessageW(CUI.modeCombo,CB_SETCURSEL,0,0);
    for(int i=0;i<4096;i++)composerPresetMap[i]=-1;SendMessageW(CUI.scriptCombo,CB_RESETCONTENT,0,0);int defaultSel=-1;for(int i=0;i<gOxcScriptCount;i++){OxcMapScriptDef*sc=&gOxcScripts[i];if(composer_add_ship_choices(i))continue;wchar_t lab[360],why[300];int ok=exact_script_supported(sc,why,300);if(sc->source==SRC_MOD)_snwprintf(lab,359,tr(L"[MOD: %ls] %ls%ls"),sc->origin,composer_script_friendly(sc->name),ok?L"":tr(L"  [non pris en charge]"));else _snwprintf(lab,359,tr(L"[%ls] %ls%ls"),source_name(sc->source),composer_script_friendly(sc->name),ok?L"":tr(L"  [non pris en charge]"));int j=(int)SendMessageW(CUI.scriptCombo,CB_ADDSTRING,0,(LPARAM)lab);SendMessageW(CUI.scriptCombo,CB_SETITEMDATA,j,(LPARAM)i);if(defaultSel<0&&_wcsicmp(sc->name,L"DEFAULT")==0&&sc->source==SRC_OXCE)defaultSel=j;}if(gOxcScriptCount>0)SendMessageW(CUI.scriptCombo,CB_SETCURSEL,defaultSel>=0?defaultSel:0,0);
    SendMessageW(CUI.terrainCombo,CB_RESETCONTENT,0,0);
    for(int i=0;i<A.mapCount;i++){int pi=A.maps[i].profileIndex;if(pi<0||pi>=A.profileCount||A.profiles[pi].kind!=0)continue;wchar_t pref[96],lab[260];map_prefix(A.maps[i].name,pref,96);if(!pref[0])continue;if(A.maps[i].source==SRC_MOD)_snwprintf(lab,259,tr(L"[MOD: %ls] %ls"),A.maps[i].origin,composer_family_label(pref));else _snwprintf(lab,259,tr(L"[%ls] %ls"),source_name(A.maps[i].source),composer_family_label(pref));if(combo_has_text(CUI.terrainCombo,lab))continue;int j=(int)SendMessageW(CUI.terrainCombo,CB_ADDSTRING,0,(LPARAM)lab);SendMessageW(CUI.terrainCombo,CB_SETITEMDATA,j,(LPARAM)i);}
    SendMessageW(CUI.terrainCombo,CB_SETCURSEL,0,0);SetWindowTextW(CUI.widthEdit,L"60");SetWindowTextW(CUI.lengthEdit,L"60");
    combo_add_map_by_kind(CUI.craftCombo,1,tr(L"(aucun vaisseau X-COM)"));
    SendMessageW(CUI.usoModeCombo,CB_RESETCONTENT,0,0);SendMessageW(CUI.usoModeCombo,CB_ADDSTRING,0,(LPARAM)tr(L"Aucun USO"));SendMessageW(CUI.usoModeCombo,CB_ADDSTRING,0,(LPARAM)tr(L"USO reel"));SendMessageW(CUI.usoModeCombo,CB_ADDSTRING,0,(LPARAM)tr(L"Emplacement reserve / fantome"));SendMessageW(CUI.usoModeCombo,CB_SETCURSEL,0,0);
    combo_add_map_by_kind(CUI.usoCombo,2,tr(L"(choisir un USO)"));composer_update_info(1);
}
static int edit_get_int(HWND h,int def){wchar_t b[64];GetWindowTextW(h,b,64);if(!b[0])return def;return _wtoi(b);}
static uint32_t rng_next(uint32_t*s){*s=*s*1664525u+1013904223u;return *s;}
static int map_context_priority(int mi,const wchar_t*pref,int source,const wchar_t*origin){
    if(mi<0||mi>=A.mapCount)return -1;MapEntry*m=&A.maps[mi];int pi=m->profileIndex;if(pi<0||pi>=A.profileCount||A.profiles[pi].kind!=0)return -1;
    wchar_t p[96];map_prefix(m->name,p,96);if(_wcsicmp(p,pref)!=0)return -1;
    if(source==SRC_MOD){
        if(m->source==SRC_MOD&&origin&&origin[0]&&_wcsicmp(m->origin,origin)==0)return 3000;
        if(m->source==SRC_OXCE)return 2000;
        if(m->source==SRC_TFTD)return 1000;
        return -1;
    }
    if(source==SRC_OXCE){if(m->source==SRC_OXCE)return 2000;if(m->source==SRC_TFTD)return 1000;return -1;}
    if(source==SRC_TFTD)return m->source==SRC_TFTD?1000:-1;
    return 0;
}
static int collect_effective_terrain_maps(const wchar_t*pref,int source,const wchar_t*origin,int maxSize,int*out,int outCap){
    int n=0;
    for(int i=0;i<A.mapCount;i++){
        int pr=map_context_priority(i,pref,source,origin);if(pr<0)continue;MapEntry*m=&A.maps[i];
        if(m->x%10||m->y%10||m->x>maxSize||m->y>maxSize)continue;
        int existing=-1;for(int k=0;k<n;k++)if(_wcsicmp(A.maps[out[k]].name,m->name)==0){existing=k;break;}
        if(existing<0){if(n<outCap)out[n++]=i;}
        else{int oldpr=map_context_priority(out[existing],pref,source,origin);if(pr>oldpr)out[existing]=i;}
    }
    return n;
}
static int exact_source_score(int candSource,const wchar_t*candOrigin,int source,const wchar_t*origin){
    if(!source_allowed_for_ctx(candSource,candOrigin,source,origin))return -999999;if(source==SRC_MOD){if(candSource==SRC_MOD&&origin&&_wcsicmp(candOrigin,origin)==0)return 3000;if(candSource==SRC_OXCE)return 2000;if(candSource==SRC_TFTD)return 1000;}if(source==SRC_OXCE){if(candSource==SRC_OXCE)return 2000;if(candSource==SRC_TFTD)return 1000;}if(source==SRC_TFTD&&candSource==SRC_TFTD)return 1000;return 0;
}
static int exact_map_by_name(const wchar_t*name,int source,const wchar_t*origin){int best=-1,score=-999999;for(int i=0;i<A.mapCount;i++){MapEntry*m=&A.maps[i];if(_wcsicmp(m->name,name)!=0)continue;int sc=exact_source_score(m->source,m->origin,source,origin);if(sc>score){best=i;score=sc;}}return best;}
static int exact_block_group(const OxcBlockDef*b,int group){if(!b)return 0;if(b->groupCount==0)return group==0;for(int i=0;i<b->groupCount;i++)if(b->groups[i]==group)return 1;return 0;}
static int exact_terrain_for_family(const wchar_t*pref,int source,const wchar_t*origin){int best=-1,score=-999999;for(int ti=0;ti<gOxcTerrainCount;ti++){OxcTerrainDef*t=&gOxcTerrains[ti];int sc=exact_source_score(t->source,t->origin,source,origin);if(sc<-10000)continue;int match=_wcsicmp(t->name,pref)==0?500:0;for(int b=0;b<t->blockCount&&!match;b++){wchar_t f[96];name_family(gOxcBlocks[t->firstBlock+b].name,f,96);if(_wcsicmp(f,pref)==0)match=350;}if(!match)continue;sc+=match;if(sc>score){best=ti;score=sc;}}return best;}
static int exact_terrain_by_name_ctx(const wchar_t*name,int source,const wchar_t*origin){int best=-1,score=-999999;for(int i=0;i<gOxcTerrainCount;i++){OxcTerrainDef*t=&gOxcTerrains[i];if(_wcsicmp(t->name,name)!=0)continue;int sc=exact_source_score(t->source,t->origin,source,origin);if(sc>score){best=i;score=sc;}}return best;}
static int exact_selector_has_candidate(const OxcCommandDef*c,int terrainIndex,int selector){
    if(!c||terrainIndex<0||terrainIndex>=gOxcTerrainCount)return 0;
    OxcTerrainDef*t=&gOxcTerrains[terrainIndex];
    if(c->blockCount){
        if(selector<0||selector>=c->blockCount)return 0;
        int ix=c->blocks[selector];if(ix<0||ix>=t->blockCount)return 0;
        OxcBlockDef*b=&gOxcBlocks[t->firstBlock+ix];
        return exact_map_by_name(b->name,t->source,t->origin)>=0;
    }
    int group=(c->groupCount&&selector>=0&&selector<c->groupCount)?c->groups[selector]:0;
    for(int i=0;i<t->blockCount;i++){
        OxcBlockDef*b=&gOxcBlocks[t->firstBlock+i];
        if(b->width!=c->sizeX*10||b->length!=c->sizeY*10)continue;
        if(!exact_block_group(b,group))continue;
        if(exact_map_by_name(b->name,t->source,t->origin)>=0)return 1;
    }
    return 0;
}
static int exact_script_terrain_compatible(const OxcMapScriptDef*sc,int terrainIndex,wchar_t*why,int cap){
    if(why&&cap>0)why[0]=0;
    if(!sc||terrainIndex<0||terrainIndex>=gOxcTerrainCount){if(why)_snwprintf(why,cap-1,tr(L"terrain non indexe"));return 0;}
    int anyCandidate=0,blockCommands=0;OxcTerrainDef*base=&gOxcTerrains[terrainIndex];
    for(int i=0;i<sc->commandCount;i++){
        OxcCommandDef*c=&gOxcCommands[sc->firstCommand+i];
        if(c->type!=OXC_CMD_ADDBLOCK&&c->type!=OXC_CMD_FILLAREA)continue;
        blockCommands++;int ti=terrainIndex;
        if(c->terrain[0])ti=exact_terrain_by_name_ctx(c->terrain,base->source,base->origin);
        if(ti<0){if(why)_snwprintf(why,cap-1,tr(L"commande %d : terrain impose %ls introuvable"),i+1,c->terrain);return 0;}
        int count=c->blockCount?c->blockCount:(c->groupCount?c->groupCount:1);
        for(int k=0;k<count;k++){
            int weight=(k<c->freqCount&&c->freqs[k]>=0)?c->freqs[k]:1;
            int maxUse=k<c->maxUsesCount?c->maxUses[k]:-1;
            if(weight<=0||maxUse==0)continue;
            if(exact_selector_has_candidate(c,ti,k)){anyCandidate=1;continue;}
            if(!c->canBeSkipped){
                if(why){
                    if(c->blockCount)_snwprintf(why,cap-1,tr(L"commande %d : bloc impose #%d absent de ce terrain"),i+1,c->blocks[k]);
                    else if(c->groupCount)_snwprintf(why,cap-1,tr(L"commande %d : groupe %d sans bloc %dx%d exploitable"),i+1,c->groups[k],c->sizeX*10,c->sizeY*10);
                    else _snwprintf(why,cap-1,tr(L"commande %d : aucun bloc standard %dx%d exploitable"),i+1,c->sizeX*10,c->sizeY*10);
                }
                return 0;
            }
        }
    }
    if(blockCommands&&!anyCandidate){if(why)_snwprintf(why,cap-1,tr(L"aucun bloc disponible pour cette recette"));return 0;}
    return 1;
}
static int exact_candidate_coverage(const OxcMapScriptDef*sc,int terrainIndex){
    int total=0,found=0;OxcTerrainDef*t=&gOxcTerrains[terrainIndex];
    for(int i=0;i<sc->commandCount;i++){OxcCommandDef*c=&gOxcCommands[sc->firstCommand+i];if(c->type!=OXC_CMD_ADDBLOCK&&c->type!=OXC_CMD_FILLAREA)continue;int ti=c->terrain[0]?exact_terrain_by_name_ctx(c->terrain,t->source,t->origin):terrainIndex;int n=c->blockCount?c->blockCount:(c->groupCount?c->groupCount:1);
        for(int k=0;k<n;k++){if((k<c->freqCount&&c->freqs[k]==0)||(k<c->maxUsesCount&&c->maxUses[k]==0))continue;total++;if(exact_selector_has_candidate(c,ti,k))found++;}}
    return total?100000*found/total:100000;
}
static int composer_auto_select_compatible_terrain(const OxcMapScriptDef*sc){
    if(!CUI.terrainCombo||!sc)return -1;int cur=(int)SendMessageW(CUI.terrainCombo,CB_GETCURSEL,0,0);
    int n=(int)SendMessageW(CUI.terrainCombo,CB_GETCOUNT,0,0),bestSel=-1,bestTi=-1,bestScore=-999999;
    for(int sel=0;sel<n;sel++){
        int mi=(int)SendMessageW(CUI.terrainCombo,CB_GETITEMDATA,sel,0);if(mi<0||mi>=A.mapCount)continue;
        wchar_t pref[96],why[256];map_prefix(A.maps[mi].name,pref,96);int ti=exact_terrain_for_family(pref,A.maps[mi].source,A.maps[mi].origin);
        if(ti<0||!exact_script_terrain_compatible(sc,ti,why,256))continue;OxcTerrainDef*t=&gOxcTerrains[ti];int score=exact_source_score(t->source,t->origin,sc->source,sc->origin);if(score<-10000)continue;
        score+=exact_candidate_coverage(sc,ti)+(sel==cur?1:0);if(score>bestScore){bestScore=score;bestSel=sel;bestTi=ti;}}
    if(bestSel>=0)SendMessageW(CUI.terrainCombo,CB_SETCURSEL,bestSel,0);return bestTi;
}
typedef struct {int blockDef,mapIndex,x,y,w,h,active;} ExactPlacement;
static int exact_selector_weight(const OxcCommandDef*c,int k){return (k<c->freqCount&&c->freqs[k]>=0)?c->freqs[k]:1;}
static int exact_selector_max(const OxcCommandDef*c,int k){return k<c->maxUsesCount?c->maxUses[k]:-1;}
static int exact_pick_block(const OxcCommandDef*c,int terrainIndex,int*uses,uint32_t*seed){
    if(terrainIndex<0||terrainIndex>=gOxcTerrainCount)return -1;OxcTerrainDef*t=&gOxcTerrains[terrainIndex];int count=c->blockCount?c->blockCount:(c->groupCount?c->groupCount:1),total=0;
    for(int k=0;k<count;k++){int mx=exact_selector_max(c,k);if(mx>=0&&uses[k]>=mx)continue;int w=exact_selector_weight(c,k);if(w>0)total+=w;}if(total<=0)return -1;int pick=(int)(rng_next(seed)%(uint32_t)total),sel=-1;
    for(int k=0;k<count;k++){int mx=exact_selector_max(c,k);if(mx>=0&&uses[k]>=mx)continue;int w=exact_selector_weight(c,k);if(w<=0)continue;if(pick<w){sel=k;break;}pick-=w;}if(sel<0)return -1;uses[sel]++;
    if(c->blockCount){int ix=c->blocks[sel];if(ix<0||ix>=t->blockCount)return -1;return t->firstBlock+ix;}
    int group=c->groupCount?c->groups[sel]:0,n=0;int*candidates=(int*)malloc((size_t)(t->blockCount>0?t->blockCount:1)*sizeof(int));if(!candidates)return -1;
    for(int i=0;i<t->blockCount;i++){OxcBlockDef*b=&gOxcBlocks[t->firstBlock+i];if(b->width!=c->sizeX*10||b->length!=c->sizeY*10)continue;if(exact_block_group(b,group))candidates[n++]=t->firstBlock+i;}
    if(!n){free(candidates);return -1;}int result=candidates[rng_next(seed)%(uint32_t)n];free(candidates);return result;
}

static int macro_snap_position(int p,int mapDim,int itemDim,uint32_t*seed){
    int maxp=mapDim-itemDim;if(maxp<0)return 0;int slots=maxp/10+1;
    if(p<0)return slots>0?(int)(rng_next(seed)%(uint32_t)slots)*10:0;
    p=clampi(p,0,maxp);return (p/10)*10;
}
static void scene_macro_mark_rect(int x,int y,int w,int h,uint8_t flag){
    if(!A.scene.active||!A.scene.macroFlags||w<=0||h<=0)return;
    int x0=clampi(x/10,0,A.scene.macroW-1),y0=clampi(y/10,0,A.scene.macroH-1);
    int x1=clampi((x+w-1)/10,0,A.scene.macroW-1),y1=clampi((y+h-1)/10,0,A.scene.macroH-1);
    for(int my=y0;my<=y1;my++)for(int mx=x0;mx<=x1;mx++)A.scene.macroFlags[my*A.scene.macroW+mx]|=flag;
}
static void scene_macro_mark_underlay_cell(int mx,int my){
    if(A.scene.active&&A.scene.macroFlags&&mx>=0&&my>=0&&mx<A.scene.macroW&&my<A.scene.macroH)A.scene.macroFlags[my*A.scene.macroW+mx]|=SCENE_MACRO_UNDERLAY;
}

static int exact_place_reserved_underlay(const OxcCommandDef*c,int terrainIndex,int x,int y,int w,int h,int*owner,int gw,int gh,ExactPlacement*pl,int*pc,uint32_t*seed,uint8_t reserveFlag){
    if(terrainIndex<0||terrainIndex>=gOxcTerrainCount||w<=0||h<=0||!owner||!pl||!pc)return 0;
    int uses[MAX_OXC_SELECT]={0},x0=x/10,y0=y/10,x1=(x+w-1)/10,y1=(y+h-1)/10,placed=0;
    OxcTerrainDef*t=&gOxcTerrains[terrainIndex];
    scene_macro_mark_rect(x,y,w,h,reserveFlag);
    /* Parite OXCE BattlescapeGenerator::addCraft():
       la reservation du craft/USO reussit meme si getNextBlock() ne renvoie aucun
       sous-sol. Chaque macro-case recoit un bloc de groupe 1 seulement s'il existe;
       les autres restent libres pour les commandes MapScript suivantes. */
    for(int my=y0;my<=y1;my++)for(int mx=x0;mx<=x1;mx++){
        if(mx<0||my<0||mx>=gw||my>=gh||owner[my*gw+mx]!=0)continue;
        int bd=exact_pick_block(c,terrainIndex,uses,seed);if(bd<0)continue;
        OxcBlockDef*b=&gOxcBlocks[bd];if(b->width!=10||b->length!=10)continue;
        int mi=exact_map_by_name(b->name,t->source,t->origin);if(mi<0||*pc>=gw*gh)continue;
        int id=*pc+1;owner[my*gw+mx]=id;pl[*pc]=(ExactPlacement){bd,mi,mx,my,1,1,1};(*pc)++;
        if(paste_map_to_scene(mi,mx*10,my*10,0)){scene_macro_mark_underlay_cell(mx,my);placed++;}
        else{(*pc)--;owner[my*gw+mx]=0;ZeroMemory(&pl[*pc],sizeof(pl[*pc]));}
    }
    return placed;
}
static int free_fill_reserved_underlay(int terrainIndex,int x,int y,int w,int h,uint32_t*seed,uint8_t reserveFlag){
    if(terrainIndex<0||terrainIndex>=gOxcTerrainCount)return 0;scene_macro_mark_rect(x,y,w,h,reserveFlag);OxcTerrainDef*t=&gOxcTerrains[terrainIndex];int grp[MAX_OXC_BLOCKS<1024?MAX_OXC_BLOCKS:1024],gn=0,any[MAX_OXC_BLOCKS<1024?MAX_OXC_BLOCKS:1024],an=0;
    for(int i=0;i<t->blockCount;i++){OxcBlockDef*b=&gOxcBlocks[t->firstBlock+i];if(b->width!=10||b->length!=10)continue;if(an<(int)(sizeof(any)/sizeof(any[0])))any[an++]=t->firstBlock+i;if(exact_block_group(b,1)&&gn<(int)(sizeof(grp)/sizeof(grp[0])))grp[gn++]=t->firstBlock+i;}
    int*f=gn?grp:any,n=gn?gn:an;if(!n)return 0;int ok=1,x0=x/10,y0=y/10,x1=(x+w-1)/10,y1=(y+h-1)/10;
    for(int my=y0;my<=y1;my++)for(int mx=x0;mx<=x1;mx++){int bd=f[rng_next(seed)%(uint32_t)n];OxcBlockDef*b=&gOxcBlocks[bd];int mi=exact_map_by_name(b->name,t->source,t->origin);if(mi>=0&&paste_map_to_scene(mi,mx*10,my*10,0))scene_macro_mark_underlay_cell(mx,my);else ok=0;}return ok;
}

static int scene_macro_has_floor(int mx,int my){
    if(!A.scene.active)return 0;int x0=mx*10,y0=my*10,x1=x0+10,y1=y0+10;if(x1>A.scene.x)x1=A.scene.x;if(y1>A.scene.y)y1=A.scene.y;
    for(int z=0;z<A.scene.z;z++)for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){SceneCell*c=scene_cell_at(x,y,z);if(c&&c->lib[0]>=0)return 1;}return 0;
}
static void scene_rebuild_macro_flags(void){
    if(!A.scene.active||!A.scene.macroFlags)return;ZeroMemory(A.scene.macroFlags,(size_t)A.scene.macroW*A.scene.macroH);
    if(A.scene.craftW>0&&A.scene.craftH>0)scene_macro_mark_rect(A.scene.craftX,A.scene.craftY,A.scene.craftW,A.scene.craftH,SCENE_MACRO_CRAFT);
    if(A.scene.usoSlotActive)scene_macro_mark_rect(A.scene.usoSlotX,A.scene.usoSlotY,A.scene.usoSlotW,A.scene.usoSlotH,SCENE_MACRO_USO);
    for(int my=0;my<A.scene.macroH;my++)for(int mx=0;mx<A.scene.macroW;mx++)if((A.scene.macroFlags[my*A.scene.macroW+mx]&(SCENE_MACRO_CRAFT|SCENE_MACRO_USO))&&scene_macro_has_floor(mx,my))A.scene.macroFlags[my*A.scene.macroW+mx]|=SCENE_MACRO_UNDERLAY;
}
static int exact_cell_free(int*owner,int gw,int gh,int x,int y,int w,int h){if(x<0||y<0||x+w>gw||y+h>gh)return 0;for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++)if(owner[(y+yy)*gw+x+xx]!=0)return 0;return 1;}
static int exact_find_position(const OxcCommandDef*c,int*owner,int gw,int gh,int bw,int bh,uint32_t*seed,int*outx,int*outy){int cap=gw*gh,n=0;int*xy=(int*)malloc((size_t)cap*2*sizeof(int));if(!xy)return 0;for(int y=0;y<gh;y++)for(int x=0;x<gw;x++){int in=0;if(c->rectCount==0)in=1;else for(int r=0;r<c->rectCount;r++){int rx=c->rects[r][0],ry=c->rects[r][1],rw=c->rects[r][2],rh=c->rects[r][3];if(x>=rx&&y>=ry&&x+bw<=rx+rw&&y+bh<=ry+rh){in=1;break;}}if(in&&exact_cell_free(owner,gw,gh,x,y,bw,bh)){xy[n*2]=x;xy[n*2+1]=y;n++;}}if(!n){free(xy);return 0;}int k=(int)(rng_next(seed)%(uint32_t)n);*outx=xy[k*2];*outy=xy[k*2+1];free(xy);return 1;}

static int exact_position_in_rects(const OxcCommandDef*c,int x,int y,int w,int h){
    if(c->rectCount==0)return 1;for(int r=0;r<c->rectCount;r++){int rx=c->rects[r][0],ry=c->rects[r][1],rw=c->rects[r][2],rh=c->rects[r][3];if(x>=rx&&y>=ry&&x+w<=rx+rw&&y+h<=ry+rh)return 1;}return 0;
}
static int exact_find_reservation_position(const OxcCommandDef*c,int*owner,int gw,int gh,int bw,int bh,uint32_t*seed,int manualX,int manualY,int*outx,int*outy){
    if(manualX>=0&&manualY>=0){int x=(manualX/10),y=(manualY/10);if(exact_position_in_rects(c,x,y,bw,bh)&&exact_cell_free(owner,gw,gh,x,y,bw,bh)){*outx=x;*outy=y;return 1;}return 0;}
    return exact_find_position(c,owner,gw,gh,bw,bh,seed,outx,outy);
}
static int exact_place_block(int blockDef,int terrainIndex,const OxcCommandDef*c,int*owner,int gw,int gh,ExactPlacement*pl,int*pc,uint32_t*seed){if(blockDef<0||blockDef>=gOxcBlockCount||*pc>=gw*gh)return 0;OxcBlockDef*b=&gOxcBlocks[blockDef];int bw=(b->width+9)/10,bh=(b->length+9)/10,x,y;if(!exact_find_position(c,owner,gw,gh,bw,bh,seed,&x,&y))return 0;OxcTerrainDef*t=&gOxcTerrains[terrainIndex];int mi=exact_map_by_name(b->name,t->source,t->origin);if(mi<0)return 0;int id=*pc+1;for(int yy=0;yy<bh;yy++)for(int xx=0;xx<bw;xx++)owner[(y+yy)*gw+x+xx]=id;pl[*pc]=(ExactPlacement){blockDef,mi,x,y,bw,bh,1};(*pc)++;paste_map_to_scene(mi,x*10,y*10,0);return 1;}
static int exact_placement_matches(const ExactPlacement*p,const OxcCommandDef*c,int terrainIndex){if(!p||!p->active)return 0;OxcTerrainDef*t=&gOxcTerrains[terrainIndex];int local=p->blockDef-t->firstBlock;if(c->blockCount){for(int i=0;i<c->blockCount;i++)if(c->blocks[i]==local)return 1;return 0;}if(c->groupCount){for(int i=0;i<c->groupCount;i++)if(exact_block_group(&gOxcBlocks[p->blockDef],c->groups[i]))return 1;return 0;}return 1;}
static int exact_anchor_in_rects(const OxcCommandDef*c,int x,int y){if(!c->rectCount)return 1;for(int r=0;r<c->rectCount;r++)if(x>=c->rects[r][0]&&y>=c->rects[r][1]&&x<c->rects[r][0]+c->rects[r][2]&&y<c->rects[r][1]+c->rects[r][3])return 1;return 0;}
static int exact_script_supported(const OxcMapScriptDef*sc,wchar_t*why,int cap){
    int seenPlacement=0;
    for(int i=0;i<sc->commandCount;i++){
        OxcCommandDef*c=&gOxcCommands[sc->firstCommand+i];
        if(c->unsupportedFlags&OXC_UNSUPPORTED_VERTICAL_LEVELS){if(why)_snwprintf(why,cap-1,tr(L"Commande %d (%ls) : verticalLevels detecte. V2.8.0 bloque cette recette plutot que de perdre sa structure Z."),i+1,oxc_cmd_name(c->type));return 0;}
        if(c->unsupportedFlags&OXC_UNSUPPORTED_RANDOM_TERRAIN){if(why)_snwprintf(why,cap-1,tr(L"Commande %d (%ls) : randomTerrain detecte et non encore traduit fidelement."),i+1,oxc_cmd_name(c->type));return 0;}
        if(c->unsupportedFlags&OXC_UNSUPPORTED_CRAFT_GROUPS){if(why)_snwprintf(why,cap-1,tr(L"Commande %d (%ls) : craftGroups detecte ; la condition depend du craft deploye et ne peut pas etre devinee."),i+1,oxc_cmd_name(c->type));return 0;}
        if(c->unsupportedFlags&OXC_UNSUPPORTED_NAMED_CRAFT_UFO){if(why)_snwprintf(why,cap-1,tr(L"Commande %d (%ls) : craftName/UFOName explicite detecte ; selection manuelle interdite en mode exact."),i+1,oxc_cmd_name(c->type));return 0;}
        if(c->type==OXC_CMD_UNKNOWN||c->type==OXC_CMD_ADDLINE||c->type==OXC_CMD_DIGTUNNEL){if(why)_snwprintf(why,cap-1,tr(L"Commande %d : %ls non encore traduite dans le compositeur exact."),i+1,oxc_cmd_name(c->type));return 0;}
        if(c->type==OXC_CMD_RESIZE&&seenPlacement){if(why)_snwprintf(why,cap-1,tr(L"Commande %d : resize apres placement. OXCE l'interdit lui-meme ; recette refusee pour diagnostic."),i+1);return 0;}
        if(c->type==OXC_CMD_ADDBLOCK||c->type==OXC_CMD_ADDCRAFT||c->type==OXC_CMD_ADDUFO||c->type==OXC_CMD_FILLAREA)seenPlacement=1;
    }
    return 1;
}
static void composer_show_recipe(void){
    int si=composer_selected_script_index();if(si<0||si>=gOxcScriptCount){MessageBoxW(CUI.hwnd,tr(L"Aucune structure OpenXcom selectionnee."),APP_TITLE,MB_ICONINFORMATION);return;}OxcMapScriptDef*sc=&gOxcScripts[si];wchar_t*txt=(wchar_t*)calloc(32768,sizeof(wchar_t));if(!txt)return;int rw=0,rh=0;composer_script_recommended_size(sc,&rw,&rh);int pos=_snwprintf(txt,32767,tr(L"STRUCTURE OPENXCOM : %ls\nNom technique : %ls\nSource : %ls%ls%ls\nFichier : %ls\n"),composer_script_friendly(sc->name),sc->name,source_name(sc->source),sc->source==SRC_MOD?L" / ":L"",sc->source==SRC_MOD?sc->origin:L"",sc->rulePath);if(rw>0&&rh>0){int n=_snwprintf(txt+pos,32767-pos,tr(L"Taille historique conseillee : %d x %d cases\n"),rw,rh);if(n>0)pos+=n;}if(pos<32700){wcscat(txt,tr(L"\nEtapes de la recette :\n"));pos=(int)wcslen(txt);}for(int i=0;i<sc->commandCount&&pos<31500;i++){OxcCommandDef*c=&gOxcCommands[sc->firstCommand+i];int n=_snwprintf(txt+pos,32767-pos,tr(L"%02d. %ls"),i+1,oxc_cmd_name(c->type));if(n>0)pos+=n;if(c->sizeX>1||c->sizeY>1){n=_snwprintf(txt+pos,32767-pos,tr(L" | bloc %dx%d"),c->sizeX*10,c->sizeY*10);if(n>0)pos+=n;}if(c->groupCount){n=_snwprintf(txt+pos,32767-pos,tr(L" | groupe(s): %d"),c->groupCount);if(n>0)pos+=n;}if(c->blockCount){n=_snwprintf(txt+pos,32767-pos,tr(L" | bloc(s) impose(s): %d"),c->blockCount);if(n>0)pos+=n;}if(c->rectCount){n=_snwprintf(txt+pos,32767-pos,tr(L" | zone(s) contrainte(s): %d"),c->rectCount);if(n>0)pos+=n;}if(c->executions>1){n=_snwprintf(txt+pos,32767-pos,tr(L" | repetitions: %d"),c->executions);if(n>0)pos+=n;}if(c->executionChances<100){n=_snwprintf(txt+pos,32767-pos,tr(L" | chance: %d%%"),c->executionChances);if(n>0)pos+=n;}if(c->label){n=_snwprintf(txt+pos,32767-pos,tr(L" | condition #%d"),c->label);if(n>0)pos+=n;}if(pos<32760){txt[pos++]=L'\n';txt[pos]=0;}}
    wchar_t why[300];if(exact_script_supported(sc,why,300))wcscat(txt,tr(L"\nEtat : compatible avec le moteur FIDELE V2.8.0.\nAucun remplissage aleatoire ne remplace une contrainte manquante."));else{wcscat(txt,tr(L"\nEtat : NON PRISE EN CHARGE en mode fidele.\n"));wcscat(txt,why);}MessageBoxW(CUI.hwnd,txt,tr(L"Structure / recette OpenXcom"),MB_OK|MB_ICONINFORMATION);free(txt);
}
static int composer_generate_exact(void){
    int ss=(int)SendMessageW(CUI.scriptCombo,CB_GETCURSEL,0,0);
    if(ss<0)return 0;
    int scriptIndex=(int)SendMessageW(CUI.scriptCombo,CB_GETITEMDATA,ss,0);
    if(scriptIndex<0||scriptIndex>=gOxcScriptCount)return 0;
    OxcMapScriptDef*sc=&gOxcScripts[scriptIndex];
    wchar_t why[300];
    if(!exact_script_supported(sc,why,300)){
        MessageBoxW(CUI.hwnd,why,tr(L"FIDELE OPENXCOM - recette non traduite"),MB_ICONWARNING);
        return 0;
    }

    int ts=(int)SendMessageW(CUI.terrainCombo,CB_GETCURSEL,0,0);
    if(ts<0)return 0;
    int terrainMap=(int)SendMessageW(CUI.terrainCombo,CB_GETITEMDATA,ts,0);
    if(terrainMap<0||terrainMap>=A.mapCount)return 0;
    MapEntry*tm=&A.maps[terrainMap];
    wchar_t pref[96];map_prefix(tm->name,pref,96);
    int terrainIndex=exact_terrain_for_family(pref,tm->source,tm->origin);
    wchar_t terrainWhy[300];
    if(terrainIndex<0){
        MessageBoxW(CUI.hwnd,tr(L"FIDELE OPENXCOM : impossible de relier cette famille de MAP a sa definition terrain/mapBlocks dans les rulesets indexes."),APP_TITLE,MB_ICONERROR);
        return 0;
    }
    if(!exact_script_terrain_compatible(sc,terrainIndex,terrainWhy,300)){
        wchar_t msg[640];_snwprintf(msg,639,tr(L"Le terrain selectionne ne peut pas executer cette recette fidelement.\r\n\r\n%ls\r\n\r\nChoisissez une famille compatible ; le Workshop ne remplacera pas les blocs manquants au hasard."),terrainWhy);
        composer_update_info(0);
        MessageBoxW(CUI.hwnd,msg,tr(L"FIDELE OPENXCOM - terrain incompatible"),MB_ICONWARNING);
        return 0;
    }
    int mapW=edit_get_int(CUI.widthEdit,60),mapH=edit_get_int(CUI.lengthEdit,60);if(mapW<10)mapW=60;if(mapH<10)mapH=60;int seenPlacement=0;for(int i=0;i<sc->commandCount;i++){OxcCommandDef*c=&gOxcCommands[sc->firstCommand+i];if(c->type==OXC_CMD_RESIZE&&!seenPlacement){if(c->sizeX>0)mapW=c->sizeX*10;if(c->sizeY>0)mapH=c->sizeY*10;}else if(c->type!=OXC_CMD_RESIZE)seenPlacement=1;}if(mapW<10||mapH<10||mapW%10||mapH%10||mapW>255||mapH>255){MessageBoxW(CUI.hwnd,tr(L"FIDELE OPENXCOM : dimensions finales invalides pour le Workshop."),APP_TITLE,MB_ICONERROR);return 0;}
    int craftSel=(int)SendMessageW(CUI.craftCombo,CB_GETCURSEL,0,0),craftMap=craftSel>=0?(int)SendMessageW(CUI.craftCombo,CB_GETITEMDATA,craftSel,0):-1;int usoMode=(int)SendMessageW(CUI.usoModeCombo,CB_GETCURSEL,0,0),usoSel=(int)SendMessageW(CUI.usoCombo,CB_GETCURSEL,0,0),usoMap=usoSel>=0?(int)SendMessageW(CUI.usoCombo,CB_GETITEMDATA,usoSel,0):-1;
    int maxz=4;for(int i=0;i<A.mapCount;i++)if(A.maps[i].z>maxz)maxz=A.maps[i].z;if(maxz>32)maxz=32;if(!scene_alloc(mapW,mapH,maxz)){MessageBoxW(CUI.hwnd,tr(L"Memoire insuffisante."),APP_TITLE,MB_ICONERROR);return 0;}uint32_t seed=(uint32_t)edit_get_int(CUI.seedEdit,12345);if(!seed)seed=1;int cx=-1,cy=-1,ux=-1,uy=-1,craftPlaced=0,usoPlaced=0;int craftReqX=edit_get_int(CUI.craftXEdit,-1),craftReqY=edit_get_int(CUI.craftYEdit,-1),usoReqX=edit_get_int(CUI.usoXEdit,-1),usoReqY=edit_get_int(CUI.usoYEdit,-1);
    if(usoMode>0&&usoMap>=0){A.scene.usoMapIndex=usoMap;A.scene.usoInserted=(usoMode==1);wcsncpy(A.scene.usoSlotName,A.maps[usoMap].name,95);}
    int gw=mapW/10,gh=mapH/10,*owner=(int*)calloc((size_t)gw*gh,sizeof(int));ExactPlacement*pl=(ExactPlacement*)calloc((size_t)gw*gh,sizeof(ExactPlacement));if(!owner||!pl){free(owner);free(pl);scene_free();return 0;}
    int pc=0,known[1024]={0},cond[1024]={0},failed=0;wchar_t fail[512]=L"";
    for(int ii=0;ii<sc->commandCount&&!failed;ii++){OxcCommandDef*c=&gOxcCommands[sc->firstCommand+ii];int execute=1;for(int q=0;q<c->conditionalCount;q++){int v=c->conditionals[q],lab=abs(v);if(lab>=1024||!known[lab]||(v>0&&!cond[lab])||(v<0&&cond[lab])){execute=0;break;}}if(!execute)continue;int success=0;if((int)(rng_next(&seed)%100u)>=clampi(c->executionChances,0,100)){if(c->label>0&&c->label<1024){known[c->label]=1;cond[c->label]=0;}continue;}int uses[MAX_OXC_SELECT]={0};
        if(c->type==OXC_CMD_RESIZE){success=1;}
        else if(c->type==OXC_CMD_ADDCRAFT){
            if(craftMap>=0&&!craftPlaced){
                MapEntry*m=&A.maps[craftMap];int bw=(m->x+9)/10,bh=(m->y+9)/10,mx=-1,my=-1;
                if(exact_find_reservation_position(c,owner,gw,gh,bw,bh,&seed,craftReqX,craftReqY,&mx,&my)){
                    int tx=mx*10,ty=my*10;int ti=c->terrain[0]?exact_terrain_by_name_ctx(c->terrain,tm->source,tm->origin):terrainIndex;
                    if(ti<0){failed=1;_snwprintf(fail,511,tr(L"Commande %d addCraft : terrain de sous-sol introuvable."),ii+1);}
                    else{
                        cx=tx;cy=ty;A.scene.craftX=cx;A.scene.craftY=cy;A.scene.craftW=m->x;A.scene.craftH=m->y;
                        exact_place_reserved_underlay(c,ti,tx,ty,m->x,m->y,owner,gw,gh,pl,&pc,&seed,SCENE_MACRO_CRAFT);
                        craftPlaced=1;success=1;
                    }
                }
            }
        }
        else if(c->type==OXC_CMD_ADDUFO){
            if(usoMode>0&&usoMap>=0&&!usoPlaced){
                MapEntry*m=&A.maps[usoMap];int bw=(m->x+9)/10,bh=(m->y+9)/10,mx=-1,my=-1;
                if(exact_find_reservation_position(c,owner,gw,gh,bw,bh,&seed,usoReqX,usoReqY,&mx,&my)){
                    int tx=mx*10,ty=my*10;int ti=c->terrain[0]?exact_terrain_by_name_ctx(c->terrain,tm->source,tm->origin):terrainIndex;
                    if(ti<0){failed=1;_snwprintf(fail,511,tr(L"Commande %d addUFO : terrain de sous-sol introuvable."),ii+1);}
                    else{
                        ux=tx;uy=ty;A.scene.usoSlotActive=1;A.scene.usoSlotX=ux;A.scene.usoSlotY=uy;A.scene.usoSlotW=m->x;A.scene.usoSlotH=m->y;
                        exact_place_reserved_underlay(c,ti,tx,ty,m->x,m->y,owner,gw,gh,pl,&pc,&seed,SCENE_MACRO_USO);
                        usoPlaced=1;success=1;
                    }
                }
            }
        }
        else if(c->type==OXC_CMD_ADDBLOCK){for(int ex=0;ex<(c->executions>0?c->executions:1);ex++){int ti=c->terrain[0]?exact_terrain_by_name_ctx(c->terrain,tm->source,tm->origin):terrainIndex;if(ti<0){failed=1;_snwprintf(fail,511,tr(L"Commande %d addBlock : terrain %ls introuvable."),ii+1,c->terrain);break;}int bd=exact_pick_block(c,ti,uses,&seed);if(bd>=0&&exact_place_block(bd,ti,c,owner,gw,gh,pl,&pc,&seed))success=1;}}
        else if(c->type==OXC_CMD_FILLAREA){int ti=c->terrain[0]?exact_terrain_by_name_ctx(c->terrain,tm->source,tm->origin):terrainIndex;if(ti<0){failed=1;_snwprintf(fail,511,tr(L"Commande %d fillArea : terrain introuvable."),ii+1);}else for(int guard=0;guard<gw*gh+64;guard++){int bd=exact_pick_block(c,ti,uses,&seed);if(bd<0)break;if(!exact_place_block(bd,ti,c,owner,gw,gh,pl,&pc,&seed))break;success=1;}}
        else if(c->type==OXC_CMD_CHECKBLOCK){for(int p=0;p<pc&&!success;p++)if(pl[p].active&&exact_anchor_in_rects(c,pl[p].x,pl[p].y)&&exact_placement_matches(&pl[p],c,terrainIndex))success=1;}
        else if(c->type==OXC_CMD_REMOVE){for(int p=0;p<pc;p++)if(pl[p].active&&exact_anchor_in_rects(c,pl[p].x,pl[p].y)&&exact_placement_matches(&pl[p],c,terrainIndex)){scene_clear_rect(pl[p].x*10,pl[p].y*10,pl[p].w*10,pl[p].h*10);for(int yy=0;yy<pl[p].h;yy++)for(int xx=0;xx<pl[p].w;xx++)if(owner[(pl[p].y+yy)*gw+pl[p].x+xx]==p+1)owner[(pl[p].y+yy)*gw+pl[p].x+xx]=0;pl[p].active=0;success=1;}}
        if(c->label>0&&c->label<1024){known[c->label]=1;cond[c->label]=success;}if(!success&&!c->canBeSkipped&&c->type!=OXC_CMD_CHECKBLOCK){failed=1;_snwprintf(fail,511,tr(L"Commande %d (%ls) obligatoire : placement impossible avec les contraintes groups/blocks/rects/occupation."),ii+1,oxc_cmd_name(c->type));}
    }
    int holes=0;for(int i=0;i<gw*gh;i++)if(owner[i]==0)holes++;
    if(!failed&&holes){
        failed=1;int rw=0,rh=0;composer_script_recommended_size(sc,&rw,&rh);
        if(rw>0&&rh>0)_snwprintf(fail,511,tr(L"La recette %ls est concue pour environ %dx%d cases. Avec %dx%d, %d macro-case(s) 10x10 restent libres. Le mode FIDELE ne comble jamais au hasard. Verifiez la famille de terrain ou utilisez la taille historique."),sc->name,rw,rh,mapW,mapH,holes);
        else _snwprintf(fail,511,tr(L"Recette executee dans l'ordre, mais %d macro-case(s) 10x10 restent libres. Aucun remplissage aleatoire n'a ete applique."),holes);
    }
    /* Recalcule l'indicateur "sol sous reservation" avant de superposer les sprites
       craft/USO, afin de ne pas confondre leur propre floor avec le terrain dessous. */
    scene_rebuild_macro_flags();
    if(craftPlaced)paste_map_to_scene(craftMap,cx,cy,0);
    if(usoPlaced&&A.scene.usoInserted)paste_map_to_scene(A.scene.usoMapIndex,A.scene.usoSlotX,A.scene.usoSlotY,0);
    free(owner);free(pl);
    _snwprintf(A.scene.title,159,tr(L"FIDELE OPENXCOM %ls / %ls %dx%d | seed %u%ls"),pref,sc->name,mapW,mapH,(unsigned)edit_get_int(CUI.seedEdit,12345),failed?tr(L" [INCOMPLET]"):L"");A.currentZ=A.scene.z-1;A.panX=A.panY=0;A.hoverValid=0;A.scene.dirty=0;reset_undo();if(failed){set_status(fail);MessageBoxW(CUI.hwnd,fail,tr(L"FIDELE OPENXCOM - generation arretee proprement"),MB_ICONWARNING);}else{wchar_t st[512];_snwprintf(st,511,tr(L"FIDELE OPENXCOM : %ls execute sans remplissage aleatoire. %d placements contraints."),sc->name,pc);set_status(st);}InvalidateRect(A.hwnd,NULL,FALSE);return !failed;
}
static int scene_alloc(int w,int h,int z){scene_free();A.scene.x=w;A.scene.y=h;A.scene.z=z;A.scene.macroW=(w+9)/10;A.scene.macroH=(h+9)/10;size_t n=(size_t)w*h*z;A.scene.cells=(SceneCell*)malloc(n*sizeof(SceneCell));A.scene.plan=(PlanCell*)calloc(n,sizeof(PlanCell));A.scene.macroFlags=(uint8_t*)calloc((size_t)A.scene.macroW*A.scene.macroH,1);if(!A.scene.cells||!A.scene.plan||!A.scene.macroFlags){free(A.scene.cells);free(A.scene.plan);free(A.scene.macroFlags);ZeroMemory(&A.scene,sizeof(A.scene));return 0;}for(size_t i=0;i<n;i++)for(int p=0;p<4;p++){A.scene.cells[i].lib[p]=-1;A.scene.cells[i].local[p]=-1;}A.scene.active=1;A.scene.dirty=0;A.scene.usoMapIndex=-1;A.planSelActive=0;A.planDiagWarnMask=0;reset_undo();return 1;}
static void composer_generate(void){
    if(!proc_can_replace(CUI.hwnd))return;
    int modeSel=CUI.modeCombo?(int)SendMessageW(CUI.modeCombo,CB_GETCURSEL,0,0):1;if(modeSel==0){composer_generate_exact();return;}
    wchar_t pref[96]={0};int ts=(int)SendMessageW(CUI.terrainCombo,CB_GETCURSEL,0,0);if(ts<0)return;int terrainMap=(int)SendMessageW(CUI.terrainCombo,CB_GETITEMDATA,ts,0);if(terrainMap<0||terrainMap>=A.mapCount)return;MapEntry*tm=&A.maps[terrainMap];map_prefix(tm->name,pref,96);int terrainSource=tm->source;wchar_t terrainOrigin[96];wcsncpy(terrainOrigin,tm->origin,95);terrainOrigin[95]=0;
    int mapW=edit_get_int(CUI.widthEdit,60),mapH=edit_get_int(CUI.lengthEdit,60);if(mapW<10)mapW=60;if(mapH<10)mapH=60;if(mapW>255||mapH>255||mapW%10||mapH%10){MessageBoxW(CUI.hwnd,tr(L"Mode LIBRE : largeur et longueur doivent etre des multiples de 10 entre 10 et 250."),APP_TITLE,MB_ICONWARNING);return;}int maxSize=mapW>mapH?mapW:mapH;
    int craftSel=(int)SendMessageW(CUI.craftCombo,CB_GETCURSEL,0,0);int craftMap=craftSel>=0?(int)SendMessageW(CUI.craftCombo,CB_GETITEMDATA,craftSel,0):-1;
    int usoMode=(int)SendMessageW(CUI.usoModeCombo,CB_GETCURSEL,0,0);int usoSel=(int)SendMessageW(CUI.usoCombo,CB_GETCURSEL,0,0);int usoMap=usoSel>=0?(int)SendMessageW(CUI.usoCombo,CB_GETITEMDATA,usoSel,0):-1;
    int candidates[1024],cc=collect_effective_terrain_maps(pref,terrainSource,terrainOrigin,maxSize,candidates,1024);int maxz=4;for(int c=0;c<cc;c++)if(A.maps[candidates[c]].z>maxz)maxz=A.maps[candidates[c]].z;if(craftMap>=0&&A.maps[craftMap].z>maxz)maxz=A.maps[craftMap].z;if(usoMap>=0&&A.maps[usoMap].z>maxz)maxz=A.maps[usoMap].z;
    if(!scene_alloc(mapW,mapH,maxz)){MessageBoxW(CUI.hwnd,tr(L"Memoire insuffisante pour la scene."),APP_TITLE,MB_ICONERROR);return;}
    uint32_t seed=(uint32_t)edit_get_int(CUI.seedEdit,12345);if(seed==0)seed=1;
    int cx=-1,cy=-1,ux=-1,uy=-1,minDist=edit_get_int(CUI.minDistEdit,20);
    if(craftMap>=0){MapEntry*m=&A.maps[craftMap];cx=macro_snap_position(edit_get_int(CUI.craftXEdit,-1),mapW,m->x,&seed);cy=macro_snap_position(edit_get_int(CUI.craftYEdit,-1),mapH,m->y,&seed);A.scene.craftX=cx;A.scene.craftY=cy;A.scene.craftW=m->x;A.scene.craftH=m->y;scene_macro_mark_rect(cx,cy,m->x,m->y,SCENE_MACRO_CRAFT);}
    if(usoMode>0&&usoMap>=0){MapEntry*m=&A.maps[usoMap];ux=edit_get_int(CUI.usoXEdit,-1);uy=edit_get_int(CUI.usoYEdit,-1);if(ux<0||uy<0){for(int tries=0;tries<400;tries++){int tx=macro_snap_position(-1,mapW,m->x,&seed);int ty=macro_snap_position(-1,mapH,m->y,&seed);int ok=1;if(craftMap>=0){int c1x=cx+A.scene.craftW/2,c1y=cy+A.scene.craftH/2,u1x=tx+m->x/2,u1y=ty+m->y/2;int dx=c1x-u1x,dy=c1y-u1y;if(dx*dx+dy*dy<minDist*minDist)ok=0;}if(ok){ux=tx;uy=ty;break;}}}if(ux<0)ux=0;if(uy<0)uy=0;ux=macro_snap_position(ux,mapW,m->x,&seed);uy=macro_snap_position(uy,mapH,m->y,&seed);A.scene.usoSlotActive=1;A.scene.usoSlotX=ux;A.scene.usoSlotY=uy;A.scene.usoSlotW=m->x;A.scene.usoSlotH=m->y;A.scene.usoMapIndex=usoMap;A.scene.usoInserted=(usoMode==1);wcsncpy(A.scene.usoSlotName,m->name,95);scene_macro_mark_rect(ux,uy,m->x,m->y,SCENE_MACRO_USO);}
    int gw=mapW/10,gh=mapH/10;uint8_t*occ=(uint8_t*)calloc((size_t)gw*gh,1);if(!occ)return;
    /* Reserver d'abord les macro-cases touchees par craft et slot USO. */
    if(craftMap>=0){int x0=cx/10,y0=cy/10,x1=(cx+A.scene.craftW-1)/10,y1=(cy+A.scene.craftH-1)/10;for(int yy=y0;yy<=y1;yy++)for(int xx=x0;xx<=x1;xx++)if(xx>=0&&yy>=0&&xx<gw&&yy<gh)occ[yy*gw+xx]=1;}
    if(A.scene.usoSlotActive){int x0=A.scene.usoSlotX/10,y0=A.scene.usoSlotY/10,x1=(A.scene.usoSlotX+A.scene.usoSlotW-1)/10,y1=(A.scene.usoSlotY+A.scene.usoSlotH-1)/10;for(int yy=y0;yy<=y1;yy++)for(int xx=x0;xx<=x1;xx++)if(xx>=0&&yy>=0&&xx<gw&&yy<gh)occ[yy*gw+xx]=1;}
    int terrainIndex=exact_terrain_for_family(pref,terrainSource,terrainOrigin),underlayOk=1;if(craftMap>=0&& !free_fill_reserved_underlay(terrainIndex,cx,cy,A.scene.craftW,A.scene.craftH,&seed,SCENE_MACRO_CRAFT))underlayOk=0;if(A.scene.usoSlotActive&&!free_fill_reserved_underlay(terrainIndex,A.scene.usoSlotX,A.scene.usoSlotY,A.scene.usoSlotW,A.scene.usoSlotH,&seed,SCENE_MACRO_USO))underlayOk=0;
    if(!underlayOk)MessageBoxW(CUI.hwnd,tr(L"Certaines macro-cases reservees au vaisseau/USO n'ont pas trouve de sous-sol de groupe 1 dans ce terrain. La generation continue en mode LIBRE, mais ces reservations sont signalees sans croix de sous-sol."),APP_TITLE,MB_OK|MB_ICONWARNING);
    /* candidates contient l'etat effectif : blocs du mod selectionne + OXCE/TFTD herites, sans doublons de nom. */
    for(int gy=0;gy<gh;gy++)for(int gx=0;gx<gw;gx++){if(occ[gy*gw+gx])continue;int fits[1024],fc=0;for(int c=0;c<cc;c++){MapEntry*m=&A.maps[candidates[c]];int bw=m->x/10,bh=m->y/10;if(gx+bw>gw||gy+bh>gh)continue;int ok=1;for(int yy=0;yy<bh&&ok;yy++)for(int xx=0;xx<bw;xx++)if(occ[(gy+yy)*gw+gx+xx]){ok=0;break;}if(ok)fits[fc++]=candidates[c];}if(fc){int pick=fits[rng_next(&seed)%fc];MapEntry*m=&A.maps[pick];int bw=m->x/10,bh=m->y/10;paste_map_to_scene(pick,gx*10,gy*10,0);for(int yy=0;yy<bh;yy++)for(int xx=0;xx<bw;xx++)occ[(gy+yy)*gw+gx+xx]=1;}}
    free(occ);
    if(craftMap>=0)paste_map_to_scene(craftMap,cx,cy,0);
    if(A.scene.usoSlotActive&&A.scene.usoInserted)paste_map_to_scene(A.scene.usoMapIndex,A.scene.usoSlotX,A.scene.usoSlotY,0);
    _snwprintf(A.scene.title,159,tr(L"%ls %dx%d | seed %u | %d blocs effectifs"),pref,mapW,mapH,(unsigned)edit_get_int(CUI.seedEdit,12345),cc);A.currentZ=A.scene.z-1;A.panX=A.panY=0;A.hoverValid=0;A.scene.dirty=0;reset_undo();wchar_t st[512];_snwprintf(st,511,tr(L"Scene generee : %ls. Slot USO : %ls. Edition manuelle active."),A.scene.title,A.scene.usoSlotActive?A.scene.usoSlotName:tr(L"aucun"));set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);
}
static void scene_insert_uso(void){
    if(!A.scene.active||!A.scene.usoSlotActive||A.scene.usoMapIndex<0){set_status(tr(L"Aucun slot USO exploitable dans cette scene."));return;}
    if(A.scene.usoInserted){set_status(tr(L"L'USO est deja insere dans le slot."));return;}
    if(paste_map_to_scene(A.scene.usoMapIndex,A.scene.usoSlotX,A.scene.usoSlotY,0)){A.scene.usoInserted=1;A.scene.dirty=1;reset_undo();set_status(tr(L"USO insere dans le slot. Vous pouvez maintenant casser ses murs ou ajouter du decor dans la scene."));InvalidateRect(A.hwnd,NULL,FALSE);}else set_status(tr(L"Impossible d'inserer l'USO dans le slot."));
}

static uint32_t composer_random_seed_value(void){
    LARGE_INTEGER q;FILETIME ft;POINT pt={0,0};QueryPerformanceCounter(&q);GetSystemTimeAsFileTime(&ft);GetCursorPos(&pt);
    uint64_t x=(uint64_t)q.QuadPart^(((uint64_t)ft.dwHighDateTime<<32)|ft.dwLowDateTime)^GetTickCount64();
    x^=((uint64_t)(uint32_t)pt.x<<32)^(uint32_t)pt.y;
    x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;
    uint32_t v=(uint32_t)(x^(x>>32))&0x7fffffffu;static uint32_t last=0;if(v==0)v=1;if(v==last)v=(v%2147483646u)+1;last=v;return v;
}
static void composer_random_seed_and_generate(void){
    if(!CUI.seedEdit)return;uint32_t seed=composer_random_seed_value();wchar_t b[32];_snwprintf(b,31,L"%u",(unsigned)seed);SetWindowTextW(CUI.seedEdit,b);composer_generate();
}
static HWND composer_advanced_control(HWND h){
    if(h&&CUI.advancedCount<(int)(sizeof(CUI.advanced)/sizeof(CUI.advanced[0])))CUI.advanced[CUI.advancedCount++]=h;
    if(h)ShowWindow(h,SW_HIDE);
    return h;
}
static void composer_set_advanced_visible(int visible){
    CUI.advancedVisible=visible?1:0;
    for(int i=0;i<CUI.advancedCount;i++)ShowWindow(CUI.advanced[i],CUI.advancedVisible?SW_SHOW:SW_HIDE);
    if(CUI.advancedBtn)SetWindowTextW(CUI.advancedBtn,CUI.advancedVisible?tr(L"Masquer les options vaisseau / USO"):tr(L"Options avancees : vaisseau / USO..."));
    if(CUI.hwnd){RECT r;GetWindowRect(CUI.hwnd,&r);SetWindowPos(CUI.hwnd,NULL,0,0,r.right-r.left,CUI.advancedVisible?720:485,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);}
}

static LRESULT CALLBACK composer_wndproc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_CREATE:{HFONT f=(HFONT)GetStockObject(DEFAULT_GUI_FONT);int y=16;CUI.advancedCount=0;CUI.advancedVisible=0;
        CreateWindowW(L"STATIC",tr(L"Mode de composition"),WS_CHILD|WS_VISIBLE,18,y,270,20,hwnd,NULL,NULL,NULL);CUI.modeCombo=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,220,y-4,340,180,hwnd,(HMENU)CID_COMPOSER_MODE,NULL,NULL);y+=38;
        CreateWindowW(L"STATIC",tr(L"Carte a composer"),WS_CHILD|WS_VISIBLE,18,y,270,20,hwnd,NULL,NULL,NULL);CUI.scriptCombo=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,220,y-4,356,320,hwnd,(HMENU)CID_MAPSCRIPT,NULL,NULL);CUI.recipeBtn=CreateWindowW(L"BUTTON",tr(L"Voir la recette"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,490,y+72,86,25,hwnd,(HMENU)CID_RECIPE,NULL,NULL);y+=38;
        CreateWindowW(L"STATIC",tr(L"Terrain / decor"),WS_CHILD|WS_VISIBLE,18,y,270,20,hwnd,NULL,NULL,NULL);CUI.terrainCombo=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,220,y-4,356,240,hwnd,(HMENU)CID_TERRAIN,NULL,NULL);y+=38;
        CreateWindowW(L"STATIC",tr(L"Largeur / longueur (cases)"),WS_CHILD|WS_VISIBLE,18,y,270,20,hwnd,NULL,NULL,NULL);CUI.widthEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"60",WS_CHILD|WS_VISIBLE|ES_NUMBER|ES_AUTOHSCROLL,220,y-4,72,24,hwnd,(HMENU)CID_WIDTH,NULL,NULL);CreateWindowW(L"STATIC",L"x",WS_CHILD|WS_VISIBLE,300,y,18,20,hwnd,NULL,NULL,NULL);CUI.lengthEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"60",WS_CHILD|WS_VISIBLE|ES_NUMBER|ES_AUTOHSCROLL,320,y-4,72,24,hwnd,(HMENU)CID_LENGTH,NULL,NULL);y+=38;
        CUI.infoText=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE|SS_LEFT,18,y,558,78,hwnd,NULL,NULL,NULL);y+=90;
        CreateWindowW(L"STATIC",tr(L"Variation (numero)"),WS_CHILD|WS_VISIBLE,18,y,170,20,hwnd,NULL,NULL,NULL);CUI.seedEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"12345",WS_CHILD|WS_VISIBLE|ES_NUMBER,220,y-5,120,24,hwnd,(HMENU)CID_SEED,NULL,NULL);CreateWindowW(L"BUTTON",tr(L"Autre variation"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,350,y-5,120,24,hwnd,(HMENU)CID_RANDOM_SEED,NULL,NULL);y+=42;
        CreateWindowW(L"BUTTON",tr(L"Generer / regenerer"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,70,y,180,34,hwnd,(HMENU)CID_GENERATE,NULL,NULL);CreateWindowW(L"BUTTON",tr(L"Fermer"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,280,y,120,34,hwnd,(HMENU)CID_CLOSE,NULL,NULL);y+=48;
        CUI.advancedBtn=CreateWindowW(L"BUTTON",tr(L"Options avancees : vaisseau / USO..."),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,70,y,330,30,hwnd,(HMENU)CID_ADVANCED,NULL,NULL);
        int ay=y+42;HWND h;
        h=CreateWindowW(L"STATIC",tr(L"Vaisseau X-COM (optionnel)"),WS_CHILD|WS_VISIBLE,18,ay,270,20,hwnd,NULL,NULL,NULL);composer_advanced_control(h);CUI.craftCombo=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,220,ay-4,250,220,hwnd,(HMENU)CID_CRAFT,NULL,NULL);composer_advanced_control(CUI.craftCombo);ay+=38;
        h=CreateWindowW(L"STATIC",tr(L"Position vaisseau X / Y (-1 = auto)"),WS_CHILD|WS_VISIBLE,18,ay,270,20,hwnd,NULL,NULL,NULL);composer_advanced_control(h);CUI.craftXEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"-1",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,300,ay-5,70,24,hwnd,(HMENU)CID_CRAFTX,NULL,NULL);composer_advanced_control(CUI.craftXEdit);CUI.craftYEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"-1",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,390,ay-5,70,24,hwnd,(HMENU)CID_CRAFTY,NULL,NULL);composer_advanced_control(CUI.craftYEdit);ay+=38;
        h=CreateWindowW(L"STATIC",tr(L"USO (optionnel)"),WS_CHILD|WS_VISIBLE,18,ay,270,20,hwnd,NULL,NULL,NULL);composer_advanced_control(h);CUI.usoModeCombo=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,220,ay-4,250,160,hwnd,(HMENU)CID_USOMODE,NULL,NULL);composer_advanced_control(CUI.usoModeCombo);ay+=38;
        h=CreateWindowW(L"STATIC",tr(L"Type d'USO / emplacement"),WS_CHILD|WS_VISIBLE,18,ay,270,20,hwnd,NULL,NULL,NULL);composer_advanced_control(h);CUI.usoCombo=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,220,ay-4,250,240,hwnd,(HMENU)CID_USO,NULL,NULL);composer_advanced_control(CUI.usoCombo);ay+=38;
        h=CreateWindowW(L"STATIC",tr(L"Position USO X / Y (-1 = auto)"),WS_CHILD|WS_VISIBLE,18,ay,270,20,hwnd,NULL,NULL,NULL);composer_advanced_control(h);CUI.usoXEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"-1",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,300,ay-5,70,24,hwnd,(HMENU)CID_USOX,NULL,NULL);composer_advanced_control(CUI.usoXEdit);CUI.usoYEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"-1",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,390,ay-5,70,24,hwnd,(HMENU)CID_USOY,NULL,NULL);composer_advanced_control(CUI.usoYEdit);ay+=38;
        h=CreateWindowW(L"STATIC",tr(L"Distance mini vaisseau / USO"),WS_CHILD|WS_VISIBLE,18,ay,270,20,hwnd,NULL,NULL,NULL);composer_advanced_control(h);CUI.minDistEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"20",WS_CHILD|WS_VISIBLE|ES_NUMBER,300,ay-5,90,24,hwnd,(HMENU)CID_MINDIST,NULL,NULL);composer_advanced_control(CUI.minDistEdit);
        for(HWND c=GetWindow(hwnd,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))SendMessageW(c,WM_SETFONT,(WPARAM)f,TRUE);composer_set_advanced_visible(0);return 0;}
    case WM_COMMAND:if(LOWORD(wp)==CID_GENERATE){composer_generate();return 0;}if(LOWORD(wp)==CID_RECIPE){composer_show_recipe();return 0;}if(LOWORD(wp)==CID_RANDOM_SEED){composer_random_seed_and_generate();return 0;}if(LOWORD(wp)==CID_ADVANCED){composer_set_advanced_visible(!CUI.advancedVisible);return 0;}if(LOWORD(wp)==CID_MAPSCRIPT&&HIWORD(wp)==CBN_SELCHANGE){composer_update_info(1);return 0;}if(LOWORD(wp)==CID_TERRAIN&&HIWORD(wp)==CBN_SELCHANGE){composer_update_info(0);return 0;}if(LOWORD(wp)==CID_COMPOSER_MODE&&HIWORD(wp)==CBN_SELCHANGE){composer_update_info(0);return 0;}if(LOWORD(wp)==CID_CLOSE){DestroyWindow(hwnd);return 0;}break;
    case WM_DESTROY:CUI.hwnd=NULL;return 0;
    }return DefWindowProcW(hwnd,msg,wp,lp);
}

static int current_map_index(void){return A.map.path[0]?map_index_by_path(A.map.path):-1;}
static int current_map_source(void){
    int i=current_map_index();if(i>=0)return A.maps[i].source;
    if(A.map.path[0]&&path_starts_with_ci(A.map.path,A.modsRoot))return SRC_MOD;
    if(A.map.path[0]&&path_starts_with_ci(A.map.path,A.tftdRoot))return SRC_TFTD;
    if(A.map.path[0]&&path_starts_with_ci(A.map.path,A.oxceRoot)&&!path_starts_with_ci(A.map.path,A.modsRoot))return SRC_OXCE;
    return SRC_MANUAL;
}
static const wchar_t* current_map_origin(void){
    static wchar_t derived[96];int i=current_map_index();if(i>=0)return A.maps[i].origin;
    if(A.map.path[0]&&path_starts_with_ci(A.map.path,A.modsRoot)){const wchar_t*p=A.map.path+wcslen(A.modsRoot);while(*p==L'\\'||*p==L'/')p++;int n=0;while(p[n]&&p[n]!=L'\\'&&p[n]!=L'/'&&n<95){derived[n]=p[n];n++;}derived[n]=0;if(derived[0])return derived;}
    return tr(L"MANUEL");
}
static int current_map_is_protected(void){int s=current_map_source();return s==SRC_TFTD||s==SRC_OXCE;}
static void current_map_name(wchar_t*out,int cap){if(A.map.path[0])path_basename_noext(A.map.path,out,cap);else{wcsncpy(out,L"NOUVEAU_BLOC",cap-1);out[cap-1]=0;}}
static int profile_is_generated(const MapProfile*q){if(!q||!q->rulePath[0])return 0;wchar_t b[96];path_basename_noext(q->rulePath,b,96);return _wcsicmp(b,L"000_workshop_generated")==0;}
static int list_has_w(wchar_t arr[][96],int n,const wchar_t*v){for(int i=0;i<n;i++)if(_wcsicmp(arr[i],v)==0)return 1;return 0;}
static int ruleset_file_has_block(const wchar_t*path,const wchar_t*biome,const wchar_t*block){
    if(!path||!path[0]||!biome||!block)return 0;DWORD sz=0;uint8_t*raw=read_all(path,&sz);if(!raw)return 0;char*buf=(char*)malloc((size_t)sz+1);if(!buf){free(raw);return 0;}memcpy(buf,raw,sz);buf[sz]=0;free(raw);
    char bio8[256],blk8[256];WideCharToMultiByte(CP_UTF8,0,biome,-1,bio8,256,NULL,NULL);WideCharToMultiByte(CP_UTF8,0,block,-1,blk8,256,NULL,NULL);
    int inTerrains=0,inTarget=0,inBlocks=0,blocksIndent=-1,found=0;char*cur=buf;
    while(*cur){char*end=strpbrk(cur,"\r\n");if(end)*end=0;int ind=0;while(cur[ind]==' ')ind++;char line[768];strncpy(line,cur+ind,767);line[767]=0;trim_ascii(line);
        if(ind==0&&strncmp(line,"terrains:",9)==0){inTerrains=1;inTarget=0;inBlocks=0;}
        else if(ind==0&&(strncmp(line,"crafts:",7)==0||strncmp(line,"ufos:",5)==0||strncmp(line,"mapScripts:",11)==0)){inTerrains=0;inTarget=0;inBlocks=0;}
        if(inTerrains&&!inBlocks&&strncmp(line,"- name:",7)==0){char*v=line+7;trim_ascii(v);inTarget=(strcmp(v,bio8)==0);}
        if(inTarget&&strncmp(line,"mapBlocks:",10)==0){inBlocks=1;blocksIndent=ind;}
        else if(inBlocks&&ind<blocksIndent&&strncmp(line,"mapBlocks:",10)!=0){inBlocks=0;}
        if(inTarget&&inBlocks&&strncmp(line,"- name:",7)==0){char*v=line+7;trim_ascii(v);if(strcmp(v,blk8)==0){found=1;break;}}
        if(!end)break;cur=end+1;if(*cur=='\n'||*cur=='\r')cur++;
    }free(buf);return found;
}
static int terrain_profile_base(const wchar_t*biome,const wchar_t*mod){
    int best=-1,bestScore=-999999;
    for(int i=0;i<A.profileCount;i++){MapProfile*q=&A.profiles[i];if(q->kind!=0||profile_is_generated(q))continue;wchar_t pref[96];map_prefix(q->mapName,pref,96);if(_wcsicmp(pref,biome)!=0)continue;int sc=profile_score_ctx(i,SRC_MOD,mod);if(sc<=-999999)continue;sc+=q->dataSetCount;if(sc>bestScore){bestScore=sc;best=i;}}
    return best;
}
static int terrain_has_block_base(const wchar_t*biome,const wchar_t*block,const wchar_t*mod){
    for(int i=0;i<A.profileCount;i++){MapProfile*q=&A.profiles[i];if(q->kind!=0||profile_is_generated(q)||_wcsicmp(q->mapName,block)!=0)continue;if(!source_allowed_for_ctx(q->source,q->origin,SRC_MOD,mod))continue;if(ruleset_file_has_block(q->rulePath,biome,block))return 1;}return 0;
}
static void csv_add_unique(wchar_t*csv,int cap,const wchar_t*v){
    if(!v||!v[0])return;wchar_t tmp[4096];wcsncpy(tmp,csv,4095);tmp[4095]=0;wchar_t*ctx=NULL,*p=wcstok(tmp,L",",&ctx);while(p){while(*p==L' ')p++;if(_wcsicmp(p,v)==0)return;p=wcstok(NULL,L",",&ctx);}
    if(csv[0])wcsncat(csv,L",",cap-wcslen(csv)-1);wcsncat(csv,v,cap-wcslen(csv)-1);
}
static int csv_to_list(const wchar_t*csv,wchar_t out[][96],int max){
    if(!csv||!csv[0])return 0;wchar_t tmp[4096];wcsncpy(tmp,csv,4095);tmp[4095]=0;int n=0;wchar_t*ctx=NULL,*p=wcstok(tmp,L",",&ctx);
    while(p&&n<max){while(*p==L' ')p++;wcsncpy(out[n],p,95);out[n][95]=0;size_t k=wcslen(out[n]);while(k&&out[n][k-1]==L' ')out[n][--k]=0;if(out[n][0])n++;p=wcstok(NULL,L",",&ctx);}return n;
}
static void export_paths(const wchar_t*mod,wchar_t*modRoot,wchar_t*manifest,wchar_t*rule){
    _snwprintf(modRoot,PATH_CAP-1,L"%ls\\%ls",A.modsRoot,mod);modRoot[PATH_CAP-1]=0;_snwprintf(manifest,PATH_CAP-1,L"%ls\\Workshop\\workshop_exports.ini",modRoot);manifest[PATH_CAP-1]=0;_snwprintf(rule,PATH_CAP-1,L"%ls\\Ruleset\\000_workshop_generated.rul",modRoot);rule[PATH_CAP-1]=0;
}
static int manifest_find_block_biome(const wchar_t*mod,const wchar_t*block,wchar_t*out,int cap){
    wchar_t mr[PATH_CAP],mf[PATH_CAP],ru[PATH_CAP],sections[16384];export_paths(mod,mr,mf,ru);sections[0]=0;GetPrivateProfileSectionNamesW(sections,16384,mf);wchar_t key[160],val[256];_snwprintf(key,159,L"Block.%ls",block);
    for(wchar_t*sec=sections;*sec;sec+=wcslen(sec)+1){if(_wcsnicmp(sec,L"Terrain:",8)!=0)continue;GetPrivateProfileStringW(sec,key,L"",val,256,mf);if(val[0]){wcsncpy(out,sec+8,cap-1);out[cap-1]=0;return 1;}}return 0;
}
static void current_map_biome(wchar_t*out,int cap){
    wchar_t name[96];current_map_name(name,96);int mi=current_map_index();if(mi>=0&&A.maps[mi].source==SRC_MOD&&manifest_find_block_biome(A.maps[mi].origin,name,out,cap))return;map_prefix(name,out,cap);if(!out[0]){wcsncpy(out,L"SEABED",cap-1);out[cap-1]=0;}
}
static int manifest_added_datasets(const wchar_t*manifest,const wchar_t*biome,wchar_t out[][96],int max){
    wchar_t sec[160],csv[4096];_snwprintf(sec,159,L"Terrain:%ls",biome);GetPrivateProfileStringW(sec,L"AddedDatasets",L"",csv,4096,manifest);return csv_to_list(csv,out,max);
}
static int build_target_datasets(const wchar_t*biome,const wchar_t*mod,SceneCell*logical,wchar_t out[][96],int*count,wchar_t*addedCsv,int addedCap,wchar_t*err,int errCap){
    *count=0;if(addedCsv)addedCsv[0]=0;int pi=terrain_profile_base(biome,mod);if(pi<0){_snwprintf(err,errCap-1,tr(L"Aucun profil terrain trouve pour le biome %ls."),biome);return 0;}MapProfile*q=&A.profiles[pi];
    for(int k=0;k<q->dataSetCount&&*count<MAX_EXPORT_DATASETS;k++){wcsncpy(out[*count],q->dataSets[k],95);out[*count][95]=0;(*count)++;}
    wchar_t mr[PATH_CAP],mf[PATH_CAP],ru[PATH_CAP];export_paths(mod,mr,mf,ru);wchar_t oldAdd[MAX_EXPORT_DATASETS][96];int oldN=manifest_added_datasets(mf,biome,oldAdd,MAX_EXPORT_DATASETS);
    for(int i=0;i<oldN;i++)if(!list_has_w(out,*count,oldAdd[i])&&*count<MAX_EXPORT_DATASETS){wcsncpy(out[*count],oldAdd[i],95);out[*count][95]=0;(*count)++;}
    size_t cells=(size_t)A.map.x*A.map.y*A.map.z;
    for(size_t i=0;i<cells;i++)for(int p=0;p<4;p++){int lib=logical[i].lib[p];if(lib<0)continue;const wchar_t*ds=A.library[lib].name;if(!list_has_w(out,*count,ds)){if(*count>=MAX_EXPORT_DATASETS){_snwprintf(err,errCap-1,tr(L"Trop de datasets pour %ls."),biome);return 0;}wcsncpy(out[*count],ds,95);out[*count][95]=0;(*count)++;if(addedCsv)csv_add_unique(addedCsv,addedCap,ds);}}
    for(int i=0;i<oldN;i++)if(addedCsv)csv_add_unique(addedCsv,addedCap,oldAdd[i]);return 1;
}
static SceneCell* map_to_logical(wchar_t*err,int cap){
    if(!A.map.cells){_snwprintf(err,cap-1,tr(L"Aucune MAP ouverte."));return NULL;}size_t n=(size_t)A.map.x*A.map.y*A.map.z;SceneCell*l=(SceneCell*)malloc(n*sizeof(SceneCell));if(!l)return NULL;
    for(size_t i=0;i<n;i++)for(int p=0;p<4;p++){l[i].lib[p]=-1;l[i].local[p]=-1;uint8_t raw=A.map.cells[i].part[p];if(!raw)continue;int lib=-1,local=-1;if(!active_resolve_raw(raw,&lib,&local)){free(l);_snwprintf(err,cap-1,tr(L"Indice MAP %u non resolu. Chargez le profil/palette correct avant export."),(unsigned)raw);return NULL;}l[i].lib[p]=lib;l[i].local[p]=local;}return l;
}
static int encode_logical_map(SceneCell*logical,const wchar_t*mod,wchar_t datasets[][96],int dsCount,uint8_t**out,DWORD*outSz,wchar_t*err,int cap){
    int base[MAX_EXPORT_DATASETS],cnt[MAX_EXPORT_DATASETS];int total=0;
    for(int d=0;d<dsCount;d++){int lib=library_find_name_ctx(datasets[d],SRC_MOD,mod);if(lib<0){int foreign=library_find_foreign_mod(datasets[d],mod);if(foreign>=0)_snwprintf(err,cap-1,tr(L"Le dataset %ls n'existe que dans un autre mod (%ls). Export bloque pour eviter une dependance implicite."),datasets[d],A.library[foreign].origin);else _snwprintf(err,cap-1,tr(L"Dataset introuvable dans la chaine autorisee %ls -> OXCE STANDARD -> TFTD ORIGINAL : %ls"),mod,datasets[d]);return 0;}base[d]=total;cnt[d]=A.library[lib].mcdCount;total+=cnt[d];}
    DWORD sz=3u+(DWORD)A.map.x*A.map.y*A.map.z*4u;uint8_t*b=(uint8_t*)calloc(sz,1);if(!b)return 0;b[0]=(uint8_t)A.map.y;b[1]=(uint8_t)A.map.x;b[2]=(uint8_t)A.map.z;DWORD off=3;
    for(int fz=0;fz<A.map.z;fz++){int iz=A.map.z-1-fz;for(int fy=0;fy<A.map.y;fy++)for(int fx=0;fx<A.map.x;fx++){size_t ci=(size_t)(iz*A.map.y+fy)*A.map.x+fx;for(int p=0;p<4;p++){int lib=logical[ci].lib[p],local=logical[ci].local[p];if(lib<0){b[off++]=0;continue;}const wchar_t*name=A.library[lib].name;int di=-1;for(int d=0;d<dsCount;d++)if(_wcsicmp(datasets[d],name)==0){di=d;break;}if(di<0||local<0||local>=cnt[di]){free(b);_snwprintf(err,cap-1,tr(L"%ls/MCD %d n'est pas valide dans le dataset cible."),name,local);return 0;}int raw=base[di]+local;if(raw<=0||raw>255){free(b);_snwprintf(err,cap-1,tr(L"%ls/MCD %d donnerait l'indice MAP %d dans %ls (limite 1..255)."),name,local,raw,datasets[di]);return 0;}b[off++]=(uint8_t)raw;}}}
    *out=b;*outSz=sz;return 1;
}
static void append_utf8(char**buf,size_t*len,size_t*cap,const char*txt){size_t n=strlen(txt);if(*len+n+1>*cap){size_t nc=*cap?*cap:4096;while(nc<*len+n+1)nc*=2;char*nb=(char*)realloc(*buf,nc);if(!nb)return;*buf=nb;*cap=nc;}memcpy(*buf+*len,txt,n);*len+=n;(*buf)[*len]=0;}
static void wide_utf8(const wchar_t*w,char*out,int cap){if(!WideCharToMultiByte(CP_UTF8,0,w,-1,out,cap,NULL,NULL))out[0]=0;}
static int regenerate_workshop_rules(const wchar_t*mod,wchar_t*err,int errCap){
    wchar_t mr[PATH_CAP],mf[PATH_CAP],rule[PATH_CAP];export_paths(mod,mr,mf,rule);wchar_t rd[PATH_CAP];wcsncpy(rd,rule,PATH_CAP-1);rd[PATH_CAP-1]=0;path_dirname(rd);ensure_dir_recursive(rd);wchar_t sections[16384];sections[0]=0;GetPrivateProfileSectionNamesW(sections,16384,mf);
    char*buf=NULL;size_t len=0,bc=0;append_utf8(&buf,&len,&bc,"# Generated by TFTD Workshop. Existing mod rules are NOT modified.\n# OXCE loads rulesets in descending path order, so 000_* is intentionally loaded last.\nterrains:\n");
    for(wchar_t*sec=sections;*sec;sec+=wcslen(sec)+1){
        if(_wcsnicmp(sec,L"Terrain:",8)!=0)continue;const wchar_t*biome=sec+8;
        wchar_t addCsv[4096],resolvedCsv[4096],entries[16384];GetPrivateProfileStringW(sec,L"AddedDatasets",L"",addCsv,4096,mf);GetPrivateProfileStringW(sec,L"ResolvedDatasets",L"",resolvedCsv,4096,mf);entries[0]=0;GetPrivateProfileSectionW(sec,entries,16384,mf);
        int anyBlock=0;for(wchar_t*e=entries;*e;e+=wcslen(e)+1)if(_wcsnicmp(e,L"Block.",6)==0){anyBlock=1;break;}
        wchar_t ds[MAX_EXPORT_DATASETS][96];int dn=csv_to_list(resolvedCsv,ds,MAX_EXPORT_DATASETS);char b8[256],line[512];wide_utf8(biome,b8,256);_snprintf(line,sizeof(line)-1,"- name: %s\n  addOnly: true\n",b8);append_utf8(&buf,&len,&bc,line);
        /* Si on ajoute un dataset OU un nouveau mapBlock, on repete la liste
           effective complete. C'est volontaire : RuleTerrain::load remplace
           mapDataSets quand ce champ existe. La liste contient donc la base
           existante + seulement les ajouts, sans perdre les regles du mod. */
        if((addCsv[0]||anyBlock)&&dn>0){append_utf8(&buf,&len,&bc,"  mapDataSets:\n");for(int d=0;d<dn;d++){char d8[256];wide_utf8(ds[d],d8,256);_snprintf(line,sizeof(line)-1,"  - %s\n",d8);append_utf8(&buf,&len,&bc,line);}}
        if(anyBlock){append_utf8(&buf,&len,&bc,"  mapBlocks:\n");for(wchar_t*e=entries;*e;e+=wcslen(e)+1){if(_wcsnicmp(e,L"Block.",6)!=0)continue;wchar_t*eq=wcschr(e,L'=');if(!eq)continue;wchar_t key[160],val[256];size_t kn=(size_t)(eq-e);if(kn>=160)kn=159;wcsncpy(key,e,kn);key[kn]=0;wcsncpy(val,eq+1,255);val[255]=0;const wchar_t*bn=key+6;int w=10,l=10,h=4,g=0;swscanf(val,L"%d,%d,%d,%d",&w,&l,&h,&g);char n8[256];wide_utf8(bn,n8,256);_snprintf(line,sizeof(line)-1,"  - name: %s\n    width: %d\n    length: %d\n    height: %d\n    groups: [%d]\n",n8,w,l,h,g);append_utf8(&buf,&len,&bc,line);}}
    }
    if(!buf){_snwprintf(err,errCap-1,tr(L"Echec generation ruleset."));return 0;}int ok=write_all(rule,buf,(DWORD)len);free(buf);if(!ok)_snwprintf(err,errCap-1,tr(L"Impossible d'ecrire %ls"),rule);return ok;
}
static int backup_export_state(const wchar_t*mod,const wchar_t*sourcePath,const wchar_t targetPaths[][PATH_CAP],int targetCount,wchar_t*outDir,int outCap){
    wchar_t mr[PATH_CAP],mf[PATH_CAP],rule[PATH_CAP],stamp[64];export_paths(mod,mr,mf,rule);make_timestamp(stamp,64);wchar_t userRoot[PATH_CAP];wcsncpy(userRoot,A.modsRoot,PATH_CAP-1);userRoot[PATH_CAP-1]=0;path_dirname(userRoot);_snwprintf(outDir,outCap-1,L"%ls\\TFTD_Workshop_Backups\\%ls\\%ls",userRoot,mod,stamp);outDir[outCap-1]=0;if(!ensure_dir_recursive(outDir))return 0;int ok=1;
    if(sourcePath&&sourcePath[0]){wchar_t dst[PATH_CAP];_snwprintf(dst,PATH_CAP-1,L"%ls\\SOURCE_BEFORE.MAP",outDir);if(!copy_file_if_exists(sourcePath,dst))ok=0;}
    for(int i=0;i<targetCount;i++)if(path_exists_file(targetPaths[i])){wchar_t name[96],dst[PATH_CAP];path_basename_noext(targetPaths[i],name,96);_snwprintf(dst,PATH_CAP-1,L"%ls\\TARGET_BEFORE_%ls.MAP",outDir,name);if(!copy_file_if_exists(targetPaths[i],dst))ok=0;}
    if(path_exists_file(mf)){wchar_t dst[PATH_CAP];_snwprintf(dst,PATH_CAP-1,L"%ls\\workshop_exports_before.ini",outDir);if(!copy_file_if_exists(mf,dst))ok=0;}if(path_exists_file(rule)){wchar_t dst[PATH_CAP];_snwprintf(dst,PATH_CAP-1,L"%ls\\000_workshop_generated_before.rul",outDir);if(!copy_file_if_exists(rule,dst))ok=0;}return ok;
}
static int manifest_update_terrain(const wchar_t*mod,const wchar_t*biome,wchar_t datasets[][96],int dsCount,const wchar_t*addedCsv,const wchar_t*block,int addBlock,int group){
    wchar_t mr[PATH_CAP],mf[PATH_CAP],rule[PATH_CAP],sec[160],resolved[4096],oldAdd[4096];export_paths(mod,mr,mf,rule);wchar_t md[PATH_CAP];wcsncpy(md,mf,PATH_CAP-1);md[PATH_CAP-1]=0;path_dirname(md);ensure_dir_recursive(md);_snwprintf(sec,159,L"Terrain:%ls",biome);resolved[0]=0;for(int i=0;i<dsCount;i++)csv_add_unique(resolved,4096,datasets[i]);GetPrivateProfileStringW(sec,L"AddedDatasets",L"",oldAdd,4096,mf);if(addedCsv&&addedCsv[0]){wchar_t tmp[MAX_EXPORT_DATASETS][96];int n=csv_to_list(addedCsv,tmp,MAX_EXPORT_DATASETS);for(int i=0;i<n;i++)csv_add_unique(oldAdd,4096,tmp[i]);}int ok=1;if(!WritePrivateProfileStringW(sec,L"ResolvedDatasets",resolved,mf))ok=0;if(!WritePrivateProfileStringW(sec,L"AddedDatasets",oldAdd,mf))ok=0;
    if(addBlock&&block&&block[0]){wchar_t key[160],val[128],existing[128];_snwprintf(key,159,L"Block.%ls",block);GetPrivateProfileStringW(sec,key,L"",existing,128,mf);if(existing[0]&&group<0)return ok;if(group<0)group=0;_snwprintf(val,127,L"%d,%d,%d,%d",A.map.x,A.map.y,A.map.z,group);if(!WritePrivateProfileStringW(sec,key,val,mf))ok=0;}return ok;}
static int valid_block_name(const wchar_t*n){if(!n||!n[0])return 0;for(const wchar_t*p=n;*p;p++)if(!(iswalnum(*p)||*p==L'_'||*p==L'-'))return 0;return 1;}

static int current_source_rmp(wchar_t*out,int cap){
    if(!A.map.path[0])return 0;wchar_t name[96],mapDir[PATH_CAP],root[PATH_CAP];current_map_name(name,96);wcsncpy(mapDir,A.map.path,PATH_CAP-1);mapDir[PATH_CAP-1]=0;path_dirname(mapDir);wcsncpy(root,mapDir,PATH_CAP-1);root[PATH_CAP-1]=0;path_dirname(root);
    _snwprintf(out,cap-1,L"%ls\\ROUTES\\%ls.RMP",root,name);out[cap-1]=0;if(path_exists_file(out))return 1;
    _snwprintf(out,cap-1,L"%ls\\Routes\\%ls.RMP",root,name);out[cap-1]=0;return path_exists_file(out);
}

static void rmp_clear(void){
    int keepShow=A.rmp.show;
    ZeroMemory(&A.rmp,sizeof(A.rmp));
    A.rmp.show=keepShow;
    A.rmp.selected=-1;
}
static int rmp_sibling_for_map_path(const wchar_t*mapPath,const wchar_t*name,wchar_t*out,int cap){
    if(!mapPath||!mapPath[0])return 0;wchar_t mapDir[PATH_CAP],root[PATH_CAP];
    wcsncpy(mapDir,mapPath,PATH_CAP-1);mapDir[PATH_CAP-1]=0;path_dirname(mapDir);
    wcsncpy(root,mapDir,PATH_CAP-1);root[PATH_CAP-1]=0;path_dirname(root);
    _snwprintf(out,cap-1,L"%ls\\ROUTES\\%ls.RMP",root,name);out[cap-1]=0;
    return path_exists_file(out);
}
static int rmp_find_effective_path(wchar_t*out,int cap,wchar_t*label,int labelCap,int*inherited){
    if(!A.map.path[0])return 0;wchar_t name[96];path_basename_noext(A.map.path,name,96);
    int mi=map_index_by_path(A.map.path);int currentSrc=mi>=0?A.maps[mi].source:current_map_source();const wchar_t*curOrigin=mi>=0?A.maps[mi].origin:current_map_origin();
    if(rmp_sibling_for_map_path(A.map.path,name,out,cap)){*inherited=0;_snwprintf(label,labelCap-1,L"%ls%ls%ls",source_name(currentSrc),currentSrc==SRC_MOD?L" / ":L"",currentSrc==SRC_MOD?curOrigin:L"");return 1;}
    if(currentSrc==SRC_MOD){
        for(int pass=0;pass<2;pass++){int wanted=pass==0?SRC_OXCE:SRC_TFTD;for(int i=0;i<A.mapCount;i++){if(A.maps[i].source!=wanted||_wcsicmp(A.maps[i].name,name)!=0)continue;if(rmp_sibling_for_map_path(A.maps[i].path,name,out,cap)){*inherited=1;_snwprintf(label,labelCap-1,tr(L"HERITE : %ls"),source_name(wanted));return 1;}}}
    }else if(currentSrc==SRC_OXCE){
        for(int i=0;i<A.mapCount;i++)if(A.maps[i].source==SRC_TFTD&&_wcsicmp(A.maps[i].name,name)==0)if(rmp_sibling_for_map_path(A.maps[i].path,name,out,cap)){*inherited=1;_snwprintf(label,labelCap-1,tr(L"HERITE : TFTD ORIGINAL"));return 1;}
    }
    return 0;
}
static void rmp_sync_link_raw(RmpNode*n){
    for(int j=0;j<5;j++){int v=n->links[j];n->raw[4+j*3]=(uint8_t)(v<0?v+256:v);if(v==-1){n->raw[5+j*3]=0;n->raw[6+j*3]=0;}}
}
static int rmp_load_path(const wchar_t*path,const wchar_t*label,int inherited){
    DWORD sz=0;uint8_t*b=read_all(path,&sz);if(!b)return 0;if(sz%RMP_REC_SIZE!=0){free(b);return 0;}int count=(int)(sz/RMP_REC_SIZE);if(count>MAX_RMP_NODES)count=MAX_RMP_NODES;
    int keep=A.rmp.show;rmp_clear();A.rmp.show=keep;A.rmp.loaded=1;A.rmp.inherited=inherited;wcsncpy(A.rmp.path,path,PATH_CAP-1);if(label)wcsncpy(A.rmp.sourceLabel,label,159);
    for(int i=0;i<count;i++){RmpNode*n=&A.rmp.nodes[i];memcpy(n->raw,b+i*RMP_REC_SIZE,RMP_REC_SIZE);n->x=n->raw[1];n->y=n->raw[0];n->z=A.map.z-1-n->raw[2];for(int j=0;j<5;j++){int v=n->raw[4+j*3];n->links[j]=v<=250?v:v-256;}n->autoScore=0;}
    A.rmp.count=count;free(b);return 1;
}
static void rmp_load_for_current_map(void){
    int keep=A.rmp.show;rmp_clear();A.rmp.show=keep;if(!A.map.cells||!A.map.path[0])return;wchar_t p[PATH_CAP],lab[160];int inh=0;if(rmp_find_effective_path(p,PATH_CAP,lab,160,&inh))rmp_load_path(p,lab,inh);
}
static void rmp_toggle_overlay(void){
    A.rmp.show=!A.rmp.show;if(A.rmp.show&&!A.rmp.loaded)rmp_load_for_current_map();set_status(A.rmp.show?tr(L"Routes RMP affichees."):tr(L"Routes RMP masquees."));InvalidateRect(A.hwnd,NULL,FALSE);
}
static void rmp_node_screen(const RmpNode*n,int*cx,int*cy){
    int ox,oy,sx,sy;world_origin(&ox,&oy);project_tile(n->x,n->y,n->z,&sx,&sy);*cx=ox+sx*A.zoom+16*A.zoom;*cy=oy+sy*A.zoom+30*A.zoom;
}
static void rmp_draw_disc(int cx,int cy,int r,uint32_t c){for(int yy=-r;yy<=r;yy++)for(int xx=-r;xx<=r;xx++)if(xx*xx+yy*yy<=r*r)putpx(cx+xx,cy+yy,c);}
static void rmp_draw_socket(const RmpNode*n,int special,uint32_t c){
    int cx,cy,dx=0,dy=0;rmp_node_screen(n,&cx,&cy);if(special==-2)dy=-18*A.zoom;else if(special==-3)dx=18*A.zoom;else if(special==-4)dy=18*A.zoom;else if(special==-5)dx=-18*A.zoom;line_px_thick(cx,cy,cx+dx,cy+dy,c);rmp_draw_disc(cx+dx,cy+dy,2*A.zoom,c);
}
static void rmp_draw_overlay(void){
    if(!A.rmp.show||A.scene.active||!A.map.cells)return;
    for(int i=0;i<A.rmp.count;i++){RmpNode*a=&A.rmp.nodes[i];if(a->z<0||a->z>=A.map.z||!normal_z_visible(a->z))continue;int ax,ay;rmp_node_screen(a,&ax,&ay);
        for(int j=0;j<5;j++){int l=a->links[j];if(l>=0&&l<A.rmp.count&&l>i){RmpNode*b=&A.rmp.nodes[l];int bx,by;rmp_node_screen(b,&bx,&by);uint32_t lc=0xFFB466FFu;if(b->z==a->z){int pd=rmp_grid_distance(a->x,a->y,b->x,b->y,a->z,64);int man=abs(a->x-b->x)+abs(a->y-b->y);if(pd>=9999)lc=0xFFFF4040u;else if(man>0&&pd>man*2)lc=0xFFFFA833u;else lc=0xFF39D789u;}line_px_thick(ax,ay,bx,by,lc);}else if(l<=-2&&l>=-5)rmp_draw_socket(a,l,0xFFFFA833u);else if(l>=A.rmp.count&&l>=0){line_px(ax-5,ay-5,ax+5,ay+5,0xFFFF4040u);line_px(ax+5,ay-5,ax-5,ay+5,0xFFFF4040u);}}
    }
    for(int i=0;i<A.rmp.count;i++){RmpNode*n=&A.rmp.nodes[i];if(n->z<0||n->z>=A.map.z||!normal_z_visible(n->z))continue;int cx,cy;rmp_node_screen(n,&cx,&cy);uint32_t c=i==A.rmp.selected?0xFFFFFF00u:(n->z==A.currentZ?0xFF20E09Bu:0xFF17765Au);rmp_draw_disc(cx,cy,(i==A.rmp.selected?5:4)*A.zoom,c);}
    for(int i=0;i<A.rmp.proposalCount;i++){RmpNode*n=&A.rmp.proposals[i];if(!normal_z_visible(n->z))continue;int cx,cy;rmp_node_screen(n,&cx,&cy);uint32_t c=n->autoScore>=2?0xFF20E6FFu:0xFF4D7DFFu;rmp_draw_disc(cx,cy,3*A.zoom,c);for(int j=0;j<5;j++){int l=n->links[j];if(l>=0&&l<A.rmp.proposalCount&&l>i){int bx,by;rmp_node_screen(&A.rmp.proposals[l],&bx,&by);line_px_thick(cx,cy,bx,by,0xFF3C70CFu);}else if(l<=-2&&l>=-5)rmp_draw_socket(n,l,0xFF49BFFFu);}}
}
static int rmp_hit_test(int mx,int my){
    if(!A.rmp.show||!A.rmp.loaded)return -1;int best=-1,bd=999999,rr=9*A.zoom;for(int i=0;i<A.rmp.count;i++){RmpNode*n=&A.rmp.nodes[i];if(n->z!=A.currentZ)continue;int x,y;rmp_node_screen(n,&x,&y);int dx=mx-x,dy=my-y,d=dx*dx+dy*dy;if(d<=rr*rr&&d<bd){best=i;bd=d;}}return best;
}
static int rmp_free_link(RmpNode*n){for(int i=0;i<5;i++)if(n->links[i]==-1)return i;return -1;}
static void rmp_toggle_link(int a,int b){
    if(a<0||b<0||a>=A.rmp.count||b>=A.rmp.count||a==b)return;RmpNode*na=&A.rmp.nodes[a],*nb=&A.rmp.nodes[b];int ia=-1,ib=-1;for(int j=0;j<5;j++){if(na->links[j]==b)ia=j;if(nb->links[j]==a)ib=j;}
    if(ia>=0||ib>=0){if(ia>=0)na->links[ia]=-1;if(ib>=0)nb->links[ib]=-1;rmp_sync_link_raw(na);rmp_sync_link_raw(nb);A.rmp.dirty=1;set_status(tr(L"Liaison RMP retiree."));return;}
    ia=rmp_free_link(na);ib=rmp_free_link(nb);if(ia<0||ib<0){MessageBoxW(A.hwnd,tr(L"Un des nodes utilise deja ses 5 liens."),APP_TITLE,MB_ICONWARNING);return;}na->links[ia]=b;nb->links[ib]=a;rmp_sync_link_raw(na);rmp_sync_link_raw(nb);A.rmp.dirty=1;set_status(tr(L"Liaison RMP bidirectionnelle ajoutee."));
}
static int rmp_add_node(int x,int y,int z){
    if(A.rmp.count>=MAX_RMP_NODES)return -1;for(int i=0;i<A.rmp.count;i++)if(A.rmp.nodes[i].x==x&&A.rmp.nodes[i].y==y&&A.rmp.nodes[i].z==z)return i;int id=A.rmp.count++;RmpNode*n=&A.rmp.nodes[id];ZeroMemory(n,sizeof(*n));n->x=x;n->y=y;n->z=z;for(int j=0;j<5;j++)n->links[j]=-1;memset(n->raw,0,RMP_REC_SIZE);n->raw[19]=(uint8_t)(A.rmpNewType&3);n->raw[20]=(uint8_t)clampi(A.rmpNewRank,0,255);n->raw[21]=(uint8_t)clampi(A.rmpNewFlags,0,255);n->raw[22]=(uint8_t)(A.rmpNewTarget?5:0);n->raw[23]=(uint8_t)clampi(A.rmpNewPriority,0,255);rmp_sync_link_raw(n);A.rmp.loaded=1;A.rmp.dirty=1;return id;
}
static void rmp_delete_selected(void){
    int id=A.rmp.selected;if(id<0||id>=A.rmp.count)return;for(int i=0;i<A.rmp.count;i++)for(int j=0;j<5;j++){int l=A.rmp.nodes[i].links[j];if(l==id)A.rmp.nodes[i].links[j]=-1;else if(l>id&&l<=250)A.rmp.nodes[i].links[j]--;rmp_sync_link_raw(&A.rmp.nodes[i]);}memmove(&A.rmp.nodes[id],&A.rmp.nodes[id+1],sizeof(RmpNode)*(A.rmp.count-id-1));A.rmp.count--;A.rmp.selected=-1;A.rmp.dirty=1;set_status(tr(L"Node RMP supprime."));
}
static void rmp_toggle_selected_socket(int special){
    int id=A.rmp.selected;if(id<0||id>=A.rmp.count){set_status(tr(L"Selectionnez d'abord un node RMP."));return;}RmpNode*n=&A.rmp.nodes[id];for(int j=0;j<5;j++)if(n->links[j]==special){n->links[j]=-1;rmp_sync_link_raw(n);A.rmp.dirty=1;set_status(tr(L"Socket RMP retire."));InvalidateRect(A.hwnd,NULL,FALSE);return;}int f=rmp_free_link(n);if(f<0){MessageBoxW(A.hwnd,tr(L"Ce node n'a plus de slot de connexion libre."),APP_TITLE,MB_ICONWARNING);return;}n->links[f]=special;rmp_sync_link_raw(n);A.rmp.dirty=1;set_status(tr(L"Socket RMP ajoute."));InvalidateRect(A.hwnd,NULL,FALSE);
}
static int rmp_handle_left_click(int mx,int my,WPARAM wp){
    (void)wp;if(!A.rmp.show||!A.rmp.editMode||A.scene.active)return 0;int hit=rmp_hit_test(mx,my);int editable=current_map_source()==SRC_MOD;
    if(A.rmpTool==0){
        if(hit>=0){A.rmp.selected=hit;rmp_template_from_selected();set_status(tr(L"Node RMP selectionne. Les parametres du panneau reprennent ses valeurs."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}
        if(!editable){MessageBoxW(A.hwnd,tr(L"RMP en lecture seule. Copiez d'abord la MAP dans un mod."),APP_TITLE,MB_ICONWARNING);return 1;}
        int x,y;if(mouse_to_tile(mx,my,&x,&y)){int id=rmp_add_node(x,y,A.currentZ);A.rmp.selected=id;set_status(tr(L"Nouveau node RMP ajoute avec les parametres du panneau."));InvalidateRect(A.hwnd,NULL,FALSE);}return 1;
    }
    if(!editable){if(hit>=0){A.rmp.selected=hit;rmp_template_from_selected();InvalidateRect(A.hwnd,NULL,FALSE);}MessageBoxW(A.hwnd,tr(L"RMP en lecture seule. Copiez d'abord la MAP dans un mod."),APP_TITLE,MB_ICONWARNING);return 1;}
    if(A.rmpTool==1){
        if(hit<0){set_status(tr(L"Outil LIEN : cliquez d'abord un node source, puis un node destination."));return 1;}
        if(A.rmp.selected<0||A.rmp.selected>=A.rmp.count){A.rmp.selected=hit;rmp_template_from_selected();set_status(tr(L"Source de liaison selectionnee. Cliquez maintenant le node destination."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;}
        if(A.rmp.selected==hit){set_status(tr(L"Source deja selectionnee. Cliquez un autre node pour creer/retirer la liaison."));return 1;}
        rmp_toggle_link(A.rmp.selected,hit);A.rmp.selected=hit;rmp_template_from_selected();InvalidateRect(A.hwnd,NULL,FALSE);return 1;
    }
    if(A.rmpTool>=2&&A.rmpTool<=5){
        if(hit<0){set_status(tr(L"Outil SOCKET : cliquez un node existant."));return 1;}A.rmp.selected=hit;rmp_template_from_selected();int special=-(A.rmpTool); /* 2=N->-2 ... 5=O->-5 */rmp_toggle_selected_socket(special);return 1;
    }
    if(A.rmpTool==6){
        if(hit<0){set_status(tr(L"Outil SUPPR : cliquez le node a supprimer."));return 1;}A.rmp.selected=hit;if(MessageBoxW(A.hwnd,tr(L"Supprimer ce node RMP ?"),APP_TITLE,MB_YESNO|MB_ICONQUESTION)==IDYES)rmp_delete_selected();InvalidateRect(A.hwnd,NULL,FALSE);return 1;
    }
    return 1;
}

/* Analyse geometrique conservative. SneakyAI depend de la visibilite runtime :
   ici on favorise seulement couverts/angles/alternatives comme propositions. */
static int rmp_part_walk_cost(int raw){int lib,local;if(!active_resolve_raw(raw,&lib,&local))return 255;return mcd_u8(lib,local,39);}
static int rmp_tile_walkable(int x,int y,int z){
    MapCell*c=cell_at(x,y,z);if(!c||!c->part[0])return 0;int lib,local;if(!active_resolve_raw(c->part[0],&lib,&local))return 0;if(mcd_u8(lib,local,32)!=0||mcd_u8(lib,local,39)==255)return 0;
    if(c->part[3]&&active_resolve_raw(c->part[3],&lib,&local)&&mcd_u8(lib,local,39)==255)return 0;return 1;
}
static int rmp_wall_blocks_raw(int raw){return raw&&rmp_part_walk_cost(raw)==255;}
static int rmp_edge_open(int x,int y,int z,int nx,int ny){
    if(!rmp_tile_walkable(x,y,z)||!rmp_tile_walkable(nx,ny,z))return 0;MapCell*a=cell_at(x,y,z),*b=cell_at(nx,ny,z);if(nx==x-1&&ny==y)return !rmp_wall_blocks_raw(a->part[1]);if(nx==x+1&&ny==y)return !rmp_wall_blocks_raw(b->part[1]);if(nx==x&&ny==y-1)return !rmp_wall_blocks_raw(a->part[2]);if(nx==x&&ny==y+1)return !rmp_wall_blocks_raw(b->part[2]);return 0;
}
static int rmp_degree(int x,int y,int z){int d=0;if(x>0&&rmp_edge_open(x,y,z,x-1,y))d++;if(x+1<A.map.x&&rmp_edge_open(x,y,z,x+1,y))d++;if(y>0&&rmp_edge_open(x,y,z,x,y-1))d++;if(y+1<A.map.y&&rmp_edge_open(x,y,z,x,y+1))d++;return d;}
static int rmp_cover_score(int x,int y,int z){int c=0;if(x==0||!rmp_edge_open(x,y,z,x-1,y))c++;if(x==A.map.x-1||!rmp_edge_open(x,y,z,x+1,y))c++;if(y==0||!rmp_edge_open(x,y,z,x,y-1))c++;if(y==A.map.y-1||!rmp_edge_open(x,y,z,x,y+1))c++;return c;}
static int proposal_near(int x,int y,int z,int dist){for(int i=0;i<A.rmp.proposalCount;i++){RmpNode*n=&A.rmp.proposals[i];if(n->z==z){int dx=n->x-x,dy=n->y-y;if(dx*dx+dy*dy<=dist*dist)return 1;}}return 0;}
static int rmp_add_proposal(int x,int y,int z,int score){if(A.rmp.proposalCount>=MAX_RMP_NODES||proposal_near(x,y,z,2))return -1;int id=A.rmp.proposalCount++;RmpNode*n=&A.rmp.proposals[id];ZeroMemory(n,sizeof(*n));n->x=x;n->y=y;n->z=z;n->autoScore=score;for(int j=0;j<5;j++)n->links[j]=-1;return id;}
static int rmp_grid_distance(int ax,int ay,int bx,int by,int z,int maxd){
    int w=A.map.x,h=A.map.y,n=w*h;if(n<=0||n>4096)return 9999;int dist[4096],q[4096];for(int i=0;i<n;i++)dist[i]=-1;int qs=0,qe=0,st=ay*w+ax;dist[st]=0;q[qe++]=st;
    while(qs<qe){int v=q[qs++],x=v%w,y=v/w,d=dist[v];if(x==bx&&y==by)return d;if(d>=maxd)continue;const int dx[4]={-1,1,0,0},dy[4]={0,0,-1,1};for(int k=0;k<4;k++){int nx=x+dx[k],ny=y+dy[k];if(nx<0||ny<0||nx>=w||ny>=h)continue;int ni=ny*w+nx;if(dist[ni]>=0||!rmp_edge_open(x,y,z,nx,ny))continue;dist[ni]=d+1;q[qe++]=ni;}}return 9999;
}
static void proposal_add_link(int a,int b){if(a<0||b<0||a>=A.rmp.proposalCount||b>=A.rmp.proposalCount||a==b)return;RmpNode*na=&A.rmp.proposals[a],*nb=&A.rmp.proposals[b];for(int j=0;j<5;j++)if(na->links[j]==b)return;int ia=rmp_free_link(na),ib=rmp_free_link(nb);if(ia>=0&&ib>=0){na->links[ia]=b;nb->links[ib]=a;}}
static void proposal_add_socket(RmpNode*n,int special){for(int j=0;j<5;j++)if(n->links[j]==special)return;int f=rmp_free_link(n);if(f>=0)n->links[f]=special;}
static void rmp_auto_analyze(void){
    if(!A.map.cells||A.scene.active)return;A.rmp.proposalCount=0;
    for(int z=0;z<A.map.z;z++){int w=A.map.x,h=A.map.y,total=w*h;if(total<=0||total>4096)continue;int seen[4096];for(int i=0;i<total;i++)seen[i]=0;
        for(int y=0;y<h;y++)for(int x=0;x<w;x++)if(rmp_tile_walkable(x,y,z)){int deg=rmp_degree(x,y,z),cover=rmp_cover_score(x,y,z);if(deg>=3)rmp_add_proposal(x,y,z,1);else if(cover>=2&&deg>=2)rmp_add_proposal(x,y,z,2);}
        for(int sy=0;sy<h;sy++)for(int sx=0;sx<w;sx++){int si=sy*w+sx;if(seen[si]||!rmp_tile_walkable(sx,sy,z))continue;int q[4096],qs=0,qe=0;q[qe++]=si;seen[si]=1;long sumx=0,sumy=0;int cnt=0;
            while(qs<qe){int v=q[qs++],x=v%w,y=v/w;sumx+=x;sumy+=y;cnt++;const int dx[4]={-1,1,0,0},dy[4]={0,0,-1,1};for(int k=0;k<4;k++){int nx=x+dx[k],ny=y+dy[k];if(nx<0||ny<0||nx>=w||ny>=h)continue;int ni=ny*w+nx;if(seen[ni]||!rmp_edge_open(x,y,z,nx,ny))continue;seen[ni]=1;q[qe++]=ni;}}
            if(cnt){int cx=(int)(sumx/cnt),cy=(int)(sumy/cnt),best=-1,bd=99999;for(int qi=0;qi<qe;qi++){int x=q[qi]%w,y=q[qi]/w,dx=x-cx,dy=y-cy,d=dx*dx+dy*dy;if(d<bd){bd=d;best=q[qi];}}if(best>=0)rmp_add_proposal(best%w,best/w,z,1);}
        }
        int sideBest[4]={-1,-1,-1,-1},sideDist[4]={9999,9999,9999,9999};for(int y=0;y<h;y++)for(int x=0;x<w;x++)if(rmp_tile_walkable(x,y,z)){if(y==0){int d=abs(x-w/2);if(d<sideDist[0]){sideDist[0]=d;sideBest[0]=y*w+x;}}if(x==w-1){int d=abs(y-h/2);if(d<sideDist[1]){sideDist[1]=d;sideBest[1]=y*w+x;}}if(y==h-1){int d=abs(x-w/2);if(d<sideDist[2]){sideDist[2]=d;sideBest[2]=y*w+x;}}if(x==0){int d=abs(y-h/2);if(d<sideDist[3]){sideDist[3]=d;sideBest[3]=y*w+x;}}}
        for(int side=0;side<4;side++)if(sideBest[side]>=0){int x=sideBest[side]%w,y=sideBest[side]/w,id=rmp_add_proposal(x,y,z,1);if(id<0){int bd=9999;for(int k=0;k<A.rmp.proposalCount;k++)if(A.rmp.proposals[k].z==z){int dx=A.rmp.proposals[k].x-x,dy=A.rmp.proposals[k].y-y,d=dx*dx+dy*dy;if(d<bd&&d<=9){bd=d;id=k;}}}if(id>=0)proposal_add_socket(&A.rmp.proposals[id],-2-side);}
    }
    for(int i=0;i<A.rmp.proposalCount;i++){int best[4]={-1,-1,-1,-1},bd[4]={9999,9999,9999,9999};for(int j=0;j<A.rmp.proposalCount;j++){if(i==j||A.rmp.proposals[i].z!=A.rmp.proposals[j].z)continue;int d=rmp_grid_distance(A.rmp.proposals[i].x,A.rmp.proposals[i].y,A.rmp.proposals[j].x,A.rmp.proposals[j].y,A.rmp.proposals[i].z,12);if(d>12)continue;for(int k=0;k<4;k++)if(d<bd[k]){for(int q=3;q>k;q--){bd[q]=bd[q-1];best[q]=best[q-1];}bd[k]=d;best[k]=j;break;}}for(int k=0;k<4;k++)if(best[k]>=0)proposal_add_link(i,best[k]);}
    A.rmp.show=1;wchar_t st[320];_snwprintf(st,319,tr(L"Analyse RMP : %d nodes proposes. Bleu=generique, cyan=couvert/embuscade potentielle OXCE. Rien n'est ecrit."),A.rmp.proposalCount);set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);
}
static void rmp_apply_proposals(void){
    if(A.rmp.proposalCount<=0)return;
    if(current_map_source()!=SRC_MOD){MessageBoxW(A.hwnd,tr(L"Copiez d'abord la MAP dans un mod."),APP_TITLE,MB_ICONWARNING);return;}
    if(A.rmp.count>0){
        if(MessageBoxW(A.hwnd,tr(L"Le RMP contient deja des nodes avec des proprietes de spawn/patrouille.\n\nPar securite, Workshop ne les remplace PAS automatiquement.\n\nAjouter les propositions comme nodes supplementaires ?"),tr(L"Appliquer propositions RMP"),MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return;
        int off=A.rmp.count;
        if(off+A.rmp.proposalCount>MAX_RMP_NODES){MessageBoxW(A.hwnd,tr(L"Trop de nodes pour les IDs RMP 0..250."),APP_TITLE,MB_ICONERROR);return;}
        for(int i=0;i<A.rmp.proposalCount;i++){RmpNode n=A.rmp.proposals[i];memset(n.raw,0,RMP_REC_SIZE);n.raw[21]=(uint8_t)(n.autoScore>=2?2:1);for(int j=0;j<5;j++)if(n.links[j]>=0)n.links[j]+=off;rmp_sync_link_raw(&n);A.rmp.nodes[A.rmp.count++]=n;}
        /* Ponts conservateurs vers le graphe existant : au plus un voisin ancien par nouveau node. */
        for(int i=off;i<A.rmp.count;i++){
            RmpNode*n=&A.rmp.nodes[i];int best=-1,bd=9999;
            for(int j=0;j<off;j++){RmpNode*o=&A.rmp.nodes[j];if(o->z!=n->z)continue;int d=rmp_grid_distance(n->x,n->y,o->x,o->y,n->z,12);if(d<bd){bd=d;best=j;}}
            if(best>=0&&bd<=12&&rmp_free_link(n)>=0&&rmp_free_link(&A.rmp.nodes[best])>=0){int a=rmp_free_link(n),b=rmp_free_link(&A.rmp.nodes[best]);n->links[a]=best;A.rmp.nodes[best].links[b]=i;rmp_sync_link_raw(n);rmp_sync_link_raw(&A.rmp.nodes[best]);}
        }
    }else{
        if(MessageBoxW(A.hwnd,tr(L"Aucun RMP existant : appliquer cette proposition comme nouveau reseau de navigation ?\n\nLes nodes automatiques utilisent des valeurs neutres (pas de spawn prioritaire)."),tr(L"Creer RMP"),MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return;
        A.rmp.count=A.rmp.proposalCount;
        for(int i=0;i<A.rmp.count;i++){A.rmp.nodes[i]=A.rmp.proposals[i];memset(A.rmp.nodes[i].raw,0,RMP_REC_SIZE);A.rmp.nodes[i].raw[21]=(uint8_t)(A.rmp.nodes[i].autoScore>=2?2:1);rmp_sync_link_raw(&A.rmp.nodes[i]);}
    }
    A.rmp.proposalCount=0;A.rmp.loaded=1;A.rmp.dirty=1;A.rmp.selected=-1;
    set_status(tr(L"Propositions appliquees EN MEMOIRE. Vert=praticable, orange=detour, rouge=bloque, violet=vertical. Verifiez avant sauvegarde."));
    InvalidateRect(A.hwnd,NULL,FALSE);
}

static int rmp_save_to_mod(void){
    if(!A.map.cells||current_map_source()!=SRC_MOD){MessageBoxW(A.hwnd,tr(L"Le RMP ne peut etre ecrit que dans un MOD OXCE."),APP_TITLE,MB_ICONWARNING);return 0;}wchar_t mod[96],name[96],root[PATH_CAP],routes[PATH_CAP],target[PATH_CAP];wcsncpy(mod,current_map_origin(),95);mod[95]=0;current_map_name(name,96);_snwprintf(root,PATH_CAP-1,L"%ls\\%ls",A.modsRoot,mod);_snwprintf(routes,PATH_CAP-1,L"%ls\\ROUTES",root);ensure_dir_recursive(routes);_snwprintf(target,PATH_CAP-1,L"%ls\\%ls.RMP",routes,name);
    wchar_t stamp[64],userRoot[PATH_CAP],bakDir[PATH_CAP],bak[PATH_CAP];make_timestamp(stamp,64);wcsncpy(userRoot,A.modsRoot,PATH_CAP-1);path_dirname(userRoot);_snwprintf(bakDir,PATH_CAP-1,L"%ls\\TFTD_Workshop_Backups\\%ls\\%ls",userRoot,mod,stamp);ensure_dir_recursive(bakDir);
    if(path_exists_file(target)){_snwprintf(bak,PATH_CAP-1,L"%ls\\%ls_BEFORE.RMP",bakDir,name);if(!CopyFileW(target,bak,FALSE)){MessageBoxW(A.hwnd,tr(L"Backup RMP impossible : sauvegarde annulee."),APP_TITLE,MB_ICONERROR);return 0;}}else if(A.rmp.path[0]&&path_exists_file(A.rmp.path)){_snwprintf(bak,PATH_CAP-1,L"%ls\\%ls_INHERITED_SOURCE.RMP",bakDir,name);CopyFileW(A.rmp.path,bak,FALSE);}
    size_t sz=(size_t)A.rmp.count*RMP_REC_SIZE;uint8_t*b=(uint8_t*)calloc(sz?sz:1,1);if(!b)return 0;for(int i=0;i<A.rmp.count;i++){RmpNode*n=&A.rmp.nodes[i];uint8_t*r=b+i*RMP_REC_SIZE;memcpy(r,n->raw,RMP_REC_SIZE);r[0]=(uint8_t)n->y;r[1]=(uint8_t)n->x;r[2]=(uint8_t)(A.map.z-1-n->z);for(int j=0;j<5;j++){int v=n->links[j];r[4+j*3]=(uint8_t)(v<0?v+256:v);}}
    int ok=write_all(target,b,(DWORD)sz);free(b);if(!ok){MessageBoxW(A.hwnd,tr(L"Echec ecriture RMP."),APP_TITLE,MB_ICONERROR);return 0;}wcsncpy(A.rmp.path,target,PATH_CAP-1);A.rmp.inherited=0;A.rmp.dirty=0;_snwprintf(A.rmp.sourceLabel,159,tr(L"MOD OXCE / %ls"),mod);set_status(tr(L"RMP enregistre dans le mod avec backup automatique."));return 1;
}


typedef struct {
    wchar_t biome[96],block[96],path[PATH_CAP],routePath[PATH_CAP];
    wchar_t datasets[MAX_EXPORT_DATASETS][96];
    int dsCount,addBlock;
    int routeCreate; /* 0 inherited/existing, 1 copy source RMP, 2 create empty placeholder */
    wchar_t added[4096];
    uint8_t*bytes;DWORD size;
} ExportPlanTarget;
static void free_export_plan(ExportPlanTarget*p,int n){if(!p)return;for(int i=0;i<n;i++)free(p[i].bytes);free(p);}
static void rollback_export_state(const wchar_t*mod,ExportPlanTarget*p,int n,const wchar_t*backupDir){
    for(int i=0;i<n;i++){wchar_t bn[96],src[PATH_CAP];path_basename_noext(p[i].path,bn,96);_snwprintf(src,PATH_CAP-1,L"%ls\\TARGET_BEFORE_%ls.MAP",backupDir,bn);if(path_exists_file(src))CopyFileW(src,p[i].path,FALSE);else DeleteFileW(p[i].path);if(p[i].routeCreate)DeleteFileW(p[i].routePath);}
    wchar_t mr[PATH_CAP],mf[PATH_CAP],ru[PATH_CAP],src[PATH_CAP];export_paths(mod,mr,mf,ru);_snwprintf(src,PATH_CAP-1,L"%ls\\workshop_exports_before.ini",backupDir);if(path_exists_file(src))CopyFileW(src,mf,FALSE);else DeleteFileW(mf);_snwprintf(src,PATH_CAP-1,L"%ls\\000_workshop_generated_before.rul",backupDir);if(path_exists_file(src))CopyFileW(src,ru,FALSE);else DeleteFileW(ru);
}
static int block_first_group_from_rule(const wchar_t*path,const wchar_t*block){
    if(!path||!path[0]||!block||!block[0])return 0;DWORD sz=0;uint8_t*raw=read_all(path,&sz);if(!raw)return 0;char*txt=(char*)malloc((size_t)sz+1);if(!txt){free(raw);return 0;}memcpy(txt,raw,sz);txt[sz]=0;free(raw);char b8[256];WideCharToMultiByte(CP_UTF8,0,block,-1,b8,256,NULL,NULL);char needle[320];_snprintf(needle,319,"- name: %s",b8);char*pos=strstr(txt,needle);int result=0;if(pos){char*end=strstr(pos+strlen(needle),"\n  - name:");if(!end)end=txt+strlen(txt);char*g=strstr(pos,"groups:");if(g&&g<end){g+=7;while(g<end&&(*g==' '||*g=='\t'))g++;if(*g=='['){g++;result=atoi(g);}else{char*n=strchr(g,'\n');if(n&&n<end){n++;while(n<end&&(*n==' '||*n=='\t'))n++;if(*n=='-'){n++;while(*n==' ')n++;result=atoi(n);}}}}}free(txt);return result;
}
static int export_execute(const wchar_t*mod,const wchar_t*baseName,wchar_t biomes[][96],int biomeCount,int group,wchar_t*result,int resultCap){
    if(!A.map.cells||!valid_block_name(baseName)||biomeCount<=0)return 0;wchar_t err[512]=L"";SceneCell*logical=map_to_logical(err,512);if(!logical){MessageBoxW(EUI.hwnd?EUI.hwnd:A.hwnd,err,APP_TITLE,MB_ICONERROR);return 0;}
    ExportPlanTarget*plan=(ExportPlanTarget*)calloc((size_t)biomeCount,sizeof(ExportPlanTarget));if(!plan){free(logical);return 0;}wchar_t sourceBiome[96];current_map_biome(sourceBiome,96);int baseAssigned=0,resultIndex=0;
    for(int i=0;i<biomeCount;i++){wcsncpy(plan[i].biome,biomes[i],95);int useBase=(_wcsicmp(biomes[i],sourceBiome)==0);if(useBase){baseAssigned=1;resultIndex=i;}if(useBase)wcsncpy(plan[i].block,baseName,95);else _snwprintf(plan[i].block,95,L"%ls_%ls",baseName,biomes[i]);plan[i].block[95]=0;_snwprintf(plan[i].path,PATH_CAP-1,L"%ls\\%ls\\Maps\\%ls.MAP",A.modsRoot,mod,plan[i].block);plan[i].path[PATH_CAP-1]=0;}
    if(!baseAssigned&&biomeCount==1){wcsncpy(plan[0].block,baseName,95);plan[0].block[95]=0;_snwprintf(plan[0].path,PATH_CAP-1,L"%ls\\%ls\\Maps\\%ls.MAP",A.modsRoot,mod,plan[0].block);}
    wchar_t sourceRmp[PATH_CAP];int haveSourceRmp=current_source_rmp(sourceRmp,PATH_CAP);
    for(int i=0;i<biomeCount;i++){_snwprintf(plan[i].routePath,PATH_CAP-1,L"%ls\\%ls\\ROUTES\\%ls.RMP",A.modsRoot,mod,plan[i].block);plan[i].routePath[PATH_CAP-1]=0;}
    /* PRE-FIGHT COMPLET : aucun fichier n'est encore modifie. */
    wchar_t preview[8192];_snwprintf(preview,8191,tr(L"PRE-FLIGHT - AUCUN FICHIER N'A ENCORE ETE MODIFIE\n\nMod : %ls\n"),mod);
    for(int i=0;i<biomeCount;i++){if(!build_target_datasets(plan[i].biome,mod,logical,plan[i].datasets,&plan[i].dsCount,plan[i].added,4096,err,512)){free(logical);free_export_plan(plan,biomeCount);MessageBoxW(EUI.hwnd?EUI.hwnd:A.hwnd,err,APP_TITLE,MB_ICONERROR);return 0;}if(!encode_logical_map(logical,mod,plan[i].datasets,plan[i].dsCount,&plan[i].bytes,&plan[i].size,err,512)){free(logical);free_export_plan(plan,biomeCount);MessageBoxW(EUI.hwnd?EUI.hwnd:A.hwnd,err,APP_TITLE,MB_ICONERROR);return 0;}plan[i].addBlock=!terrain_has_block_base(plan[i].biome,plan[i].block,mod);
        if(plan[i].addBlock&&!path_exists_file(plan[i].routePath))plan[i].routeCreate=haveSourceRmp?1:2;
        wchar_t routeInfo[220];if(!plan[i].addBlock)_snwprintf(routeInfo,219,tr(L"RMP : route existante/base conservee"));else if(!plan[i].routeCreate)_snwprintf(routeInfo,219,tr(L"RMP : fichier deja present dans le mod, conserve"));else if(plan[i].routeCreate==1)_snwprintf(routeInfo,219,tr(L"RMP : copie du bloc source (a valider si la geometrie a change)"));else _snwprintf(routeInfo,219,tr(L"RMP : placeholder vide cree - routage IA a faire dans une future etape"));
        wchar_t line[1600];_snwprintf(line,1599,tr(L"\n%ls -> %ls.MAP\n  %ls\n  Datasets ajoutes : %ls\n  %ls\n"),plan[i].biome,plan[i].block,plan[i].addBlock?tr(L"Nouveau mapBlock ajoute au biome"):tr(L"Bloc deja declare : seule la MAP sera remplacee"),plan[i].added[0]?plan[i].added:tr(L"(aucun)"),routeInfo);if(wcslen(preview)+wcslen(line)<8100)wcscat(preview,line);}
    wcscat(preview,tr(L"\nGaranties :\n- backup horodate avant ecriture\n- TFTD ORIGINAL/OXCE STANDARD : 0 fichier modifie\n- rulesets existants du mod : 0 fichier modifie\n- PNG/MCD externes : references dans leur dataset d'origine, aucune copie\n\nExecuter cet export ?"));
    if(MessageBoxW(EUI.hwnd?EUI.hwnd:A.hwnd,preview,tr(L"Workshop - verification finale"),MB_YESNO|MB_ICONQUESTION)!=IDYES){free(logical);free_export_plan(plan,biomeCount);return 0;}
    wchar_t targetPaths[MAX_EXPORT_BIOMES][PATH_CAP];for(int i=0;i<biomeCount;i++)wcsncpy(targetPaths[i],plan[i].path,PATH_CAP-1);wchar_t backupDir[PATH_CAP];if(!backup_export_state(mod,A.map.path,targetPaths,biomeCount,backupDir,PATH_CAP)){free(logical);free_export_plan(plan,biomeCount);MessageBoxW(EUI.hwnd?EUI.hwnd:A.hwnd,tr(L"Impossible de creer le backup automatique. Export annule."),APP_TITLE,MB_ICONERROR);return 0;}
    wchar_t mapsDir[PATH_CAP];_snwprintf(mapsDir,PATH_CAP-1,L"%ls\\%ls\\Maps",A.modsRoot,mod);if(!ensure_dir_recursive(mapsDir)){free(logical);free_export_plan(plan,biomeCount);return 0;}
    int ok=1;for(int i=0;i<biomeCount;i++){if(!write_all(plan[i].path,plan[i].bytes,plan[i].size)){ok=0;break;}if(!manifest_update_terrain(mod,plan[i].biome,plan[i].datasets,plan[i].dsCount,plan[i].added,plan[i].block,plan[i].addBlock,group)){ok=0;break;}}
    if(ok){wchar_t routesDir[PATH_CAP];_snwprintf(routesDir,PATH_CAP-1,L"%ls\\%ls\\ROUTES",A.modsRoot,mod);if(!ensure_dir_recursive(routesDir))ok=0;for(int i=0;ok&&i<biomeCount;i++){if(plan[i].routeCreate==1){if(!CopyFileW(sourceRmp,plan[i].routePath,TRUE))ok=0;}else if(plan[i].routeCreate==2){if(!write_all(plan[i].routePath,"",0))ok=0;}}}
    if(ok)ok=regenerate_workshop_rules(mod,err,512);if(!ok){rollback_export_state(mod,plan,biomeCount,backupDir);free(logical);free_export_plan(plan,biomeCount);MessageBoxW(EUI.hwnd?EUI.hwnd:A.hwnd,tr(L"Une erreur est survenue pendant l'ecriture. Le Workshop a restaure automatiquement l'etat precedent depuis le backup."),APP_TITLE,MB_ICONERROR);return 0;}
    wchar_t mp[PATH_CAP];_snwprintf(mp,PATH_CAP-1,L"%ls\\manifest.txt",backupDir);char man[8192],s8[2048],m8[256],b8[2048];wide_utf8(A.map.path,s8,2048);wide_utf8(mod,m8,256);wide_utf8(backupDir,b8,2048);int ml=_snprintf(man,sizeof(man)-1,"TFTD Workshop V2.0 backup\r\nSource: %s\r\nMod: %s\r\nBackup: %s\r\n",s8,m8,b8);for(int i=0;i<biomeCount&&ml<(int)sizeof(man)-512;i++){char p8[2048],bi8[256];wide_utf8(plan[i].path,p8,2048);wide_utf8(plan[i].biome,bi8,256);ml+=_snprintf(man+ml,sizeof(man)-ml-1,"Target: %s => %s\r\n",bi8,p8);}write_all(mp,man,(DWORD)strlen(man));
    wcsncpy(result,plan[resultIndex].path,resultCap-1);result[resultCap-1]=0;free(logical);free_export_plan(plan,biomeCount);return 1;
}
static int safe_save_current_mod(void){
    if(current_map_source()!=SRC_MOD||!A.map.path[0])return 0;
    /* Ctrl+S sur un bloc de mod ne doit jamais perdre un RMP modifie pendant la reindexation. */
    if(A.rmp.dirty&&!rmp_save_to_mod())return 0;
    wchar_t biome[96],name[96],mod[96],target[1][PATH_CAP],backup[PATH_CAP];current_map_biome(biome,96);current_map_name(name,96);wcsncpy(mod,current_map_origin(),95);mod[95]=0;wcsncpy(target[0],A.map.path,PATH_CAP-1);target[0][PATH_CAP-1]=0;if(!backup_export_state(mod,A.map.path,target,1,backup,PATH_CAP)){MessageBoxW(A.hwnd,tr(L"Backup impossible : sauvegarde annulee."),APP_TITLE,MB_ICONERROR);return 0;}
    if(!save_map_file(A.map.path))return 0;wchar_t ds[MAX_EXPORT_DATASETS][96],added[4096]=L"";int dn=0;for(int i=0;i<A.activeCount&&dn<MAX_EXPORT_DATASETS;i++){int lib=A.active[i].libIndex;if(lib<0)continue;wcsncpy(ds[dn],A.library[lib].name,95);ds[dn][95]=0;dn++;}
    int pi=terrain_profile_base(biome,mod);if(pi>=0){MapProfile*q=&A.profiles[pi];for(int i=0;i<dn;i++)if(!list_has_w(q->dataSets,q->dataSetCount,ds[i]))csv_add_unique(added,4096,ds[i]);}
    wchar_t er[512];if(!manifest_update_terrain(mod,biome,ds,dn,added,name,!terrain_has_block_base(biome,name,mod),-1)||!regenerate_workshop_rules(mod,er,512)){ExportPlanTarget p;ZeroMemory(&p,sizeof(p));wcsncpy(p.path,A.map.path,PATH_CAP-1);rollback_export_state(mod,&p,1,backup);A.map.dirty=1;MessageBoxW(A.hwnd,tr(L"Synchronisation ruleset impossible. L'etat precedent a ete restaure automatiquement."),APP_TITLE,MB_ICONERROR);return 0;}reindex_resources();set_status(tr(L"MAP du mod sauvegardee : profils/datasets reindexes immediatement."));return 1;
}

static int export_mod_combo_is_new_test(void){
    if(!EUI.modCombo)return 0;int sel=(int)SendMessageW(EUI.modCombo,CB_GETCURSEL,0,0);if(sel<0)return 0;return (int)SendMessageW(EUI.modCombo,CB_GETITEMDATA,sel,0)==1;
}
static void export_update_testmod_state(void){
    if(!EUI.testModEdit)return;EnableWindow(EUI.testModEdit,export_mod_combo_is_new_test()?TRUE:FALSE);
}
static void export_fill_controls(void){
    if(!EUI.hwnd)return;SendMessageW(EUI.modCombo,CB_RESETCONTENT,0,0);wchar_t pat[PATH_CAP];_snwprintf(pat,PATH_CAP-1,L"%ls\\*",A.modsRoot);WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);int sel=-1;const wchar_t*cur=current_map_origin();
    if(h!=INVALID_HANDLE_VALUE){do{if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||wcscmp(fd.cFileName,L".")==0||wcscmp(fd.cFileName,L"..")==0)continue;wchar_t meta[PATH_CAP];_snwprintf(meta,PATH_CAP-1,L"%ls\\%ls\\metadata.yml",A.modsRoot,fd.cFileName);if(!path_exists_file(meta))continue;int j=(int)SendMessageW(EUI.modCombo,CB_ADDSTRING,0,(LPARAM)fd.cFileName);SendMessageW(EUI.modCombo,CB_SETITEMDATA,j,0);if(current_map_source()==SRC_MOD&&_wcsicmp(fd.cFileName,cur)==0)sel=j;}while(FindNextFileW(h,&fd));FindClose(h);}
    int special=(int)SendMessageW(EUI.modCombo,CB_ADDSTRING,0,(LPARAM)tr(L"+ CREER UN NOUVEAU MOD DE TEST..."));SendMessageW(EUI.modCombo,CB_SETITEMDATA,special,1);if(current_map_source()!=SRC_MOD||sel<0)sel=special;SendMessageW(EUI.modCombo,CB_SETCURSEL,sel,0);
    wchar_t nm[96];current_map_name(nm,96);SetWindowTextW(EUI.nameEdit,nm);int defGroup=0;if(A.autoProfileIndex>=0&&A.autoProfileIndex<A.profileCount)defGroup=block_first_group_from_rule(A.profiles[A.autoProfileIndex].rulePath,nm);wchar_t gtxt[32];_snwprintf(gtxt,31,L"%d",defGroup);SetWindowTextW(EUI.groupEdit,gtxt);
    wchar_t testName[180];_snwprintf(testName,179,L"JM_TEST_%ls",nm);SetWindowTextW(EUI.testModEdit,testName);export_update_testmod_state();
    SendMessageW(EUI.biomeList,LB_RESETCONTENT,0,0);wchar_t uniq[MAX_EXPORT_BIOMES][96];int un=0;
    for(int i=0;i<A.profileCount&&un<MAX_EXPORT_BIOMES;i++){MapProfile*q=&A.profiles[i];if(q->kind!=0||profile_is_generated(q))continue;wchar_t p[96];map_prefix(q->mapName,p,96);if(!p[0]||list_has_w(uniq,un,p))continue;wcsncpy(uniq[un],p,95);uniq[un][95]=0;un++;}
    for(int i=0;i<un;i++)for(int j=i+1;j<un;j++)if(_wcsicmp(uniq[i],uniq[j])>0){wchar_t t[96];wcscpy(t,uniq[i]);wcscpy(uniq[i],uniq[j]);wcscpy(uniq[j],t);}
    wchar_t cb[96];current_map_biome(cb,96);for(int i=0;i<un;i++){int j=(int)SendMessageW(EUI.biomeList,LB_ADDSTRING,0,(LPARAM)uniq[i]);if(_wcsicmp(uniq[i],cb)==0)SendMessageW(EUI.biomeList,LB_SETSEL,TRUE,j);}
}
static LRESULT CALLBACK export_wndproc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    (void)lp;switch(msg){case WM_CREATE:{EUI.hwnd=hwnd;HFONT f=(HFONT)GetStockObject(DEFAULT_GUI_FONT);int y=18;CreateWindowW(L"STATIC",tr(L"EXPORT SECURISE : TFTD ORIGINAL / OXCE STANDARD restent intouchables."),WS_CHILD|WS_VISIBLE,18,y,630,22,hwnd,NULL,NULL,NULL);y+=38;
        CreateWindowW(L"STATIC",tr(L"Mod cible"),WS_CHILD|WS_VISIBLE,18,y,130,20,hwnd,NULL,NULL,NULL);EUI.modCombo=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,160,y-4,420,260,hwnd,(HMENU)CID_EXPORT_MOD,NULL,NULL);y+=40;
        CreateWindowW(L"STATIC",tr(L"Nom nouveau mod de test"),WS_CHILD|WS_VISIBLE,18,y,180,20,hwnd,NULL,NULL,NULL);EUI.testModEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,210,y-4,370,25,hwnd,(HMENU)CID_EXPORT_TESTMOD,NULL,NULL);y+=40;
        CreateWindowW(L"STATIC",tr(L"Nom du macrobloc"),WS_CHILD|WS_VISIBLE,18,y,140,20,hwnd,NULL,NULL,NULL);EUI.nameEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_UPPERCASE,160,y-4,260,25,hwnd,(HMENU)CID_EXPORT_NAME,NULL,NULL);y+=42;
        CreateWindowW(L"STATIC",tr(L"Biome(s) cible(s) - Ctrl/clic pour plusieurs"),WS_CHILD|WS_VISIBLE,18,y,310,20,hwnd,NULL,NULL,NULL);y+=24;EUI.biomeList=CreateWindowExW(WS_EX_CLIENTEDGE,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|LBS_EXTENDEDSEL|WS_VSCROLL|LBS_NOINTEGRALHEIGHT,18,y,612,220,hwnd,(HMENU)CID_EXPORT_BIOMES,NULL,NULL);y+=236;
        CreateWindowW(L"STATIC",tr(L"Groupe de generation si le bloc est nouveau"),WS_CHILD|WS_VISIBLE,18,y,280,20,hwnd,NULL,NULL,NULL);EUI.groupEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"0",WS_CHILD|WS_VISIBLE|ES_NUMBER,310,y-4,80,24,hwnd,(HMENU)CID_EXPORT_GROUP,NULL,NULL);y+=38;
        EUI.infoText=CreateWindowW(L"STATIC",tr(L"Choisissez + CREER UN NOUVEAU MOD DE TEST pour fabriquer un bac a sable OXCE separe. Le Workshop cree metadata.yml, Maps, ROUTES, Ruleset et Workshop. Aucun dataset/PNG n'est recopie inutilement."),WS_CHILD|WS_VISIBLE,18,y,620,60,hwnd,NULL,NULL,NULL);y+=66;
        CreateWindowW(L"BUTTON",tr(L"Exporter en securite"),WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,150,y,190,34,hwnd,(HMENU)CID_EXPORT_GO,NULL,NULL);CreateWindowW(L"BUTTON",tr(L"Annuler"),WS_CHILD|WS_VISIBLE,370,y,120,34,hwnd,(HMENU)CID_EXPORT_CANCEL,NULL,NULL);
        for(HWND c=GetWindow(hwnd,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))SendMessageW(c,WM_SETFONT,(WPARAM)f,TRUE);export_fill_controls();return 0;}
    case WM_COMMAND:
        if(LOWORD(wp)==CID_EXPORT_MOD&&HIWORD(wp)==CBN_SELCHANGE){export_update_testmod_state();return 0;}
        if(LOWORD(wp)==CID_EXPORT_CANCEL){DestroyWindow(hwnd);return 0;}
        if(LOWORD(wp)==CID_EXPORT_GO){wchar_t mod[96],name[96],createdRoot[PATH_CAP];createdRoot[0]=0;int createdTest=0;int ms=(int)SendMessageW(EUI.modCombo,CB_GETCURSEL,0,0);if(ms<0){MessageBoxW(hwnd,tr(L"Choisissez un mod cible."),APP_TITLE,MB_ICONWARNING);return 0;}int createTest=(int)SendMessageW(EUI.modCombo,CB_GETITEMDATA,ms,0)==1;GetWindowTextW(EUI.nameEdit,name,96);if(!valid_block_name(name)){MessageBoxW(hwnd,tr(L"Nom invalide. Utilisez uniquement lettres, chiffres, _ ou -."),APP_TITLE,MB_ICONWARNING);return 0;}
        if(createTest){wchar_t tf[96];GetWindowTextW(EUI.testModEdit,tf,96);if(!valid_folder_component(tf)){MessageBoxW(hwnd,tr(L"Nom de mod de test invalide. Evitez les caracteres interdits Windows."),APP_TITLE,MB_ICONWARNING);return 0;}int cr=create_test_mod(tf,name,createdRoot,PATH_CAP);if(cr==-1){MessageBoxW(hwnd,tr(L"Un dossier portant ce nom existe deja. Choisissez un autre nom pour ce nouveau mod de test."),APP_TITLE,MB_ICONWARNING);return 0;}if(cr!=1){MessageBoxW(hwnd,tr(L"Impossible de creer le nouveau mod de test."),APP_TITLE,MB_ICONERROR);return 0;}wcsncpy(mod,tf,95);mod[95]=0;createdTest=1;}else{SendMessageW(EUI.modCombo,CB_GETLBTEXT,ms,(LPARAM)mod);}
        int n=(int)SendMessageW(EUI.biomeList,LB_GETSELCOUNT,0,0);if(n<=0||n>MAX_EXPORT_BIOMES){if(createdTest)delete_tree_recursive(createdRoot);MessageBoxW(hwnd,tr(L"Selectionnez au moins un biome."),APP_TITLE,MB_ICONWARNING);return 0;}int ids[MAX_EXPORT_BIOMES];SendMessageW(EUI.biomeList,LB_GETSELITEMS,n,(LPARAM)ids);wchar_t biomes[MAX_EXPORT_BIOMES][96];for(int i=0;i<n;i++)SendMessageW(EUI.biomeList,LB_GETTEXT,ids[i],(LPARAM)biomes[i]);wchar_t gb[32];GetWindowTextW(EUI.groupEdit,gb,32);int group=_wtoi(gb);
        wchar_t result[PATH_CAP];if(export_execute(mod,name,biomes,n,group,result,PATH_CAP)){DestroyWindow(hwnd);reindex_resources();load_map_file(result);set_status(createdTest?tr(L"Mod de test cree : copie ouverte et editable."):tr(L"Export termine : copie MOD ouverte et editable."));MessageBoxW(A.hwnd,createdTest?tr(L"Mod de test cree avec succes.\n\nIl apparait maintenant dans MODS OXCE. Activez-le dans OXCE pour tester le macrobloc. Une fois valide, re-exportez ensuite cette MAP vers TFTD PNG remastered."):tr(L"Export termine.\n\nBackup : user\\\\TFTD_Workshop_Backups\nLa copie du mod est maintenant ouverte et editable."),APP_TITLE,MB_ICONINFORMATION);}else if(createdTest){delete_tree_recursive(createdRoot);}return 0;}break;
    case WM_DESTROY:EUI.hwnd=NULL;return 0;}return DefWindowProcW(hwnd,msg,wp,lp);
}
static void open_export_wizard(void){
    if(A.scene.active&&A.scene.geoCount){MessageBoxW(A.hwnd,tr(L"Cette scene contient des reliefs GEO. Enregistrez-la en .JMW pour conserver leur geometrie. La conversion GEO vers MAP/OXCE n'est pas encore disponible."),APP_TITLE,MB_ICONINFORMATION);return;}
    if(!A.map.cells){MessageBoxW(A.hwnd,tr(L"Ouvrez ou creez d'abord un macrobloc."),APP_TITLE,MB_ICONINFORMATION);return;}if(!A.modsRoot[0]||!path_exists_dir(A.modsRoot)){MessageBoxW(A.hwnd,tr(L"Configurez d'abord Ressources > Emplacement MODS OXCE."),APP_TITLE,MB_ICONWARNING);return;}if(EUI.hwnd){SetForegroundWindow(EUI.hwnd);return;}
    static int reg=0;if(!reg){WNDCLASSEXW wc;ZeroMemory(&wc,sizeof(wc));wc.cbSize=sizeof(wc);wc.lpfnWndProc=export_wndproc;wc.hInstance=GetModuleHandleW(NULL);wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"TFTDWorkshopExport25";RegisterClassExW(&wc);reg=1;}
    EUI.hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,L"TFTDWorkshopExport25",tr(L"Exporter / copier le macrobloc vers un mod"),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,680,710,A.hwnd,NULL,GetModuleHandleW(NULL),NULL);ShowWindow(EUI.hwnd,SW_SHOW);UpdateWindow(EUI.hwnd);
}

static void open_composer(void){
    if(CUI.hwnd){SetForegroundWindow(CUI.hwnd);return;}static int registered=0;if(!registered){WNDCLASSEXW wc;ZeroMemory(&wc,sizeof(wc));wc.cbSize=sizeof(wc);wc.lpfnWndProc=composer_wndproc;wc.hInstance=GetModuleHandleW(NULL);wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"TFTDWorkshopComposer273";RegisterClassExW(&wc);registered=1;}CUI.hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,L"TFTDWorkshopComposer273",tr(L"Generation de carte - Fidele OpenXcom / Libre"),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,620,485,A.hwnd,NULL,GetModuleHandleW(NULL),NULL);ShowWindow(CUI.hwnd,SW_SHOW);UpdateWindow(CUI.hwnd);populate_composer_combos();
}
#pragma pack(push,1)
typedef struct { char magic[4]; uint16_t x,y,z,setCount; int32_t craftX,craftY,craftW,craftH; int32_t slotActive,slotX,slotY,slotW,slotH,usoInserted; wchar_t title[160]; wchar_t slotName[96]; } JmwHeaderV1;
typedef struct { char magic[4]; uint16_t x,y,z,setCount; int32_t craftX,craftY,craftW,craftH; int32_t slotActive,slotX,slotY,slotW,slotH,usoInserted; wchar_t title[160]; wchar_t slotName[96]; int32_t usoSource; wchar_t usoOrigin[96]; } JmwHeaderV2;
typedef struct { char magic[4]; uint16_t x,y,z,setCount; int32_t craftX,craftY,craftW,craftH; int32_t slotActive,slotX,slotY,slotW,slotH,usoInserted; wchar_t title[160]; wchar_t slotName[96]; int32_t usoSource; wchar_t usoOrigin[96]; uint32_t planCellSize; } JmwHeaderV3;
typedef struct { int32_t source; wchar_t name[96]; wchar_t origin[96]; } JmwSetV2;
#pragma pack(pop)
static int scene_save_project_file(const wchar_t*path){
    if(!A.scene.active||!A.scene.cells)return 0;int used[MAX_LIBRARY];for(int i=0;i<MAX_LIBRARY;i++)used[i]=-1;int cnt=0;
    for(size_t i=0,n=(size_t)A.scene.x*A.scene.y*A.scene.z;i<n;i++)for(int p=0;p<4;p++){int lib=A.scene.cells[i].lib[p];if(lib<0)continue;int found=-1;for(int k=0;k<cnt;k++)if(used[k]==lib){found=k;break;}if(found<0&&cnt<MAX_LIBRARY)used[cnt++]=lib;}
    JmwHeaderV3 h;ZeroMemory(&h,sizeof(h));memcpy(h.magic,A.scene.geoCount?"JMW4":"JMW3",4);h.x=A.scene.x;h.y=A.scene.y;h.z=A.scene.z;h.setCount=cnt;h.craftX=A.scene.craftX;h.craftY=A.scene.craftY;h.craftW=A.scene.craftW;h.craftH=A.scene.craftH;h.slotActive=A.scene.usoSlotActive;h.slotX=A.scene.usoSlotX;h.slotY=A.scene.usoSlotY;h.slotW=A.scene.usoSlotW;h.slotH=A.scene.usoSlotH;h.usoInserted=A.scene.usoInserted;wcsncpy(h.title,A.scene.title,159);wcsncpy(h.slotName,A.scene.usoSlotName,95);h.usoSource=-1;h.planCellSize=(uint32_t)sizeof(PlanCell);
    if(A.scene.usoMapIndex>=0&&A.scene.usoMapIndex<A.mapCount){h.usoSource=A.maps[A.scene.usoMapIndex].source;wcsncpy(h.usoOrigin,A.maps[A.scene.usoMapIndex].origin,95);}
    size_t cells=(size_t)A.scene.x*A.scene.y*A.scene.z;size_t total=sizeof(h)+(size_t)cnt*sizeof(JmwSetV2)+cells*16+cells*sizeof(PlanCell)+scene_geo_bytes(&A.scene);uint8_t*b=(uint8_t*)calloc(total,1);if(!b)return 0;size_t off=0;memcpy(b+off,&h,sizeof(h));off+=sizeof(h);
    for(int k=0;k<cnt;k++){JmwSetV2 e;ZeroMemory(&e,sizeof(e));LibrarySet*ls=&A.library[used[k]];e.source=ls->source;wcsncpy(e.name,ls->name,95);wcsncpy(e.origin,ls->origin,95);memcpy(b+off,&e,sizeof(e));off+=sizeof(e);}
    for(size_t i=0;i<cells;i++)for(int p=0;p<4;p++){int sid=-1,local=A.scene.cells[i].local[p],lib=A.scene.cells[i].lib[p];if(lib>=0){for(int k=0;k<cnt;k++)if(used[k]==lib){sid=k;break;}}int16_t s16=(int16_t)sid;uint16_t l16=(uint16_t)(local<0?0:local);memcpy(b+off,&s16,2);off+=2;memcpy(b+off,&l16,2);off+=2;}
    if(A.scene.plan)memcpy(b+off,A.scene.plan,cells*sizeof(PlanCell));off+=cells*sizeof(PlanCell);if(A.scene.geoCount){scene_geo_write(b+off,&A.scene);off+=scene_geo_bytes(&A.scene);}int ok=(off==total)&&write_all(path,b,(DWORD)total);free(b);if(ok)A.scene.dirty=0;return ok;
}
static int scene_load_project_file(const wchar_t*path){
    DWORD sz=0;uint8_t*b=read_all(path,&sz);if(!b||sz<4){free(b);return 0;}int v4=memcmp(b,"JMW4",4)==0,v3=memcmp(b,"JMW3",4)==0||v4,v2=memcmp(b,"JMW2",4)==0,v1=memcmp(b,"JMW1",4)==0;if(!v1&&!v2&&!v3){free(b);return 0;}
    uint16_t x=0,y=0,z=0,setCount=0;int craftX=0,craftY=0,craftW=0,craftH=0,slotActive=0,slotX=0,slotY=0,slotW=0,slotH=0,usoInserted=0,usoSource=-1;uint32_t planCellSize=0;wchar_t title[160]=L"",slotName[96]=L"",usoOrigin[96]=L"";size_t off=0;
    if(v3){if(sz<sizeof(JmwHeaderV3)){free(b);return 0;}JmwHeaderV3 h;memcpy(&h,b,sizeof(h));x=h.x;y=h.y;z=h.z;setCount=h.setCount;craftX=h.craftX;craftY=h.craftY;craftW=h.craftW;craftH=h.craftH;slotActive=h.slotActive;slotX=h.slotX;slotY=h.slotY;slotW=h.slotW;slotH=h.slotH;usoInserted=h.usoInserted;usoSource=h.usoSource;planCellSize=h.planCellSize;wcsncpy(title,h.title,159);wcsncpy(slotName,h.slotName,95);wcsncpy(usoOrigin,h.usoOrigin,95);off=sizeof(h);}
    else if(v2){if(sz<sizeof(JmwHeaderV2)){free(b);return 0;}JmwHeaderV2 h;memcpy(&h,b,sizeof(h));x=h.x;y=h.y;z=h.z;setCount=h.setCount;craftX=h.craftX;craftY=h.craftY;craftW=h.craftW;craftH=h.craftH;slotActive=h.slotActive;slotX=h.slotX;slotY=h.slotY;slotW=h.slotW;slotH=h.slotH;usoInserted=h.usoInserted;usoSource=h.usoSource;wcsncpy(title,h.title,159);wcsncpy(slotName,h.slotName,95);wcsncpy(usoOrigin,h.usoOrigin,95);off=sizeof(h);}
    else{if(sz<sizeof(JmwHeaderV1)){free(b);return 0;}JmwHeaderV1 h;memcpy(&h,b,sizeof(h));x=h.x;y=h.y;z=h.z;setCount=h.setCount;craftX=h.craftX;craftY=h.craftY;craftW=h.craftW;craftH=h.craftH;slotActive=h.slotActive;slotX=h.slotX;slotY=h.slotY;slotW=h.slotW;slotH=h.slotH;usoInserted=h.usoInserted;wcsncpy(title,h.title,159);wcsncpy(slotName,h.slotName,95);off=sizeof(h);}
    if(!x||!y||!z||setCount>MAX_LIBRARY){free(b);return 0;}size_t cells=(size_t)x*y*z;size_t table=(v2||v3)?(size_t)setCount*sizeof(JmwSetV2):(size_t)setCount*96*sizeof(wchar_t);size_t planBytes=v3?(size_t)planCellSize*cells:0,sizeNeed=off+table+cells*16+planBytes;if(sizeNeed>sz||(v3&&(planCellSize==0||planCellSize>64))){free(b);return 0;}SceneDoc geoStaged={.x=x,.y=y,.z=z};if(v4&&!scene_geo_read(b+sizeNeed,sz-sizeNeed,&geoStaged)){scene_geo_free(&geoStaged);free(b);return 0;}if(!scene_alloc(x,y,z)){scene_geo_free(&geoStaged);free(b);return 0;}A.scene.geo=geoStaged.geo;A.scene.geoCount=geoStaged.geoCount;A.scene.geoLookup=geoStaged.geoLookup;A.scene.geoDecor=geoStaged.geoDecor;A.scene.geoDecorCount=geoStaged.geoDecorCount;int map[MAX_LIBRARY];for(int k=0;k<MAX_LIBRARY;k++)map[k]=-1;
    for(int k=0;k<setCount;k++){if(v2||v3){JmwSetV2 e;memcpy(&e,b+off,sizeof(e));off+=sizeof(e);e.name[95]=0;e.origin[95]=0;map[k]=library_find_name_ctx(e.name,e.source,e.origin);}else{wchar_t name[96];memcpy(name,b+off,96*sizeof(wchar_t));name[95]=0;off+=96*sizeof(wchar_t);map[k]=library_find_name(name);}}
    for(size_t i=0;i<cells;i++)for(int p=0;p<4;p++){int16_t sid;uint16_t local;memcpy(&sid,b+off,2);off+=2;memcpy(&local,b+off,2);off+=2;if(sid>=0&&sid<setCount&&map[sid]>=0){A.scene.cells[i].lib[p]=map[sid];A.scene.cells[i].local[p]=local;}}
    if(v3&&A.scene.plan){for(size_t i=0;i<cells;i++){size_t take=planCellSize<sizeof(PlanCell)?planCellSize:sizeof(PlanCell);memcpy(&A.scene.plan[i],b+off,take);off+=planCellSize;}}
    A.scene.craftX=craftX;A.scene.craftY=craftY;A.scene.craftW=craftW;A.scene.craftH=craftH;A.scene.usoSlotActive=slotActive;A.scene.usoSlotX=slotX;A.scene.usoSlotY=slotY;A.scene.usoSlotW=slotW;A.scene.usoSlotH=slotH;A.scene.usoInserted=usoInserted;wcsncpy(A.scene.title,title,159);wcsncpy(A.scene.usoSlotName,slotName,95);A.scene.usoMapIndex=-1;
    for(int mi=0;mi<A.mapCount;mi++){MapEntry*m=&A.maps[mi];if(_wcsicmp(m->name,A.scene.usoSlotName)!=0)continue;if((v2||v3)&&usoSource>=0){if(m->source!=usoSource)continue;if(usoSource==SRC_MOD&&_wcsicmp(m->origin,usoOrigin)!=0)continue;}A.scene.usoMapIndex=mi;break;}
    scene_rebuild_macro_flags();
    A.currentZ=A.scene.z-1;A.panX=A.panY=0;A.scene.dirty=0;reset_undo();free(b);InvalidateRect(A.hwnd,NULL,FALSE);return 1;
}
static void scene_save_project(void){if(!A.scene.active)return;wchar_t p[PATH_CAP]=L"scene.JMW";if(save_file_dialog(A.hwnd,p,PATH_CAP,tr(L"Projet TFTD Workshop (*.JMW)\0*.JMW\0\0"),tr(L"Enregistrer le projet de scene"),L"JMW")){if(scene_save_project_file(p))set_status(tr(L"Projet de scene enregistre."));else MessageBoxW(A.hwnd,tr(L"Echec de l'enregistrement du projet."),APP_TITLE,MB_ICONERROR);}}
static void scene_open_project(void){if(!proc_can_replace(A.hwnd))return;wchar_t p[PATH_CAP];if(open_file_dialog(A.hwnd,p,PATH_CAP,tr(L"Projet TFTD Workshop (*.JMW)\0*.JMW\0\0"),tr(L"Ouvrir un projet de scene"))){if(scene_load_project_file(p))set_status(tr(L"Projet de scene charge."));else MessageBoxW(A.hwnd,tr(L"Projet JMW invalide ou ressources manquantes."),APP_TITLE,MB_ICONERROR);}}
#include "workshop_procedural.h"
#include "workshop_geo.h"
#include "workshop_geo_scene.h"


static int newmap_read_dimension(HWND edit){
    wchar_t b[32];GetWindowTextW(edit,b,32);if(!b[0])return 0;return _wtoi(b);
}
static LRESULT CALLBACK newmap_wndproc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    (void)lp;
    switch(msg){
    case WM_CREATE:{
        HFONT f=(HFONT)GetStockObject(DEFAULT_GUI_FONT);wchar_t bx[16],by[16],bz[16];
        _snwprintf(bx,15,L"%d",gNewMapLastX);_snwprintf(by,15,L"%d",gNewMapLastY);_snwprintf(bz,15,L"%d",gNewMapLastZ);
        CreateWindowW(L"STATIC",tr(L"Dimensions du nouveau macrobloc"),WS_CHILD|WS_VISIBLE,18,16,340,22,hwnd,NULL,NULL,NULL);
        CreateWindowW(L"STATIC",tr(L"Largeur X (cases)"),WS_CHILD|WS_VISIBLE,28,58,165,20,hwnd,NULL,NULL,NULL);
        NUI.xEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",bx,WS_CHILD|WS_VISIBLE|ES_NUMBER|ES_AUTOHSCROLL,210,53,100,26,hwnd,(HMENU)CID_NEWMAP_X,NULL,NULL);
        CreateWindowW(L"STATIC",tr(L"Profondeur Y (cases)"),WS_CHILD|WS_VISIBLE,28,94,165,20,hwnd,NULL,NULL,NULL);
        NUI.yEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",by,WS_CHILD|WS_VISIBLE|ES_NUMBER|ES_AUTOHSCROLL,210,89,100,26,hwnd,(HMENU)CID_NEWMAP_Y,NULL,NULL);
        CreateWindowW(L"STATIC",tr(L"Hauteur Z (niveaux)"),WS_CHILD|WS_VISIBLE,28,130,165,20,hwnd,NULL,NULL,NULL);
        NUI.zEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",bz,WS_CHILD|WS_VISIBLE|ES_NUMBER|ES_AUTOHSCROLL,210,125,100,26,hwnd,(HMENU)CID_NEWMAP_Z,NULL,NULL);
        CreateWindowW(L"STATIC",tr(L"Format TFTD MAP : chaque dimension doit etre comprise entre 1 et 255."),WS_CHILD|WS_VISIBLE,28,166,350,36,hwnd,NULL,NULL,NULL);
        CreateWindowW(L"BUTTON",tr(L"Creer"),WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,88,210,120,34,hwnd,(HMENU)CID_NEWMAP_CREATE,NULL,NULL);
        CreateWindowW(L"BUTTON",tr(L"Annuler"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,228,210,120,34,hwnd,(HMENU)CID_NEWMAP_CANCEL,NULL,NULL);
        for(HWND c=GetWindow(hwnd,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))SendMessageW(c,WM_SETFONT,(WPARAM)f,TRUE);
        SendMessageW(NUI.xEdit,EM_SETLIMITTEXT,3,0);SendMessageW(NUI.yEdit,EM_SETLIMITTEXT,3,0);SendMessageW(NUI.zEdit,EM_SETLIMITTEXT,3,0);
        SetFocus(NUI.xEdit);SendMessageW(NUI.xEdit,EM_SETSEL,0,-1);return 0;
    }
    case WM_COMMAND:{
        int id=LOWORD(wp);
        if(id==CID_NEWMAP_CANCEL){DestroyWindow(hwnd);return 0;}
        if(id==CID_NEWMAP_CREATE){
            int x=newmap_read_dimension(NUI.xEdit),y=newmap_read_dimension(NUI.yEdit),z=newmap_read_dimension(NUI.zEdit);
            if(x<1||x>255||y<1||y>255||z<1||z>255){MessageBoxW(hwnd,tr(L"Dimensions invalides.\n\nX, Y et Z doivent chacun etre compris entre 1 et 255."),APP_TITLE,MB_ICONWARNING);return 0;}
            if(!confirm_map_change())return 0;
            if(!new_map(x,y,z)){MessageBoxW(hwnd,tr(L"Memoire insuffisante pour creer cette MAP."),APP_TITLE,MB_ICONERROR);return 0;}
            gNewMapLastX=x;gNewMapLastY=y;gNewMapLastZ=z;
            wchar_t st[160];_snwprintf(st,159,tr(L"Nouvelle MAP %dx%dx%d creee."),x,y,z);set_status(st);
            DestroyWindow(hwnd);return 0;
        }
        break;
    }
    case WM_CLOSE:DestroyWindow(hwnd);return 0;
    case WM_DESTROY:NUI.hwnd=NULL;NUI.xEdit=NUI.yEdit=NUI.zEdit=NULL;return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static void open_new_map_dialog(void){
    if(NUI.hwnd){SetForegroundWindow(NUI.hwnd);return;}
    static int registered=0;if(!registered){WNDCLASSEXW wc;ZeroMemory(&wc,sizeof(wc));wc.cbSize=sizeof(wc);wc.lpfnWndProc=newmap_wndproc;wc.hInstance=GetModuleHandleW(NULL);wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"TFTDWorkshopNewMap264";RegisterClassExW(&wc);registered=1;}
    RECT pr={0};GetWindowRect(A.hwnd,&pr);int ww=430,wh=310,px=pr.left+((pr.right-pr.left)-ww)/2,py=pr.top+((pr.bottom-pr.top)-wh)/2;
    NUI.hwnd=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_DLGMODALFRAME,L"TFTDWorkshopNewMap264",tr(L"Nouvelle MAP"),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,px,py,ww,wh,A.hwnd,NULL,GetModuleHandleW(NULL),NULL);
    if(NUI.hwnd){ShowWindow(NUI.hwnd,SW_SHOW);UpdateWindow(NUI.hwnd);}
}

static void blueprint_hangar_layout(HWND hwnd){
    RECT rc;GetClientRect(hwnd,&rc);int w=rc.right,h=rc.bottom,margin=12,top=54,bottom=118;int leftw=w/3;if(leftw<230)leftw=230;if(leftw>360)leftw=360;int listH=h-top-bottom;if(listH<100)listH=100;
    if(BUI.pathText)MoveWindow(BUI.pathText,margin,12,w-190,24,TRUE);if(BUI.chooseBtn)MoveWindow(BUI.chooseBtn,w-166,10,154,28,TRUE);
    if(BUI.list)MoveWindow(BUI.list,margin,top,leftw-24,listH,TRUE);int px=leftw+4,py=top,pw=w-px-margin,ph=listH;
    if(BUI.nameEdit)MoveWindow(BUI.nameEdit,margin,h-96,leftw-24,26,TRUE);
    int by=h-62,bw=116,gap=6,x=margin;if(BUI.captureBtn){MoveWindow(BUI.captureBtn,x,by,bw,30,TRUE);x+=bw+gap;}if(BUI.saveBtn){MoveWindow(BUI.saveBtn,x,by,bw,30,TRUE);x+=bw+gap;}if(BUI.cancelCaptureBtn)MoveWindow(BUI.cancelCaptureBtn,x,by,92,30,TRUE);
    x=leftw+4;if(BUI.useBtn){MoveWindow(BUI.useBtn,x,by,104,30,TRUE);x+=110;}if(BUI.renameBtn){MoveWindow(BUI.renameBtn,x,by,104,30,TRUE);x+=110;}if(BUI.deleteBtn)MoveWindow(BUI.deleteBtn,x,by,104,30,TRUE);
    RECT pr={px,py,px+pw,py+ph};InvalidateRect(hwnd,&pr,FALSE);
}
static void blueprint_hangar_refresh(void){
    if(!BUI.hwnd)return;if(BUI.list){SendMessageW(BUI.list,LB_RESETCONTENT,0,0);for(int i=0;i<A.customBlueprintCount;i++)SendMessageW(BUI.list,LB_ADDSTRING,0,(LPARAM)A.customBlueprints[i].name);if(BUI.selected>=A.customBlueprintCount)BUI.selected=A.customBlueprintCount-1;if(BUI.selected>=0){SendMessageW(BUI.list,LB_SETCURSEL,BUI.selected,0);if(BUI.nameEdit&&!A.blueprintCaptureMode)SetWindowTextW(BUI.nameEdit,A.customBlueprints[BUI.selected].name);}else if(BUI.nameEdit&&!A.blueprintCaptureMode)SetWindowTextW(BUI.nameEdit,L"");}
    if(BUI.pathText){wchar_t t[PATH_CAP+32];_snwprintf(t,PATH_CAP+31,tr(L"Hangar : %ls"),A.blueprintHangarDir);SetWindowTextW(BUI.pathText,t);}
    if(BUI.captureBtn)SetWindowTextW(BUI.captureBtn,A.blueprintCaptureMode?tr(L"CAPTURE ACTIVE"):tr(L"NOUVELLE CAPTURE"));if(BUI.saveBtn)EnableWindow(BUI.saveBtn,A.blueprintCaptureMode&&A.blueprintCaptureCount>=2);if(BUI.cancelCaptureBtn)EnableWindow(BUI.cancelCaptureBtn,A.blueprintCaptureMode);int has=BUI.selected>=0&&BUI.selected<A.customBlueprintCount;if(BUI.useBtn)EnableWindow(BUI.useBtn,has);if(BUI.renameBtn)EnableWindow(BUI.renameBtn,has&&!A.blueprintCaptureMode);if(BUI.deleteBtn)EnableWindow(BUI.deleteBtn,has&&!A.blueprintCaptureMode);InvalidateRect(BUI.hwnd,NULL,FALSE);
}
static void blueprint_hangar_select_folder(void){
    wchar_t folder[PATH_CAP];if(!browse_folder(BUI.hwnd?BUI.hwnd:A.hwnd,folder,PATH_CAP,tr(L"Choisissez le dossier Hangar a Blueprints"),A.blueprintHangarDir))return;if(!path_exists_dir(folder))return;
    if(A.blueprintCaptureMode)blueprint_capture_cancel();wcsncpy(A.blueprintHangarDir,folder,PATH_CAP-1);A.blueprintHangarDir[PATH_CAP-1]=0;config_save();custom_blueprints_path_init();custom_blueprints_load();A.customBlueprintIndex=-1;if(A.blueprintMode==2)A.blueprintMode=0;BUI.selected=A.customBlueprintCount?0:-1;blueprint_hangar_refresh();set_status(tr(L"Hangar a Blueprints change : bibliotheque perso rechargee depuis le dossier choisi."));InvalidateRect(A.hwnd,NULL,FALSE);
}
static void blueprint_hangar_use_selected(void){E.tool=EDIT_PLACE;editor_selection_clear();
    int bi=BUI.selected;if(bi<0||bi>=A.customBlueprintCount)return;CustomBlueprintDef*b=&A.customBlueprints[bi];if(b->partCount<=0)return;CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart];int lib=blueprint_resolve_lib(p->dataset);if(lib<0||p->mcd<0||p->mcd>=A.library[lib].mcdCount){MessageBoxW(BUI.hwnd,tr(L"Impossible d'activer ce blueprint : son dataset n'est pas disponible dans les sources actuellement indexees."),APP_TITLE,MB_ICONWARNING);return;}A.selectedLib=lib;A.selectedLocal=p->mcd;A.selectedLayer=p->layer;A.customBlueprintIndex=bi;A.blueprintIndex=-1;A.blueprintMode=2;wchar_t st[220];_snwprintf(st,219,tr(L"Blueprint perso actif : %ls (%d pieces, %dx%dx%d)."),b->name,b->partCount,b->spanX,b->spanY,b->spanZ);set_status(st);InvalidateRect(A.hwnd,NULL,FALSE);SetForegroundWindow(A.hwnd);
}
static LRESULT CALLBACK blueprint_hangar_wndproc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_CREATE:{BUI.hwnd=hwnd;BUI.selected=A.customBlueprintCount?0:-1;HFONT f=(HFONT)GetStockObject(DEFAULT_GUI_FONT);BUI.pathText=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE|SS_PATHELLIPSIS,12,12,500,24,hwnd,NULL,NULL,NULL);BUI.chooseBtn=CreateWindowW(L"BUTTON",tr(L"CHOISIR DOSSIER..."),WS_CHILD|WS_VISIBLE,520,10,150,28,hwnd,(HMENU)CID_HANGAR_CHOOSE,NULL,NULL);BUI.list=CreateWindowExW(WS_EX_CLIENTEDGE,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,12,54,240,360,hwnd,(HMENU)CID_HANGAR_LIST,NULL,NULL);BUI.nameEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,12,430,240,26,hwnd,(HMENU)CID_HANGAR_NAME,NULL,NULL);BUI.captureBtn=CreateWindowW(L"BUTTON",tr(L"NOUVELLE CAPTURE"),WS_CHILD|WS_VISIBLE,12,465,116,30,hwnd,(HMENU)CID_HANGAR_CAPTURE,NULL,NULL);BUI.saveBtn=CreateWindowW(L"BUTTON",tr(L"ENREGISTRER"),WS_CHILD|WS_VISIBLE,134,465,116,30,hwnd,(HMENU)CID_HANGAR_SAVE,NULL,NULL);BUI.cancelCaptureBtn=CreateWindowW(L"BUTTON",tr(L"ANNULER"),WS_CHILD|WS_VISIBLE,256,465,92,30,hwnd,(HMENU)CID_HANGAR_CANCEL_CAPTURE,NULL,NULL);BUI.useBtn=CreateWindowW(L"BUTTON",tr(L"UTILISER"),WS_CHILD|WS_VISIBLE,360,465,104,30,hwnd,(HMENU)CID_HANGAR_USE,NULL,NULL);BUI.renameBtn=CreateWindowW(L"BUTTON",tr(L"RENOMMER"),WS_CHILD|WS_VISIBLE,470,465,104,30,hwnd,(HMENU)CID_HANGAR_RENAME,NULL,NULL);BUI.deleteBtn=CreateWindowW(L"BUTTON",tr(L"SUPPRIMER"),WS_CHILD|WS_VISIBLE,580,465,104,30,hwnd,(HMENU)CID_HANGAR_DELETE,NULL,NULL);for(HWND c=GetWindow(hwnd,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))SendMessageW(c,WM_SETFONT,(WPARAM)f,TRUE);blueprint_hangar_refresh();blueprint_hangar_layout(hwnd);return 0;}
    case WM_GETMINMAXINFO:{MINMAXINFO*m=(MINMAXINFO*)lp;m->ptMinTrackSize.x=720;m->ptMinTrackSize.y=500;return 0;}
    case WM_SIZE:blueprint_hangar_layout(hwnd);return 0;
    case WM_COMMAND:{int id=LOWORD(wp),code=HIWORD(wp);if(id==CID_HANGAR_LIST&&code==LBN_SELCHANGE){BUI.selected=(int)SendMessageW(BUI.list,LB_GETCURSEL,0,0);if(BUI.selected>=0&&BUI.selected<A.customBlueprintCount)SetWindowTextW(BUI.nameEdit,A.customBlueprints[BUI.selected].name);InvalidateRect(hwnd,NULL,FALSE);return 0;}if(id==CID_HANGAR_CHOOSE){blueprint_hangar_select_folder();return 0;}if(id==CID_HANGAR_CAPTURE){A.blueprintCaptureMode=1;A.blueprintCaptureCount=0;A.blueprintMode=0;wchar_t nm[96];_snwprintf(nm,95,L"Blueprint_%03d",A.customBlueprintCount+1);SetWindowTextW(BUI.nameEdit,nm);set_status(tr(L"CAPTURE HANGAR : cliquez directement les pieces visibles, sans choisir leur couche. Tous niveaux visibles sont selectionnables. Recliquez pour retirer."));blueprint_hangar_refresh();SetForegroundWindow(A.hwnd);InvalidateRect(A.hwnd,NULL,FALSE);return 0;}if(id==CID_HANGAR_CANCEL_CAPTURE){blueprint_capture_cancel();blueprint_hangar_refresh();return 0;}if(id==CID_HANGAR_SAVE){wchar_t nm[96];GetWindowTextW(BUI.nameEdit,nm,96);int bi=blueprint_capture_save_named(nm);if(bi>=0){BUI.selected=bi;blueprint_hangar_refresh();}return 0;}if(id==CID_HANGAR_USE){blueprint_hangar_use_selected();return 0;}if(id==CID_HANGAR_RENAME){int bi=BUI.selected;if(bi>=0&&bi<A.customBlueprintCount){wchar_t nm[96];GetWindowTextW(BUI.nameEdit,nm,96);if(!nm[0]){MessageBoxW(hwnd,tr(L"Donnez un nom au blueprint."),APP_TITLE,MB_ICONWARNING);return 0;}for(int i=0;i<A.customBlueprintCount;i++)if(i!=bi&&_wcsicmp(A.customBlueprints[i].name,nm)==0){MessageBoxW(hwnd,tr(L"Un blueprint de ce nom existe deja dans ce Hangar."),APP_TITLE,MB_ICONWARNING);return 0;}wcsncpy(A.customBlueprints[bi].name,nm,95);A.customBlueprints[bi].name[95]=0;custom_blueprints_save();blueprint_hangar_refresh();}return 0;}if(id==CID_HANGAR_DELETE){int bi=BUI.selected;if(bi>=0&&bi<A.customBlueprintCount&&MessageBoxW(hwnd,tr(L"Supprimer ce blueprint personnel du Hangar ?"),APP_TITLE,MB_YESNO|MB_ICONWARNING)==IDYES){custom_blueprint_delete(bi);if(BUI.selected>=A.customBlueprintCount)BUI.selected=A.customBlueprintCount-1;blueprint_hangar_refresh();InvalidateRect(A.hwnd,NULL,FALSE);}return 0;}break;}
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);RECT rc;GetClientRect(hwnd,&rc);int leftw=rc.right/3;if(leftw<230)leftw=230;if(leftw>360)leftw=360;RECT pr={leftw+4,54,rc.right-12,rc.bottom-118};draw_hangar_blueprint_preview(dc,&pr,BUI.selected);if(A.blueprintCaptureMode){SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(200,70,120));wchar_t t[100];_snwprintf(t,99,tr(L"CAPTURE ACTIVE : %d piece(s)"),A.blueprintCaptureCount);TextOutW(dc,14,rc.bottom-124,t,(int)wcslen(t));}EndPaint(hwnd,&ps);return 0;}
    case WM_CLOSE:DestroyWindow(hwnd);return 0;case WM_DESTROY:BUI.hwnd=NULL;BUI.list=BUI.nameEdit=BUI.pathText=NULL;return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static void open_blueprint_hangar(void){
    if(BUI.hwnd){ShowWindow(BUI.hwnd,SW_RESTORE);SetForegroundWindow(BUI.hwnd);return;}static int reg=0;if(!reg){WNDCLASSEXW wc;ZeroMemory(&wc,sizeof(wc));wc.cbSize=sizeof(wc);wc.lpfnWndProc=blueprint_hangar_wndproc;wc.hInstance=GetModuleHandleW(NULL);wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"TFTDWorkshopBlueprintHangar269";RegisterClassExW(&wc);reg=1;}RECT mr={0};GetWindowRect(A.hwnd,&mr);int ww=980,wh=680;int x=mr.left+60,y=mr.top+80;BUI.hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,L"TFTDWorkshopBlueprintHangar269",tr(L"Hangar a Blueprints"),WS_OVERLAPPEDWINDOW,x,y,ww,wh,A.hwnd,NULL,GetModuleHandleW(NULL),NULL);if(BUI.hwnd){ShowWindow(BUI.hwnd,SW_SHOW);UpdateWindow(BUI.hwnd);}
}
static void do_save_as(void){if(A.scene.active){scene_save_project();return;}open_export_wizard();}
static void do_save(void){
    if(A.scene.active){scene_save_project();return;}if(!A.map.cells)return;int src=current_map_source();
    if(src==SRC_MOD){if(!A.map.dirty){set_status(tr(L"Aucune modification a enregistrer."));return;}safe_save_current_mod();return;}
    open_export_wizard();
}

static HMENU make_menu(void);
#include "workshop_tutorial.h"
static void handle_command(int id){
 if(id>=IDM_LANGUAGE_BASE&&id<IDM_LANGUAGE_BASE+4){i18nLanguage=id-IDM_LANGUAGE_BASE;WritePrivateProfileStringW(CFG_SECTION,L"Language",i18nCodes[i18nLanguage],A.configPath);HMENU old=GetMenu(A.hwnd);SetMenu(A.hwnd,make_menu());DestroyMenu(old);DrawMenuBar(A.hwnd);EnumThreadWindows(GetCurrentThreadId(),i18n_refresh_window,0);tutorial_refresh();set_status(tr(L"Pret. Configurez TFTD ORIGINAL, OXCE STANDARD et MODS OXCE dans le menu Ressources."));return;}
    if(editor_action(id))return;switch(id){
    case IDM_NEW_MAP:open_new_map_dialog();break;
    case IDM_OPEN_MAP:do_open_map();break;case IDM_SAVE:do_save();break;case IDM_SAVE_AS:do_save_as();break;case IDM_EXIT:SendMessageW(A.hwnd,WM_CLOSE,0,0);break;
    case IDM_SELECT_TFTD:do_select_root(0);break;case IDM_SELECT_OXCE:do_select_root(1);break;case IDM_SELECT_MODS:do_select_root(2);break;case IDM_SELECT_HD_CUSTOM:do_select_hd_custom_root();break;case IDM_SELECT_MAPS_TFTD:do_select_map_root(0);break;case IDM_SELECT_MAPS_OXCE:do_select_map_root(1);break;case IDM_REINDEX:if(confirm_map_change())reindex_resources();break;
    case IDM_MANUAL_MCD:do_manual_mcd();break;case IDM_LOAD_PALETTE:do_load_palette();break;case IDM_CLEAR_ACTIVE:active_clear();set_status(tr(L"Liste de datasets MAP videe."));break;
    case IDM_LAYER_FLOOR:A.selectedLayer=0;break;case IDM_LAYER_WEST:A.selectedLayer=1;break;case IDM_LAYER_NORTH:A.selectedLayer=2;break;case IDM_LAYER_OBJECT:A.selectedLayer=3;break;
    case IDM_Z_UP:set_z(A.currentZ+1);break;case IDM_Z_DOWN:set_z(A.currentZ-1);break;case IDM_Z_ALL:if(A.viewMode==0)set_normal_visibility(A.showAllBelow?0:1);else A.showAllBelow=!A.showAllBelow;break;case IDM_Z_COMPLETE:set_normal_visibility(A.showComplete?(A.showAllBelow?1:0):2);break;case IDM_Z_SINGLE:set_normal_visibility(0);break;case IDM_Z_BELOW:set_normal_visibility(1);break;
    case IDM_ZOOM_IN:set_zoom(A.zoom+1);break;case IDM_ZOOM_OUT:set_zoom(A.zoom-1);break;case IDM_CENTER:center_view();break;
    case 12320:case 12321:A.hdOverlayProvider=id==12321?2:1;hd_cache_clear();config_save();CheckMenuRadioItem(GetMenu(A.hwnd),12320,12321,id,MF_BYCOMMAND);InvalidateRect(A.hwnd,NULL,FALSE);break;
    case IDM_RENDER_LEGACY:asset_render_mode_set(0);break;
    case IDM_RENDER_HD:asset_render_mode_set(1);break;
    case IDM_RENDER_REAL:asset_render_mode_set(3);break;case IDM_RENDER_DEBUG:asset_render_mode_set(4);break;case IDM_RENDER_CYCLE:asset_render_mode_set((A.assetRenderMode+1)%5);break;case IDM_RENDER_HD_CUSTOM:if(A.hdCustomRoot[0]&&path_exists_dir(A.hdCustomRoot))asset_render_mode_set(2);else do_select_hd_custom_root();break;
    case IDM_VIEW_NORMAL:case IDM_VIEW_TOP:case IDM_VIEW_ISO:A.viewMode=id-IDM_VIEW_NORMAL;A.planPasteMode=0;CheckMenuItem(GetMenu(A.hwnd),IDM_VIEW_PLAN,MF_BYCOMMAND|(plan_view_active()?MF_CHECKED:MF_UNCHECKED));set_status(A.viewMode==0?tr(L"Vue normale : pieces et assemblages."):(A.viewMode==1?tr(L"Plan 2D : vue de dessus."):tr(L"Plan isometrique.")));break;
    case IDM_GRID_TOGGLE:A.showGrid=!A.showGrid;CheckMenuItem(GetMenu(A.hwnd),IDM_GRID_TOGGLE,MF_BYCOMMAND|(A.showGrid?MF_CHECKED:MF_UNCHECKED));set_status(A.showGrid?tr(L"Grille du niveau Z affichee."):tr(L"Grille du niveau Z masquee : mode capture propre."));break;case IDM_VIEW_PLAN:A.viewMode=(A.viewMode+1)%3;A.planPasteMode=0;CheckMenuItem(GetMenu(A.hwnd),IDM_VIEW_PLAN,MF_BYCOMMAND|(plan_view_active()?MF_CHECKED:MF_UNCHECKED));set_status(A.viewMode==0?tr(L"Vue TUILES isometrique."):(A.viewMode==1?tr(L"PLAN 2D : vue de dessus logique."):tr(L"PLAN ISO : niveaux inferieurs visibles et assombris.")));break;case IDM_PLAN_COPY:plan_copy_selection();break;case IDM_PLAN_PASTE:plan_begin_paste();break;case IDM_PLAN_CANCEL_PASTE:plan_cancel_paste();break;
    case IDM_FILTER_DORMANT:A.mapFilterDormant=!A.mapFilterDormant;for(int i=0;i<3;i++)A.sourceScroll[i]=0;CheckMenuItem(GetMenu(A.hwnd),IDM_FILTER_DORMANT,MF_BYCOMMAND|(A.mapFilterDormant?MF_CHECKED:MF_UNCHECKED));set_status(A.mapFilterDormant?tr(L"Filtre MAP : LEGACY DORMANT / OXCE UNREACHABLE uniquement."):tr(L"Filtre MAP : toutes les cartes."));break;
    case IDM_EXPORT_MOD:open_export_wizard();break;
    case IDM_RMP_TOGGLE:rmp_toggle_overlay();break;
    case IDM_RMP_EDIT:A.rmp.show=1;A.rmp.editMode=!A.rmp.editMode;set_status(A.rmp.editMode?tr(L"Edition RMP : panneau simple sous les couches. Molette = niveau, Ctrl+molette = zoom. Parametres avances masques par defaut."):tr(L"Edition RMP desactivee."));break;
    case IDM_RMP_ANALYZE:rmp_auto_analyze();break;
    case IDM_RMP_CLEAR_PROPOSALS:A.rmp.proposalCount=0;set_status(tr(L"Propositions RMP effacees."));break;
    case IDM_RMP_APPLY_PROPOSALS:rmp_apply_proposals();break;
    case IDM_RMP_SOCKET_N:rmp_toggle_selected_socket(-2);break;
    case IDM_RMP_SOCKET_E:rmp_toggle_selected_socket(-3);break;
    case IDM_RMP_SOCKET_S:rmp_toggle_selected_socket(-4);break;
    case IDM_RMP_SOCKET_W:rmp_toggle_selected_socket(-5);break;
    case IDM_RMP_SAVE:rmp_save_to_mod();break;
    case IDM_GEO_NATIVE:geo_open();break;case IDM_GEO_LAB:{wchar_t path[PATH_CAP];_snwprintf(path,PATH_CAP-1,L"%ls\\TFTD_HD_Gabarits_Universels\\Datasets\\GEO_TERRAIN\\lab\\index.html",A.modsRoot);if(path_exists_file(path))ShellExecuteW(A.hwnd,L"open",path,NULL,NULL,SW_SHOWNORMAL);else MessageBoxW(A.hwnd,tr(L"Laboratoire GEO_TERRAIN introuvable dans le mod Gabarits universels."),APP_TITLE,MB_ICONINFORMATION);break;}case IDM_PROCEDURAL:open_procedural();break;case IDM_COMPOSER:open_composer();break;case IDM_SCENE_INSERT_USO:scene_insert_uso();break;case IDM_SCENE_SAVE:scene_save_project();break;case IDM_SCENE_OPEN:scene_open_project();break;case IDM_CLOSE_SCENE:if(!proc_can_replace(A.hwnd))break;scene_free();A.currentZ=A.map.cells?A.map.z-1:0;A.panX=A.panY=0;reset_undo();set_status(tr(L"Vue compositeur fermee."));break;
    case IDM_UNDO:do_undo();break;case IDM_REDO:do_redo();break;
    case IDM_BLUEPRINT_HANGAR:open_blueprint_hangar();break;
    case IDM_BLUEPRINT_CAPTURE:A.blueprintCaptureMode=1;A.blueprintCaptureCount=0;A.blueprintMode=0;open_blueprint_hangar();if(BUI.nameEdit){wchar_t nm[96];_snwprintf(nm,95,L"Blueprint_%03d",A.customBlueprintCount+1);SetWindowTextW(BUI.nameEdit,nm);}blueprint_hangar_refresh();set_status(tr(L"CAPTURE HANGAR : cliquez directement les pieces visibles sur la MAP, toutes couches confondues, puis donnez un nom et ENREGISTRER."));InvalidateRect(A.hwnd,NULL,FALSE);break;
    case IDM_BLUEPRINT_SELECT_FOLDER:open_blueprint_hangar();blueprint_hangar_select_folder();break;
    case 12311:if(MessageBoxW(A.hwnd,tr(L"Pour contacter Benjamin :\n\ncolmoutarde57700@gmail.com\n\nCopier cette adresse dans le presse-papiers ?"),tr(L"Contacter l’auteur"),MB_YESNO|MB_ICONINFORMATION)==IDYES){const wchar_t*email=L"colmoutarde57700@gmail.com";size_t bytes=(wcslen(email)+1)*sizeof(wchar_t);HGLOBAL data=GlobalAlloc(GMEM_MOVEABLE,bytes);if(data){void*ptr=GlobalLock(data);if(ptr){memcpy(ptr,email,bytes);GlobalUnlock(data);if(OpenClipboard(A.hwnd)){EmptyClipboard();if(SetClipboardData(CF_UNICODETEXT,data)){data=NULL;set_status(tr(L"Adresse de contact copiée."));}CloseClipboard();}}if(data)GlobalFree(data);}}break;
    case IDM_TUTORIAL:tutorial_open();break;
    case IDM_ABOUT:MessageBoxW(A.hwnd,tr(L"TFTD Workshop 2.12.7\n\nEditeur de cartes et assemblages, generateur procedural GEO, apercus PNG HD et REAL HD.\n\nLangage : francais, anglais, espagnol, allemand. Tuto : guide local de l'interface et des textures.\n\nGEO sauvegarde en JMW4 ; export MAP/OXCE et edition directe GEO non disponibles. Aucun asset TFTD/OXCE inclus.\n\nThanks to GPT-6 Sol"),tr(L"A propos"),MB_ICONINFORMATION);
    }InvalidateRect(A.hwnd,NULL,FALSE);
}

static void cancel_placement(void){editor_cancel();}
static void fit_sidebars(void){
    if(A.clientW<1)return;
    int budget=A.clientW-420;if(budget<360)budget=360;
    A.sidebarW=clampi(A.sidebarW,180,budget-180);A.inspectorW=clampi(A.inspectorW,180,budget-A.sidebarW);
}
#include "workshop_semantics.h"
#include "workshop_editing.h"
#include "workshop_ui.h"
static LRESULT CALLBACK wndproc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){switch(msg){
    case WM_GETMINMAXINFO:{MINMAXINFO*m=(MINMAXINFO*)lp;m->ptMinTrackSize.x=820;m->ptMinTrackSize.y=560;return 0;}
    case WM_CLOSE:if(confirm_map_change()&&(!GEO.hwnd||geo_replace()))DestroyWindow(hwnd);return 0;
    case WM_DESTROY:PostQuitMessage(0);return 0;
    case WM_SIZE:{
        A.clientW=LOWORD(lp);A.clientH=HIWORD(lp);fit_sidebars();
        if(A.searchEdit)MoveWindow(A.searchEdit,60,136,A.sidebarW-68,24,TRUE);
        InvalidateRect(hwnd,NULL,FALSE);return 0;
    }
    case WM_SETCURSOR:{POINT pt;GetCursorPos(&pt);ScreenToClient(hwnd,&pt);if(LOWORD(lp)==HTCLIENT){
        if(abs(pt.x-A.sidebarW)<=5||abs(pt.x-(A.clientW-A.inspectorW))<=5){SetCursor(LoadCursor(NULL,IDC_SIZEWE));return TRUE;}
        if(A.browserMode==0&&pt.x<A.sidebarW&&(abs(pt.y-A.panelSplit1)<=5||abs(pt.y-A.panelSplit2)<=5)){SetCursor(LoadCursor(NULL,IDC_SIZENS));return TRUE;}}
        break;}
    case WM_CAPTURECHANGED:A.resizingSidebar=A.resizingInspector=A.resizingPanel=A.painting=A.erasing=0;return 0;
    case WM_CTLCOLOREDIT:{
        static HBRUSH br=NULL;if(!br)br=CreateSolidBrush(RGB(24,34,41));HDC dc=(HDC)wp;SetTextColor(dc,RGB(225,238,242));SetBkColor(dc,RGB(24,34,41));return (LRESULT)br;
    }
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);paint(hwnd,dc);EndPaint(hwnd,&ps);return 0;}
    case WM_COMMAND:{
        if(LOWORD(wp)==SEARCH_ID && HIWORD(wp)==EN_CHANGE){
            GetWindowTextW(A.searchEdit,A.searchFilter,96);A.treeScroll=0;InvalidateRect(hwnd,NULL,FALSE);return 0;
        }
        handle_command(LOWORD(wp));return 0;
    }
    case WM_KEYDOWN:{
        if(wp==VK_F6){asset_render_mode_set((A.assetRenderMode+1)%5);return 0;}
        int k=(int)wp;int ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0;
        if(editor_key(k,ctrl)){InvalidateRect(hwnd,NULL,FALSE);return 0;}
        if(k==VK_DELETE&&A.rmp.editMode&&A.rmp.show){rmp_delete_selected();InvalidateRect(hwnd,NULL,FALSE);return 0;}
        if(!ctrl&&k=='R'){rmp_toggle_overlay();return 0;}
        if(!ctrl&&k=='G'){handle_command(IDM_GRID_TOGGLE);InvalidateRect(hwnd,NULL,FALSE);return 0;}
        if(ctrl&&k=='Z')do_undo();else if(ctrl&&k=='Y')do_redo();else if(ctrl&&k=='S')do_save();
        else if(ctrl&&k=='C'&&plan_view_active())plan_copy_selection();else if(ctrl&&k=='V'&&plan_view_active())plan_begin_paste();
        else if(k==VK_ESCAPE&&plan_view_active()&&A.planPasteMode)plan_cancel_paste();
        else if(k==VK_ESCAPE&&!plan_view_active()){if(A.blueprintCaptureMode)blueprint_capture_cancel();else cancel_placement();}
        else if(!ctrl&&k=='P'){handle_command(IDM_VIEW_PLAN);}
        else if(ctrl&&k=='F'){SetFocus(A.searchEdit);SendMessageW(A.searchEdit,EM_SETSEL,0,-1);}
        else if(k==VK_F1)A.selectedLayer=0;else if(k==VK_F2)A.selectedLayer=1;else if(k==VK_F3)A.selectedLayer=2;else if(k==VK_F4)A.selectedLayer=3;
        else if(k==VK_PRIOR)set_zoom(A.zoom+1);else if(k==VK_NEXT)set_zoom(A.zoom-1);
        else if(k==VK_ADD||k==VK_OEM_PLUS)set_zoom(A.zoom+1);else if(k==VK_SUBTRACT||k==VK_OEM_MINUS)set_zoom(A.zoom-1);
        InvalidateRect(hwnd,NULL,FALSE);return 0;
    }
    case WM_LBUTTONDOWN:{
        int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp);SetFocus(hwnd);ensure_panel_splits();
        if(x>=A.clientW-A.inspectorW-5&&x<=A.clientW-A.inspectorW+5){A.resizingInspector=1;SetCapture(hwnd);return 0;}
        if(x>=A.sidebarW-5&&x<=A.sidebarW+5){A.resizingSidebar=1;SetCapture(hwnd);return 0;}
        if(A.browserMode==0&&x<A.sidebarW&&y>=TREE_TOP&&y<A.clientH-TREE_BOTTOM){if(y>=A.panelSplit1-5&&y<=A.panelSplit1+5){A.resizingPanel=1;SetCapture(hwnd);return 0;}if(y>=A.panelSplit2-5&&y<=A.panelSplit2+5){A.resizingPanel=2;SetCapture(hwnd);return 0;}}
        if(x<A.sidebarW){sidebar_click(x,y);return 0;}
        if(x>=A.clientW-A.inspectorW){editor_inspector_click(x,y);return 0;}
        if(editor_header_click(x,y))return 0;
        if(level_bar_click(x,y))return 0;
        if(plan_view_active()){if(plan_toolbar_click(x,y))return 0;}else if(layer_bar_click(x,y))return 0;
        
        if(!A.viewMode&&rmp_toolbar_click(x,y))return 0;
        if(plan_view_active()){int tx,ty;if(!mouse_to_tile(x,y,&tx,&ty))return 0;if(A.planTool==4&&!A.planPasteMode){A.planSelecting=1;A.planSelActive=1;A.planSelX0=A.planSelX1=tx;A.planSelY0=A.planSelY1=ty;SetCapture(hwnd);set_status(tr(L"PLAN : selection rectangulaire en cours..."));InvalidateRect(hwnd,NULL,FALSE);return 0;}A.lastPaintX=A.lastPaintY=-999;plan_edit_at(x,y,E.planErase);if((A.planTool==0||A.planTool==1||A.planTool==2||A.planTool==3)&&!(A.planTool==0&&A.brushMode==3)){A.painting=!E.planErase;A.erasing=E.planErase;SetCapture(hwnd);}return 0;}
        if(A.scene.active && (GetKeyState(VK_MENU)&0x8000) && A.scene.usoSlotActive && !A.scene.usoInserted){int tx,ty;if(mouse_to_tile(x,y,&tx,&ty)&&point_in_uso_slot(tx,ty)){A.draggingUsoSlot=1;A.usoDragOffX=tx-A.scene.usoSlotX;A.usoDragOffY=ty-A.scene.usoSlotY;SetCapture(hwnd);set_status(tr(L"Deplacement du masque USO : relachez Alt+clic a la nouvelle position."));return 0;}}
        if(A.rmp.editMode&&A.rmp.show&&!A.scene.active){if(rmp_handle_left_click(x,y,wp))return 0;}
        if(A.blueprintCaptureMode){blueprint_capture_click(x,y);return 0;}
        if(GetKeyState(VK_CONTROL)&0x8000){pipette_at(x,y);editor_palette_selected();return 0;}
        if(editor_left(x,y,wp))return 0;
        A.lastPaintX=A.lastPaintY=-999;int bpStamp=blueprint_current_matches_selection();edit_at(x,y,0);
        if(bpStamp){A.painting=0;A.erasing=0;return 0;}
        if(A.brushMode!=3){A.painting=1;A.erasing=0;SetCapture(hwnd);}else{A.painting=0;A.erasing=0;}
        return 0;
    }
    case WM_LBUTTONUP:E.strokeGroup=0;if(A.erasing){A.erasing=0;ReleaseCapture();}if(A.resizingSidebar||A.resizingInspector||A.resizingPanel){A.resizingSidebar=0;A.resizingInspector=0;A.resizingPanel=0;save_panel_layout();ReleaseCapture();InvalidateRect(hwnd,NULL,FALSE);return 0;}if(A.planSelecting){A.planSelecting=0;ReleaseCapture();set_status(tr(L"PLAN : selection prete. Ctrl+C copie, Ctrl+V colle."));InvalidateRect(hwnd,NULL,FALSE);return 0;}if(A.painting){A.painting=0;ReleaseCapture();}if(A.draggingUsoSlot){A.draggingUsoSlot=0;ReleaseCapture();scene_rebuild_macro_flags();A.scene.dirty=1;set_status(tr(L"Reservation USO deplacee. Les macro-cases sont remises en evidence ; le sous-sol n'est pas regenere automatiquement."));}return 0;
    case WM_RBUTTONDOWN:{int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp);SetFocus(hwnd);if(x<A.sidebarW&&A.browserMode==0){maps_tree_context(x,y);return 0;}cancel_placement();return 0;}
    case WM_RBUTTONUP:if(A.erasing){A.erasing=0;ReleaseCapture();}return 0;
    case WM_MBUTTONDOWN:A.painting=0;A.erasing=0;A.draggingPan=1;A.dragLastX=GET_X_LPARAM(lp);A.dragLastY=GET_Y_LPARAM(lp);SetCapture(hwnd);return 0;
    case WM_MBUTTONUP:A.draggingPan=0;ReleaseCapture();return 0;
    case WM_MOUSEMOVE:{
        int oldValid=A.hoverValid,oldX=A.hoverX,oldY=A.hoverY;
        A.mouseX=GET_X_LPARAM(lp);A.mouseY=GET_Y_LPARAM(lp);
        int hx,hy;if((plan_view_active()||A.rmp.editMode||editor_canvas(A.mouseX,A.mouseY))&&mouse_to_tile(A.mouseX,A.mouseY,&hx,&hy)){A.hoverValid=1;A.hoverX=hx;A.hoverY=hy;}else A.hoverValid=0;
        if(A.planSelecting&&plan_view_active()){int tx,ty;if(mouse_to_tile(A.mouseX,A.mouseY,&tx,&ty)){A.planSelX1=tx;A.planSelY1=ty;InvalidateRect(hwnd,NULL,FALSE);}return 0;}
        if(A.resizingInspector){int maxw=A.clientW-A.sidebarW-420;if(maxw<180)maxw=180;A.inspectorW=clampi(A.clientW-A.mouseX,180,maxw);InvalidateRect(hwnd,NULL,FALSE);return 0;}
        if(A.resizingSidebar){int maxw=A.clientW-A.inspectorW-420;if(maxw<180)maxw=180;A.sidebarW=clampi(A.mouseX,180,maxw);if(A.searchEdit)MoveWindow(A.searchEdit,60,136,A.sidebarW-68,24,TRUE);InvalidateRect(hwnd,NULL,FALSE);return 0;}
        if(A.resizingPanel){int top=TREE_TOP,bottom=A.clientH-TREE_BOTTOM;if(A.resizingPanel==1)A.panelSplit1=clampi(A.mouseY,top+55,A.panelSplit2-55);else A.panelSplit2=clampi(A.mouseY,A.panelSplit1+55,bottom-55);InvalidateRect(hwnd,NULL,FALSE);return 0;}
        if(A.draggingPan){int x=A.mouseX,y=A.mouseY;A.panX+=x-A.dragLastX;A.panY+=y-A.dragLastY;A.dragLastX=x;A.dragLastY=y;InvalidateRect(hwnd,NULL,FALSE);}
        else if(A.draggingUsoSlot&&A.scene.active&&A.scene.usoSlotActive){int tx,ty;if(mouse_to_tile(A.mouseX,A.mouseY,&tx,&ty)){int nx=clampi(tx-A.usoDragOffX,0,A.scene.x-A.scene.usoSlotW),ny=clampi(ty-A.usoDragOffY,0,A.scene.y-A.scene.usoSlotH);if(nx!=A.scene.usoSlotX||ny!=A.scene.usoSlotY){A.scene.usoSlotX=nx;A.scene.usoSlotY=ny;InvalidateRect(hwnd,NULL,FALSE);}}}
        else if(A.painting && (wp&MK_LBUTTON)){int x,y;if(mouse_to_tile(A.mouseX,A.mouseY,&x,&y)&& (x!=A.lastPaintX||y!=A.lastPaintY)){if(plan_view_active())plan_edit_at(A.mouseX,A.mouseY,0);else if(editor_canvas(A.mouseX,A.mouseY)){if(A.brushMode==0&&!A.blueprintMode){editor_stamp(x,y);A.lastPaintX=x;A.lastPaintY=y;}else edit_at(A.mouseX,A.mouseY,0);}}}
        else if(A.erasing && (wp&MK_LBUTTON)){int x,y;if(mouse_to_tile(A.mouseX,A.mouseY,&x,&y)&& (x!=A.lastPaintX||y!=A.lastPaintY)){if(plan_view_active())plan_edit_at(A.mouseX,A.mouseY,1);else if(editor_canvas(A.mouseX,A.mouseY)){BlueprintCapturePart p;if(hit_piece_at_levels(A.mouseX,A.mouseY,&p.x,&p.y,&p.z,&p.layer,&p.lib,&p.local,A.currentZ,A.currentZ)&&!editor_locked(p.x,p.y,p.z)){A.currentEditGroup=E.strokeGroup;erase_visible_piece_at(A.mouseX,A.mouseY);A.currentEditGroup=0;}}}}
        else if(oldValid!=A.hoverValid || (A.hoverValid&&(oldX!=A.hoverX||oldY!=A.hoverY))){if(plan_view_active()||(!A.rmp.editMode&&(A.selectedLib>=0||E.moving)))InvalidateRect(hwnd,NULL,FALSE);else{RECT ir={A.clientW-A.inspectorW,0,A.clientW,A.clientH};InvalidateRect(hwnd,&ir,FALSE);}}
        return 0;
    }
    case WM_MOUSEWHEEL:{
        POINT pt={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(hwnd,&pt);A.mouseX=pt.x;A.mouseY=pt.y;int d=GET_WHEEL_DELTA_WPARAM(wp);int ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0;
        if(A.rmp.editMode&&A.rmp.show&&!A.scene.active&&A.mouseX>=A.sidebarW){if(ctrl&&A.mouseX<A.clientW-A.inspectorW)set_zoom(A.zoom+(d>0?1:-1));else set_z(A.currentZ+(d>0?1:-1));return 0;}
        if(A.mouseX<A.sidebarW){if(A.browserMode==0){ensure_panel_splits();int panel=A.mouseY<A.panelSplit1?0:(A.mouseY<A.panelSplit2?1:2);int top,bottom;panel_bounds(panel,&top,&bottom);int visible=(bottom-2)-(top+30),max=map_panel_content_height(panel)-visible;if(max<0)max=0;A.sourceScroll[panel]-=d/WHEEL_DELTA*72;A.sourceScroll[panel]=clampi(A.sourceScroll[panel],0,max);}else{A.treeScroll-=d/WHEEL_DELTA*72;int visible=A.clientH-TREE_BOTTOM-TREE_TOP,max=A.treeContentH-visible;if(max<0)max=0;A.treeScroll=clampi(A.treeScroll,0,max);}InvalidateRect(hwnd,NULL,FALSE);}
        else if(A.mouseX>=A.clientW-A.inspectorW&&!plan_view_active()&&!A.rmp.editMode&&!E.details){E.inspectorScroll=clampi(E.inspectorScroll-d/WHEEL_DELTA*60,0,E.inspectorExtent);InvalidateRect(hwnd,NULL,FALSE);}
        else if(A.mouseX<A.clientW-A.inspectorW){if(ctrl)set_zoom(A.zoom+(d>0?1:-1));else set_z(A.currentZ+(d>0?1:-1));}
        return 0;
    }
    }return DefWindowProcW(hwnd,msg,wp,lp);
}

static HMENU make_menu(void){
    HMENU root=CreateMenu(),f=CreatePopupMenu(),r=CreatePopupMenu(),e=CreatePopupMenu(),v=CreatePopupMenu(),rt=CreatePopupMenu(),bp=CreatePopupMenu(),o=CreatePopupMenu(),h=CreatePopupMenu();
    AppendMenuW(f,MF_STRING,IDM_NEW_MAP,tr(L"Nouvelle MAP..."));AppendMenuW(f,MF_SEPARATOR,0,NULL);AppendMenuW(f,MF_STRING,IDM_OPEN_MAP,tr(L"Ouvrir une MAP..."));AppendMenuW(f,MF_STRING,IDM_SAVE,tr(L"Enregistrer   Ctrl+S"));AppendMenuW(f,MF_STRING,IDM_SAVE_AS,tr(L"Enregistrer vers un mod..."));AppendMenuW(f,MF_STRING,IDM_EXPORT_MOD,tr(L"Exporter / copier le macrobloc vers un mod..."));AppendMenuW(f,MF_SEPARATOR,0,NULL);AppendMenuW(f,MF_STRING,IDM_EXIT,tr(L"Quitter"));AppendMenuW(root,MF_POPUP,(UINT_PTR)f,tr(L"Fichier"));
    AppendMenuW(r,MF_STRING,IDM_SELECT_TFTD,tr(L"Emplacement TFTD ORIGINAL..."));AppendMenuW(r,MF_STRING,IDM_SELECT_OXCE,tr(L"Emplacement OXCE STANDARD..."));AppendMenuW(r,MF_STRING,IDM_SELECT_MODS,tr(L"Emplacement MODS OXCE (user\\mods)..."));AppendMenuW(r,MF_STRING,IDM_SELECT_HD_CUSTOM,tr(L"Emplacement MOD HD manuel / gabarit universel..."));AppendMenuW(r,MF_SEPARATOR,0,NULL);AppendMenuW(r,MF_STRING,IDM_SELECT_MAPS_TFTD,tr(L"Compatibilite : dossier MAP TFTD supplementaire..."));AppendMenuW(r,MF_STRING,IDM_SELECT_MAPS_OXCE,tr(L"Compatibilite : dossier MAP OXCE supplementaire..."));AppendMenuW(r,MF_STRING,IDM_REINDEX,tr(L"Reindexer les ressources"));AppendMenuW(r,MF_SEPARATOR,0,NULL);AppendMenuW(r,MF_STRING,IDM_CLEAR_ACTIVE,tr(L"Vider la palette MAP"));AppendMenuW(r,MF_SEPARATOR,0,NULL);AppendMenuW(r,MF_STRING,IDM_MANUAL_MCD,tr(L"Avance : ajouter un MCD manuellement..."));AppendMenuW(r,MF_STRING,IDM_LOAD_PALETTE,tr(L"Avance : charger une palette LBM..."));AppendMenuW(root,MF_POPUP,(UINT_PTR)r,tr(L"Ressources"));
    AppendMenuW(e,MF_STRING,IDM_UNDO,tr(L"Annuler   Ctrl+Z"));AppendMenuW(e,MF_STRING,IDM_REDO,tr(L"Retablir   Ctrl+Y"));AppendMenuW(e,MF_SEPARATOR,0,NULL);AppendMenuW(e,MF_STRING,IDM_LAYER_FLOOR,tr(L"Sol   F1"));AppendMenuW(e,MF_STRING,IDM_LAYER_WEST,tr(L"Mur ouest   F2"));AppendMenuW(e,MF_STRING,IDM_LAYER_NORTH,tr(L"Mur nord   F3"));AppendMenuW(e,MF_STRING,IDM_LAYER_OBJECT,tr(L"Objet   F4"));AppendMenuW(e,MF_SEPARATOR,0,NULL);AppendMenuW(v,MF_STRING,IDM_Z_UP,tr(L"Niveau Z superieur   Molette haut"));AppendMenuW(v,MF_STRING,IDM_Z_DOWN,tr(L"Niveau Z inferieur   Molette bas"));AppendMenuW(v,MF_STRING,IDM_Z_ALL,tr(L"Basculer : comme dans le jeu / niveau seul"));AppendMenuW(v,MF_STRING,IDM_Z_COMPLETE,tr(L"Vue complete (tous les Z) - vue normale"));AppendMenuW(root,MF_POPUP,(UINT_PTR)e,tr(L"Edition"));
    HMENU renders=CreatePopupMenu();HMENU overlays=CreatePopupMenu();AppendMenuW(overlays,MF_STRING|(A.hdOverlayProvider!=2?MF_CHECKED:0),12320,tr(L"PNG Remastered"));AppendMenuW(overlays,MF_STRING|(A.hdOverlayProvider==2?MF_CHECKED:0),12321,tr(L"Gabarits universels"));AppendMenuW(renders,MF_POPUP,(UINT_PTR)overlays,tr(L"PNG avec REAL HD"));AppendMenuW(renders,MF_STRING|MF_CHECKED,IDM_RENDER_LEGACY,L"Legacy");AppendMenuW(renders,MF_STRING,IDM_RENDER_HD,tr(L"PNG Remastered"));AppendMenuW(renders,MF_STRING,IDM_RENDER_HD_CUSTOM,tr(L"Gabarits universels"));AppendMenuW(renders,MF_STRING,IDM_RENDER_REAL,tr(L"REAL HD texture - apercu SAND/DEBRIS"));AppendMenuW(renders,MF_STRING,IDM_RENDER_DEBUG,tr(L"REAL HD debug - geometrie"));AppendMenuW(renders,MF_SEPARATOR,0,NULL);AppendMenuW(renders,MF_STRING,IDM_RENDER_CYCLE,tr(L"Rendu suivant    F6"));AppendMenuW(renders,MF_STRING,IDM_SELECT_HD_CUSTOM,tr(L"Dossier des gabarits universels..."));AppendMenuW(renders,MF_SEPARATOR,0,NULL);AppendMenuW(renders,MF_STRING,IDM_GEO_NATIVE,tr(L"GEO_TERRAIN : gabarits et assemblages..."));AppendMenuW(renders,MF_STRING,IDM_GEO_LAB,tr(L"GEO_TERRAIN : laboratoire externe des 102 gabarits..."));AppendMenuW(root,MF_POPUP,(UINT_PTR)renders,tr(L"Rendu"));
    AppendMenuW(v,MF_STRING,IDM_FILTER_DORMANT,tr(L"Filtre : LEGACY DORMANT / OXCE UNREACHABLE uniquement"));AppendMenuW(v,MF_SEPARATOR,0,NULL);AppendMenuW(v,MF_STRING|MF_CHECKED,IDM_GRID_TOGGLE,tr(L"Grille du niveau Z   G"));AppendMenuW(v,MF_STRING,IDM_VIEW_NORMAL,tr(L"Vue normale (pieces)"));AppendMenuW(v,MF_STRING,IDM_VIEW_TOP,tr(L"Plan 2D (dessus)"));AppendMenuW(v,MF_STRING,IDM_VIEW_ISO,tr(L"Plan isometrique"));AppendMenuW(v,MF_STRING,IDM_VIEW_PLAN,tr(L"Vues : TUILES / PLAN 2D / PLAN ISO   P"));AppendMenuW(v,MF_SEPARATOR,0,NULL);AppendMenuW(e,MF_STRING,IDM_PLAN_COPY,tr(L"PLAN : copier la selection   Ctrl+C"));AppendMenuW(e,MF_STRING,IDM_PLAN_PASTE,tr(L"PLAN : coller   Ctrl+V"));AppendMenuW(e,MF_STRING,IDM_PLAN_CANCEL_PASTE,tr(L"PLAN : annuler le collage   Echap"));AppendMenuW(v,MF_SEPARATOR,0,NULL);
    AppendMenuW(v,MF_STRING,IDM_ZOOM_IN,tr(L"Zoom avant   PgUp / Ctrl+molette"));AppendMenuW(v,MF_STRING,IDM_ZOOM_OUT,tr(L"Zoom arriere   PgDn / Ctrl+molette"));AppendMenuW(v,MF_STRING,IDM_CENTER,tr(L"Recentrer la vue"));AppendMenuW(root,MF_POPUP,(UINT_PTR)v,tr(L"Affichage"));
    AppendMenuW(rt,MF_STRING,IDM_RMP_TOGGLE,tr(L"Afficher / masquer les routes RMP   R"));
    AppendMenuW(rt,MF_STRING,IDM_RMP_EDIT,tr(L"Mode edition nodes"));
    AppendMenuW(rt,MF_SEPARATOR,0,NULL);
    AppendMenuW(rt,MF_STRING,IDM_RMP_ANALYZE,tr(L"Analyser la MAP / proposer des nodes"));
    AppendMenuW(rt,MF_STRING,IDM_RMP_CLEAR_PROPOSALS,tr(L"Effacer les propositions"));
    AppendMenuW(rt,MF_STRING,IDM_RMP_APPLY_PROPOSALS,tr(L"Appliquer les propositions..."));
    AppendMenuW(rt,MF_SEPARATOR,0,NULL);
    AppendMenuW(rt,MF_STRING,IDM_RMP_SOCKET_N,tr(L"Socket N sur node selectionne"));
    AppendMenuW(rt,MF_STRING,IDM_RMP_SOCKET_E,tr(L"Socket E sur node selectionne"));
    AppendMenuW(rt,MF_STRING,IDM_RMP_SOCKET_S,tr(L"Socket S sur node selectionne"));
    AppendMenuW(rt,MF_STRING,IDM_RMP_SOCKET_W,tr(L"Socket W sur node selectionne"));
    AppendMenuW(rt,MF_SEPARATOR,0,NULL);
    AppendMenuW(rt,MF_STRING,IDM_RMP_SAVE,tr(L"Enregistrer RMP dans le mod..."));
    AppendMenuW(root,MF_POPUP,(UINT_PTR)rt,tr(L"Routes RMP"));
    AppendMenuW(bp,MF_STRING,IDM_BLUEPRINT_HANGAR,tr(L"Ouvrir le Hangar a Blueprints..."));AppendMenuW(bp,MF_STRING,IDM_BLUEPRINT_CAPTURE,tr(L"Nouvelle capture perso..."));AppendMenuW(bp,MF_SEPARATOR,0,NULL);AppendMenuW(bp,MF_STRING,IDM_BLUEPRINT_SELECT_FOLDER,tr(L"Choisir le dossier Hangar..."));AppendMenuW(root,MF_POPUP,(UINT_PTR)bp,tr(L"Assemblages"));
    AppendMenuW(o,MF_STRING,IDM_COMPOSER,tr(L"Compositeur de carte complete..."));AppendMenuW(o,MF_STRING,IDM_SCENE_INSERT_USO,tr(L"Scene : inserer l'USO dans le slot"));AppendMenuW(o,MF_STRING,IDM_SCENE_SAVE,tr(L"Scene : enregistrer le projet .JMW..."));AppendMenuW(o,MF_STRING,IDM_SCENE_OPEN,tr(L"Scene : ouvrir un projet .JMW..."));AppendMenuW(o,MF_SEPARATOR,0,NULL);AppendMenuW(o,MF_STRING,IDM_CLOSE_SCENE,tr(L"Fermer la vue compositeur"));AppendMenuW(root,MF_POPUP,(UINT_PTR)o,tr(L"Compositeur"));AppendMenuW(root,MF_STRING,IDM_PROCEDURAL,tr(L"Procedural..."));
    AppendMenuW(h,MF_STRING,12311,tr(L"Contact / assistance..."));AppendMenuW(h,MF_STRING,IDM_ABOUT,tr(L"A propos"));AppendMenuW(root,MF_POPUP,(UINT_PTR)h,tr(L"Aide"));HMENU language=CreatePopupMenu(),tutorial=CreatePopupMenu();for(int l=0;l<4;l++)AppendMenuW(language,MF_STRING|(i18nLanguage==l?MF_CHECKED:0),IDM_LANGUAGE_BASE+l,i18nNames[l]);AppendMenuW(root,MF_POPUP,(UINT_PTR)language,tr(L"Langage"));AppendMenuW(tutorial,MF_STRING,IDM_TUTORIAL,tr(L"Guide complet..."));AppendMenuW(root,MF_POPUP,(UINT_PTR)tutorial,tr(L"Tuto"));return root;
}

int WINAPI WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show){
    (void)prev;(void)cmd;ZeroMemory(&A,sizeof(A));A.sidebarW=380;A.inspectorW=INSPECTOR_W;A.zoom=2;A.selectedLayer=3;A.viewMode=0;A.planTool=0;A.planSemantic=PLAN_FLOOR;A.planObjectSemantic=PLAN_OBJ_OBJECT;A.planSemanticLayer=0;A.planLinkType=1;A.brushSize=1;A.brushMode=0;A.brushShape=0;A.showAllBelow=1;A.showGrid=1;A.browserMode=0;A.resourceFilter=-1;A.assetRenderMode=0;A.selectedMap=-1;A.expandedLib=A.expandedActive=-1;A.selectedLib=-1;A.blueprintIndex=-1;A.customBlueprintIndex=-1;A.blueprintMode=0;A.lastPaintX=A.lastPaintY=-999;A.painting=0;A.erasing=0;A.rmpTool=0;A.rmpNewType=0;A.rmpNewRank=0;A.rmpNewFlags=1;A.rmpNewTarget=0;A.rmpNewPriority=0;A.rmpAdvanced=0;
    wcscpy(A.status,tr(L"Pret. Configurez TFTD ORIGINAL, OXCE STANDARD et MODS OXCE dans le menu Ressources."));default_palette();config_path_init();config_load();wcscpy(A.status,tr(L"Pret. Configurez TFTD ORIGINAL, OXCE STANDARD et MODS OXCE dans le menu Ressources."));custom_blueprints_path_init();custom_blueprints_load();CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
    GdiplusStartupInput gdsi;ZeroMemory(&gdsi,sizeof(gdsi));gdsi.GdiplusVersion=1;GdiplusStartup(&gGdiPlusToken,&gdsi,NULL);
    WNDCLASSEXW wc;ZeroMemory(&wc,sizeof(wc));wc.cbSize=sizeof(wc);wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=wndproc;wc.hInstance=inst;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpszClassName=L"TFTDWorkshopJM270";if(!RegisterClassExW(&wc))return 1;
    HWND w=CreateWindowExW(0,wc.lpszClassName,APP_TITLE,WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1600,920,NULL,NULL,inst,NULL);if(!w)return 2;A.hwnd=w;SetMenu(w,make_menu());
    A.searchEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,60,136,A.sidebarW-68,24,w,(HMENU)(INT_PTR)SEARCH_ID,inst,NULL);
    SendMessageW(A.searchEdit,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
    ShowWindow(w,show);UpdateWindow(w);
    if((A.tftdRoot[0]&&path_exists_dir(A.tftdRoot))||(A.oxceRoot[0]&&path_exists_dir(A.oxceRoot))||(A.modsRoot[0]&&path_exists_dir(A.modsRoot)))reindex_resources();
    MSG m;while(GetMessageW(&m,NULL,0,0)>0){if(PUI.hwnd&&IsDialogMessageW(PUI.hwnd,&m))continue;TranslateMessage(&m);DispatchMessageW(&m);}proc_catalog_clear();hd_cache_clear();library_clear();active_clear();free_map();scene_free();free(A.backbuf);free(A.overviewBuf);free(A.planClip);free(sceneGeoDepth);free(sceneGeoPick);if(gGdiPlusToken)GdiplusShutdown(gGdiPlusToken);CoUninitialize();return 0;
}
