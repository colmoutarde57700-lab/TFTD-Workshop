#include <windows.h>
#include <stdio.h>
static int dialogAnswer=IDNO;
static int test_message(HWND h,LPCWSTR t,LPCWSTR c,UINT u){(void)h;(void)c;(void)u;wprintf(L"DIALOG: %ls\n",t);return dialogAnswer;}
#define MessageBoxW test_message
#include "../src/tftd_workshop_v2_12_1.c"
static int failures,checks;
#define CHECK(c,m) do{int result=!!(c);checks++;printf("[%s] %s\n",result?"PASS":"FAIL",m);if(!result)failures++;}while(0)

static HWND panel(void){WNDCLASSW w={0};w.lpfnWndProc=proc_wndproc;w.hInstance=GetModuleHandleW(NULL);w.lpszClassName=L"PrefsTest";RegisterClassW(&w);return CreateWindowW(w.lpszClassName,L"Prefs",WS_OVERLAPPEDWINDOW,0,0,636,630,A.hwnd,NULL,w.hInstance,NULL);}
static void select_map(HWND c,int mi){for(int i=0;i<SendMessageW(c,CB_GETCOUNT,0,0);i++)if(SendMessageW(c,CB_GETITEMDATA,i,0)==mi){SendMessageW(c,CB_SETCURSEL,i,0);return;}}
static uint32_t seed(void){wchar_t b[64];GetWindowTextW(PUI.seed,b,64);return (uint32_t)wcstoull(b,NULL,10);}
static int prefs(int craft,int uso){return edit_get_int(PUI.width,0)==80&&edit_get_int(PUI.height,0)==90&&SendMessageW(PUI.density,CB_GETCURSEL,0,0)==2&&SendMessageW(PUI.relief,CB_GETCURSEL,0,0)==1&&SendMessageW(PUI.gap,CB_GETCURSEL,0,0)==0&&proc_combo_data(PUI.craft)==craft&&proc_combo_data(PUI.uso)==uso;}
int main(int argc,char**argv){if(argc<4)return 2;
MultiByteToWideChar(CP_UTF8,0,argv[1],-1,A.tftdRoot,PATH_CAP);MultiByteToWideChar(CP_UTF8,0,argv[2],-1,A.oxceRoot,PATH_CAP);
A.sidebarW=240;A.inspectorW=280;A.zoom=1;A.brushSize=1;A.selectedLib=-1;A.expandedLib=A.expandedActive=-1;A.selectedMap=-1;A.autoProfileIndex=-1;
WNDCLASSW wc={0};wc.lpfnWndProc=wndproc;wc.hInstance=GetModuleHandleW(NULL);wc.lpszClassName=L"ProcTest";RegisterClassW(&wc);A.hwnd=CreateWindowW(wc.lpszClassName,L"Procedural tests",WS_OVERLAPPEDWINDOW,0,0,2560,1500,NULL,make_menu(),wc.hInstance,NULL);RECT cr;GetClientRect(A.hwnd,&cr);A.clientW=cr.right;A.clientH=cr.bottom;ensure_backbuf(A.clientW,A.clientH);
default_palette();scan_dir_recursive(A.tftdRoot,SRC_TFTD,0,L"TFTD ORIGINAL");scan_dir_recursive(A.oxceRoot,SRC_OXCE,0,L"OXCE STANDARD");wchar_t mirrorRoot[PATH_CAP];MultiByteToWideChar(CP_UTF8,0,argv[3],-1,mirrorRoot,PATH_CAP);scan_dir_recursive(mirrorRoot,SRC_MOD,0,L"JM_USO_SYMETRIE");library_sort();map_sort();map_refresh_profiles();

GetFullPathNameW(L"work/preferences-isolated.ini",PATH_CAP,A.configPath,NULL);
int craft=exact_map_by_name(L"TRITON",SRC_TFTD,L"TFTD ORIGINAL"),uso=exact_map_by_name(L"JMUFO08",SRC_MOD,L"JM_USO_SYMETRIE");
CHECK(craft>=0&&uso>=0,"ship fixtures found");
if(argc>4){HWND h=panel();CHECK(prefs(craft,uso)&&seed()==UINT32_MAX,"fresh process restores all settings and maximum seed");DestroyWindow(h);printf("RESULT %d/%d\n",checks-failures,checks);return failures?1:0;}
DeleteFileW(A.configPath);WritePrivateProfileStringW(L"Other",L"Keep",L"intact",A.configPath);
HWND h=panel();SetWindowTextW(PUI.width,L"80");SetWindowTextW(PUI.height,L"90");SetWindowTextW(PUI.seed,L"4294967295");SendMessageW(PUI.density,CB_SETCURSEL,2,0);SendMessageW(PUI.relief,CB_SETCURSEL,1,0);SendMessageW(PUI.gap,CB_SETCURSEL,0,0);select_map(PUI.craft,craft);select_map(PUI.uso,uso);
CHECK(proc_ui_generate(),"successful UI generation");DestroyWindow(h);h=panel();CHECK(prefs(craft,uso)&&seed()==UINT32_MAX,"close/reopen restores dimensions settings and exact mirrored ship");
wchar_t b[1024];GetPrivateProfileStringW(L"Other",L"Keep",L"",b,1024,A.configPath);CHECK(!wcscmp(b,L"intact"),"unrelated INI settings preserved");
uint32_t prev=seed();SceneCell*old=A.scene.cells;A.scene.dirty=0;SendMessageW(h,WM_COMMAND,PROC_NEW_SEED,0);CHECK(seed()!=prev&&A.scene.cells!=old&&prefs(craft,uso),"random button generates a new scene and changes only seed");
prev=seed();A.scene.dirty=0;SendMessageW(h,WM_COMMAND,PROC_NEW_SEED,0);CHECK(seed()!=prev&&prefs(craft,uso),"successive random generation changes seed");
prev=seed();old=A.scene.cells;dialogAnswer=IDCANCEL;SendMessageW(h,WM_COMMAND,PROC_NEW_SEED,0);CHECK(seed()==prev&&A.scene.cells==old,"cancel preserves scene and seed");dialogAnswer=IDNO;
SetWindowTextW(PUI.width,L"5");SendMessageW(h,WM_COMMAND,PROC_NEW_SEED,0);CHECK(seed()==prev&&A.scene.cells==old,"failed generation preserves scene and restores seed");DestroyWindow(h);h=panel();CHECK(prefs(craft,uso)&&seed()==prev,"failed settings never replace last successful recipe");
WritePrivateProfileStringW(PROC_PREFS,L"UsoMap",L"Z:\\missing\\UFO08.MAP",A.configPath);DestroyWindow(h);h=panel();GetWindowTextW(PUI.report,b,1024);CHECK(proc_combo_data(PUI.uso)==-1&&wcsstr(b,L"introuvable"),"missing ship selects None and warns");
select_map(PUI.uso,uso);SetWindowTextW(PUI.seed,L"4294967295");A.scene.dirty=0;CHECK(proc_ui_generate(),"final recipe persisted for fresh process test");DestroyWindow(h);
printf("RESULT %d/%d\n",checks-failures,checks);return failures?1:0;}

