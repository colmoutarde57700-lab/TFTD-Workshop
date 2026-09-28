#include "../src/tftd_workshop_v2_12_1.c"
int main(int argc,char**argv){if(argc!=3)return 2;GetFullPathNameW(L"work/fresh-view.ini",PATH_CAP,A.configPath,NULL);int mode=atoi(argv[2]);if(!strcmp(argv[1],"write")){set_normal_visibility(mode);return 0;}config_load();int ok=A.showComplete==(mode==2)&&A.showAllBelow==(mode!=0);printf("fresh process mode %d: %s\n",mode,ok?"PASS":"FAIL");return ok?0:1;}
