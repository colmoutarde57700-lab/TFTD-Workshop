/* Editor state: transient interaction, separate from MAP/JMW data. */
enum { EDIT_SELECT, EDIT_PLACE, EDIT_ERASE, EDIT_PICK };
enum { UI_SELECT=1300, UI_PLACE, UI_ERASE, UI_PICK, UI_MOVE, UI_DUPLICATE, UI_DELETE,
 UI_CLEAR, UI_SAVE_ASSEMBLY, UI_SCOPE, UI_REPLACE, UI_DETAILS, UI_WORKING_COPY,
 UI_VIEW, UI_LEVELS, UI_BRUSH, UI_ASSEMBLIES, UI_FRAME, UI_BELOW, UI_ABOVE,
 UI_OPACITY, UI_COPY, UI_PASTE, UI_HELP, UI_HOME, UI_END };
#define EDITOR_TOP 208
#define EDITOR_CAP 1024
typedef struct {
 int tool,count,moving,scope,replace,details,strokeGroup;
 int clipCount,below,above,opacity,planErase,inspectorScroll,inspectorExtent,planPanelX,planPanelY;
 BlueprintCapturePart parts[EDITOR_CAP],clip[EDITOR_CAP];
 wchar_t selectionName[120],reason[240];
 HFONT font,titleFont;
} WorkshopEditor;
static WorkshopEditor E={.below=1,.above=0,.opacity=50};
static void editor_reset(void);
static void editor_cancel(void);
static void editor_tool(int tool);
static void editor_draw_overlay(void);
static void editor_draw_header(HDC dc);
static void editor_draw_inspector(HDC dc);
static int editor_header_click(int x,int y);
static int editor_inspector_click(int x,int y);
static int editor_left(int x,int y,WPARAM modifiers);
static int editor_key(int key,int ctrl);
static int editor_action(int id);
static int editor_canvas(int x,int y);
static void editor_selection_clear(void);
static void editor_palette_selected(void);

static int editor_begin_transform(int mode);
static int editor_commit_transform(int x,int y,int z);
static void editor_select_at(int x,int y,int extend);
static void editor_frame(void);
static int editor_opacity_for_z(int z);
static void editor_apply_opacity(unsigned*pixel);
static int editorRenderOpacity=100;
static int editor_locked(int x,int y,int z);
static int editor_stamp(int x,int y);
static void handle_command(int id);
