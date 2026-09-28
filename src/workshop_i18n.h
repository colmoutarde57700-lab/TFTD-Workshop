/* Presentation translations. Resource identifiers and user documents are untouched. */
#include "workshop_i18n_data.h"
static int i18nLanguage;
static const wchar_t*i18nCodes[]={L"fr",L"en",L"es",L"de"};
static const wchar_t*i18nNames[]={L"Français",L"English",L"Español",L"Deutsch"};
static const wchar_t*tr(const wchar_t*s){if(!s)return s;for(size_t i=0;i<sizeof(i18nEntries)/sizeof(*i18nEntries);i++)if(!wcscmp(s,i18nEntries[i].text[0]))return i18nEntries[i].text[i18nLanguage];for(size_t i=0;i<sizeof(i18nEntries)/sizeof(*i18nEntries);i++)for(int l=1;l<4;l++)if(!wcscmp(s,i18nEntries[i].text[l]))return i18nEntries[i].text[i18nLanguage];return s;}
static void i18n_load(const wchar_t*path){wchar_t code[16];GetPrivateProfileStringW(L"TFTDWorkshop",L"Language",L"fr",code,16,path);i18nLanguage=0;for(int l=0;l<4;l++)if(!wcscmp(code,i18nCodes[l]))i18nLanguage=l;}
static BOOL i18n_textout(HDC dc,int x,int y,LPCWSTR s,int n){const wchar_t*t=tr(s);return TextOutW(dc,x,y,t,t!=s?(int)wcslen(t):n);}
static int i18n_drawtext(HDC dc,LPCWSTR s,int n,LPRECT r,UINT flags){const wchar_t*t=tr(s);return DrawTextW(dc,t,t!=s?-1:n,r,flags);}
#include <stdarg.h>
static wchar_t i18nLast[4][8192];
static int i18nLastValid;
typedef struct {HWND h;wchar_t*text[4];} I18nCaption;
static I18nCaption i18nCaptions[128];
static int i18n_snwprintf(wchar_t*buf,size_t cap,const wchar_t*fmt,...){va_list args;va_start(args,fmt);va_list copy;va_copy(copy,args);int n=_vsnwprintf(buf,cap,fmt,copy);va_end(copy);i18nLastValid=0;const I18nEntry*e=NULL;for(size_t k=0;k<sizeof(i18nEntries)/sizeof(*i18nEntries)&&!e;k++)for(int l=0;l<4;l++)if(!wcscmp(fmt,i18nEntries[k].text[l])){e=&i18nEntries[k];break;}if(e){for(int l=0;l<4;l++){va_copy(copy,args);_vsnwprintf(i18nLast[l],8191,e->text[l],copy);va_end(copy);i18nLast[l][8191]=0;}i18nLastValid=1;}va_end(args);return n;}
static BOOL i18n_windowtext(HWND h,LPCWSTR s){if(i18nLastValid&&s&&!wcscmp(s,i18nLast[i18nLanguage])){int slot=-1;for(int k=0;k<128;k++){if(i18nCaptions[k].h==h){slot=k;break;}if(!i18nCaptions[k].h&&slot<0)slot=k;}if(slot>=0){I18nCaption*c=&i18nCaptions[slot];c->h=h;for(int l=0;l<4;l++){free(c->text[l]);c->text[l]=_wcsdup(i18nLast[l]);}}}return SetWindowTextW(h,tr(s));}
#define _snwprintf i18n_snwprintf
static HWND i18n_create(DWORD ex,LPCWSTR cls,LPCWSTR s,DWORD style,int x,int y,int w,int h,HWND parent,HMENU menu,HINSTANCE inst,LPVOID data){return CreateWindowExW(ex,cls,tr(s),style,x,y,w,h,parent,menu,inst,data);}
static BOOL i18n_menu(HMENU m,UINT flags,UINT_PTR id,LPCWSTR s){return AppendMenuW(m,flags,id,(flags&MF_SEPARATOR)?s:tr(s));}
static int i18n_message(HWND h,LPCWSTR s,LPCWSTR title,UINT flags){return MessageBoxW(h,tr(s),tr(title),flags);}
static LRESULT i18n_send(HWND h,UINT msg,WPARAM wp,LPARAM lp){if((msg==CB_ADDSTRING||msg==LB_ADDSTRING)&&lp)lp=(LPARAM)tr((LPCWSTR)lp);return SendMessageW(h,msg,wp,lp);}
#define TextOutW i18n_textout
#define DrawTextW i18n_drawtext
#define SetWindowTextW i18n_windowtext
#define CreateWindowExW i18n_create
#undef CreateWindowW
#define CreateWindowW(c,t,s,x,y,w,h,p,m,i,d) i18n_create(0,c,t,s,x,y,w,h,p,m,i,d)
#define AppendMenuW i18n_menu
#ifndef WORKSHOP_TEST
#undef MessageBoxW
#define MessageBoxW i18n_message
#endif
#define SendMessageW i18n_send
#define IDM_LANGUAGE_BASE 12300
#define IDM_TUTORIAL 12310
static BOOL CALLBACK i18n_refresh_child(HWND h,LPARAM unused){(void)unused;wchar_t cls[80],text[8192];GetClassNameW(h,cls,80);if(_wcsicmp(cls,L"EDIT")&&_wcsnicmp(cls,L"RICHEDIT",8)){GetWindowTextW(h,text,8192);for(int k=0;k<128;k++)if(i18nCaptions[k].h==h){I18nCaption*c=&i18nCaptions[k];int matches=0;for(int l=0;l<4;l++)if(c->text[l]&&!wcscmp(text,c->text[l]))matches=1;if(matches&&c->text[i18nLanguage]){SetWindowTextW(h,c->text[i18nLanguage]);GetWindowTextW(h,text,8192);}break;}const wchar_t*t=tr(text);if(t!=text)SetWindowTextW(h,t);}
 if(!_wcsicmp(cls,L"COMBOBOX")){int count=(int)SendMessageW(h,CB_GETCOUNT,0,0),selection=(int)SendMessageW(h,CB_GETCURSEL,0,0);for(int n=0;n<count;n++){if(SendMessageW(h,CB_GETLBTEXTLEN,n,0)>=8192)continue;SendMessageW(h,CB_GETLBTEXT,n,(LPARAM)text);const wchar_t*t=tr(text);if(t==text)continue;LRESULT data=SendMessageW(h,CB_GETITEMDATA,n,0);SendMessageW(h,CB_DELETESTRING,n,0);SendMessageW(h,CB_INSERTSTRING,n,(LPARAM)t);SendMessageW(h,CB_SETITEMDATA,n,data);}SendMessageW(h,CB_SETCURSEL,selection,0);}
 InvalidateRect(h,NULL,TRUE);return TRUE;}
static BOOL CALLBACK i18n_refresh_window(HWND h,LPARAM unused){i18n_refresh_child(h,unused);EnumChildWindows(h,i18n_refresh_child,0);return TRUE;}
