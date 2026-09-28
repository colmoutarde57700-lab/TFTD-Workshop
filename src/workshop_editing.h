/* Selection and transactional assembly editing. All mutations use existing undo records. */
static void editor_selection_clear(void){E.count=0;E.moving=0;E.selectionName[0]=0;}
static void editor_reset(void){editor_selection_clear();E.clipCount=0;E.tool=EDIT_SELECT;E.strokeGroup=0;E.planErase=0;E.inspectorScroll=0;}
static int editor_canvas(int x,int y){return x>A.sidebarW+5&&x<A.clientW-A.inspectorW-5&&y>=EDITOR_TOP&&y<A.clientH-30;}
static int editor_read(int x,int y,int z,int layer,BlueprintCapturePart*out){
 int lib=-1,local=-1;if(layer<0||layer>3)return 0;
 if(A.scene.active){SceneCell*c=scene_cell_at(x,y,z);if(!c)return 0;lib=c->lib[layer];local=c->local[layer];}
 else{MapCell*c=cell_at(x,y,z);if(!c||!active_resolve_raw(c->part[layer],&lib,&local))return 0;}
 if(lib<0||lib>=A.libraryCount||local<0)return 0;
 if(out){out->x=x;out->y=y;out->z=z;out->layer=layer;out->lib=lib;out->local=local;}return 1;
}
static int editor_index(int x,int y,int z,int layer){for(int i=0;i<E.count;i++){BlueprintCapturePart*p=&E.parts[i];if(p->x==x&&p->y==y&&p->z==z&&p->layer==layer)return i;}return -1;}
static int editor_same(BlueprintCapturePart a,BlueprintCapturePart b){return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.layer==b.layer&&a.lib==b.lib&&a.local==b.local;}
static int editor_locked(int x,int y,int z){PlanCell*p=plan_cell_at(x,y,z);return p&&(p->flags&PLAN_FLAG_LOCKED);}
static int editor_editable(void){
 if(A.scene.active||(!current_map_is_protected()&&A.map.cells))return 1;
 set_status(tr(L"Original en lecture seule : cliquez sur 'Copie de travail' dans le panneau droit."));return 0;
}
static int editor_blueprint_parts(int mode,int bi,int anchor,BlueprintCapturePart*out){
 int n=0,ax=0,ay=0,az=0;
 if(mode==1){if(bi<0||bi>=BLUEPRINT_RC15_COUNT)return 0;const BlueprintDef*b=&BLUEPRINTS_RC15[bi];n=b->partCount;if(n>EDITOR_CAP||anchor<0||anchor>=n)return 0;const BlueprintPartDef*a=&BLUEPRINT_PARTS_RC15[b->firstPart+anchor];ax=a->dx;ay=a->dy;az=a->dz;
  for(int j=0;j<n;j++){const BlueprintPartDef*p=&BLUEPRINT_PARTS_RC15[b->firstPart+j];int lib=blueprint_resolve_lib(p->dataset);if(lib<0||p->mcd>=A.library[lib].mcdCount)return 0;out[j]=(BlueprintCapturePart){.x=p->dx-ax,.y=p->dy-ay,.z=p->dz-az,.layer=p->layer,.lib=lib,.local=p->mcd};}}
 else if(mode==2){if(bi<0||bi>=A.customBlueprintCount)return 0;CustomBlueprintDef*b=&A.customBlueprints[bi];n=b->partCount;if(n>EDITOR_CAP||anchor<0||anchor>=n)return 0;CustomBlueprintPart*a=&A.customBlueprintParts[b->firstPart+anchor];ax=a->dx;ay=a->dy;az=a->dz;
  for(int j=0;j<n;j++){CustomBlueprintPart*p=&A.customBlueprintParts[b->firstPart+j];int lib=blueprint_resolve_lib(p->dataset);if(lib<0||p->mcd>=A.library[lib].mcdCount)return 0;out[j]=(BlueprintCapturePart){.x=p->dx-ax,.y=p->dy-ay,.z=p->dz-az,.layer=p->layer,.lib=lib,.local=p->mcd};}}
 return n;
}
static void editor_recognize(BlueprintCapturePart hit){
 /* Only complete, exact matches. User explicitly opts into assembly selection. */
 BlueprintCapturePart candidate[EDITOR_CAP];int best=1;E.parts[0]=hit;E.count=1;wcscpy(E.selectionName,tr(L"Piece seule"));
 int oldLib=A.selectedLib;A.selectedLib=hit.lib;
 for(int mode=2;mode>=1;mode--){int limit=mode==1?BLUEPRINT_RC15_COUNT:A.customBlueprintCount;
  for(int bi=0;bi<limit;bi++){int n=mode==1?BLUEPRINTS_RC15[bi].partCount:A.customBlueprints[bi].partCount;if(n<=best||n>EDITOR_CAP)continue;
   for(int anchor=0;anchor<n;anchor++){int local,layer;const wchar_t*dataset;
    if(mode==1){const BlueprintPartDef*p=&BLUEPRINT_PARTS_RC15[BLUEPRINTS_RC15[bi].firstPart+anchor];dataset=p->dataset;local=p->mcd;layer=p->layer;}
    else{CustomBlueprintPart*p=&A.customBlueprintParts[A.customBlueprints[bi].firstPart+anchor];dataset=p->dataset;local=p->mcd;layer=p->layer;}
    if(local!=hit.local||layer!=hit.layer||_wcsicmp(dataset,A.library[hit.lib].name))continue;
    if(editor_blueprint_parts(mode,bi,anchor,candidate)!=n)continue;int valid=1;
    for(int j=0;j<n;j++){candidate[j].x+=hit.x;candidate[j].y+=hit.y;candidate[j].z+=hit.z;BlueprintCapturePart actual;if(!editor_read(candidate[j].x,candidate[j].y,candidate[j].z,candidate[j].layer,&actual)||!editor_same(actual,candidate[j])){valid=0;break;}}
    if(valid){memcpy(E.parts,candidate,n*sizeof(*candidate));BlueprintCapturePart first=E.parts[0];E.parts[0]=E.parts[anchor];E.parts[anchor]=first;E.count=best=n;_snwprintf(E.selectionName,119,L"%ls",mode==1?tr(L"Assemblage reconnu (Bible RC15)"):A.customBlueprints[bi].name);break;}
   }
  }
 }
 A.selectedLib=oldLib;
}
static void editor_select_at(int mx,int my,int extend){
 BlueprintCapturePart hit;if(!hit_piece_at_levels(mx,my,&hit.x,&hit.y,&hit.z,&hit.layer,&hit.lib,&hit.local,A.currentZ,A.currentZ)){if(!extend)editor_selection_clear();InvalidateRect(A.hwnd,NULL,FALSE);return;}
 E.moving=0;if(!extend){if(E.scope)editor_recognize(hit);else{E.count=1;E.parts[0]=hit;wcscpy(E.selectionName,tr(L"Piece seule"));}}
 else{int i=editor_index(hit.x,hit.y,hit.z,hit.layer);if(i>=0){memmove(E.parts+i,E.parts+i+1,(E.count-i-1)*sizeof(E.parts[0]));E.count--;}else if(E.count<EDITOR_CAP)E.parts[E.count++]=hit;else set_status(tr(L"Selection limitee a 1024 pieces."));wcscpy(E.selectionName,tr(L"Selection manuelle"));}
 E.tool=EDIT_SELECT;A.selectedLib=-1;A.blueprintMode=0;wchar_t msg[220];_snwprintf(msg,219,L"%d piece(s) selectionnee(s). Maj+clic ajoute/retire. Deplacer, Dupliquer ou Suppr.",E.count);set_status(msg);InvalidateRect(A.hwnd,NULL,FALSE);
}
static int editor_validate(const BlueprintCapturePart*parts,int count,int dx,int dy,int dz,int moving){
 E.reason[0]=0;int width=A.scene.active?A.scene.x:A.map.x,height=A.scene.active?A.scene.y:A.map.y,levels=doc_zmax();
 for(int i=0;i<count;i++){const BlueprintCapturePart*p=&parts[i];int x=p->x+dx,y=p->y+dy,z=p->z+dz;
  if(p->layer<0||p->layer>3||p->lib<0||p->lib>=A.libraryCount||p->local<0||p->local>=A.library[p->lib].mcdCount){wcscpy(E.reason,tr(L"Reference de piece invalide."));return 0;}
  for(int j=0;j<i;j++)if(parts[j].x==p->x&&parts[j].y==p->y&&parts[j].z==p->z&&parts[j].layer==p->layer){wcscpy(E.reason,tr(L"Deux pieces occupent le meme emplacement dans l'assemblage."));return 0;}
  if(x<0||y<0||z<0||x>=width||y>=height||z>=levels){wcscpy(E.reason,tr(L"Hors de la carte : choisissez une autre position ou un autre Z."));return 0;}
  if(editor_locked(x,y,z)||(moving&&editor_locked(p->x,p->y,p->z))){wcscpy(E.reason,tr(L"Une case est verrouillee. Deverrouillez-la avant cette action."));return 0;}
  if(moving){BlueprintCapturePart old;if(!editor_read(p->x,p->y,p->z,p->layer,&old)||!editor_same(old,*p)){wcscpy(E.reason,tr(L"Le contenu a change. Refaites la selection."));return 0;}}
  BlueprintCapturePart other;int occupied=editor_read(x,y,z,p->layer,&other);if(!A.scene.active){MapCell*c=cell_at(x,y,z);if(c&&c->part[p->layer])occupied=1;}
  if(occupied&&!E.replace&&!(moving&&editor_index(x,y,z,p->layer)>=0)){wcscpy(E.reason,tr(L"Place occupee. Changez de position ou activez 'Remplacer' explicitement."));return 0;}
 }
 return 1;
}
static int editor_prepare_palette(const BlueprintCapturePart*parts,int count,int*raws){
 if(A.scene.active)return 1;ActiveSet draft[MAX_ACTIVE];memcpy(draft,A.active,sizeof(draft));int n=A.activeCount,total=A.activeTotalMcd;
 if(n==0&&total==0&&A.autoProfileIndex<0)total=1;
 for(int i=0;i<count;i++){int lib=parts[i].lib,local=parts[i].local;if(lib<0||lib>=A.libraryCount||local<0||local>=A.library[lib].mcdCount){wcscpy(E.reason,tr(L"Une ressource de la selection manque."));return 0;}
  int slot=-1;for(int k=0;k<n;k++)if(draft[k].libIndex==lib){slot=k;break;}
  if(slot<0){if(n>=MAX_ACTIVE){wcscpy(E.reason,tr(L"Trop de jeux de pieces dans cette carte."));return 0;}slot=n++;draft[slot].libIndex=lib;draft[slot].baseIndex=total;total+=A.library[lib].mcdCount;}
  int raw=draft[slot].baseIndex+local;if(raw<=0||raw>255){wcscpy(E.reason,tr(L"Cette piece depasse la capacite du format MAP. Utilisez une scene ou un autre jeu de pieces."));return 0;}raws[i]=raw;
 }
 if(n>A.activeCount){size_t cells=(size_t)A.map.x*A.map.y*A.map.z;for(size_t i=0;i<cells;i++)for(int layer=0;layer<4;layer++){int raw=A.map.cells[i].part[layer],lib,local;if(raw&&!active_resolve_raw(raw,&lib,&local)&&raw>=A.activeTotalMcd&&raw<total){wcscpy(E.reason,tr(L"References MAP non resolues : corrigez le profil avant d'ajouter ces pieces."));return 0;}}}
 memcpy(A.active,draft,sizeof(draft));A.activeCount=n;A.activeTotalMcd=total;return 1;
}
static int editor_apply_batch(BlueprintCapturePart*parts,int count,int dx,int dy,int dz,int moving){
 if(count<1||count>EDITOR_CAP||!editor_editable())return 0;
 if(!editor_validate(parts,count,dx,dy,dz,moving)){set_status(E.reason);return 0;}
 int raws[EDITOR_CAP];if(!editor_prepare_palette(parts,count,raws)){set_status(E.reason);return 0;}
 A.currentEditGroup=E.strokeGroup?E.strokeGroup:++A.nextEditGroup;
 if(moving)for(int i=0;i<count;i++){BlueprintCapturePart*p=&parts[i];if(A.scene.active)apply_scene_edit_value(p->x,p->y,p->z,p->layer,-1,-1,1);else apply_edit_value(p->x,p->y,p->z,p->layer,0,1);}
 for(int i=0;i<count;i++){BlueprintCapturePart*p=&parts[i];int x=p->x+dx,y=p->y+dy,z=p->z+dz;if(A.scene.active)apply_scene_edit_value(x,y,z,p->layer,p->lib,p->local,1);else apply_edit_value(x,y,z,p->layer,(uint8_t)raws[i],1);}
 A.currentEditGroup=0;A.overviewDirty=1;return 1;
}
static int editor_begin_transform(int mode){
 if(!E.count){set_status(tr(L"Selectionnez une piece ou un assemblage sur la carte."));return 0;}if(mode==1&&!editor_editable())return 0;
 E.strokeGroup=0;E.moving=mode;E.tool=EDIT_SELECT;A.currentZ=E.parts[0].z;A.selectedLib=-1;A.blueprintMode=0;A.painting=A.erasing=0;
 set_status(mode==1?tr(L"Deplacement : cliquez la destination. Molette = Z. Clic droit / Echap = annuler."):tr(L"Duplication : cliquez la destination. Molette = Z. Clic droit / Echap = annuler."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;
}
static int editor_commit_transform(int x,int y,int z){
 if(!E.moving||!E.count)return 0;int dx=x-E.parts[0].x,dy=y-E.parts[0].y,dz=z-E.parts[0].z;
 if(!editor_apply_batch(E.parts,E.count,dx,dy,dz,E.moving==1))return 0;
 for(int i=0;i<E.count;i++){E.parts[i].x+=dx;E.parts[i].y+=dy;E.parts[i].z+=dz;}E.moving=0;set_status(tr(L"Montage pose. La selection reste active. Ctrl+Z annule toute l'operation."));InvalidateRect(A.hwnd,NULL,FALSE);return 1;
}
static void editor_delete_selection(void){
 if(!E.count||!editor_editable())return;
 for(int i=0;i<E.count;i++){BlueprintCapturePart*p=&E.parts[i],actual;if(editor_locked(p->x,p->y,p->z)||!editor_read(p->x,p->y,p->z,p->layer,&actual)||!editor_same(*p,actual)){set_status(tr(L"Selection modifiee ou verrouillee : suppression annulee."));return;}}
 A.currentEditGroup=++A.nextEditGroup;for(int i=0;i<E.count;i++){BlueprintCapturePart*p=&E.parts[i];if(A.scene.active)apply_scene_edit_value(p->x,p->y,p->z,p->layer,-1,-1,1);else apply_edit_value(p->x,p->y,p->z,p->layer,0,1);}A.currentEditGroup=0;editor_selection_clear();set_status(tr(L"Selection supprimee. Ctrl+Z restaure l'ensemble."));InvalidateRect(A.hwnd,NULL,FALSE);
}
static int editor_stamp(int x,int y){
 BlueprintCapturePart parts[EDITOR_CAP];int n=0;
 if(A.blueprintMode==1)n=editor_blueprint_parts(1,A.blueprintIndex,blueprint_anchor_part(A.blueprintIndex),parts);
 else if(A.blueprintMode==2)n=editor_blueprint_parts(2,A.customBlueprintIndex,custom_blueprint_anchor_part(A.customBlueprintIndex),parts);
 else if(A.selectedLib>=0){parts[0]=(BlueprintCapturePart){.layer=A.selectedLayer,.lib=A.selectedLib,.local=A.selectedLocal};n=1;}
 if(!n){set_status(tr(L"Choisissez une piece dans la bibliotheque, ou un assemblage."));return 0;}
 for(int i=0;i<n;i++){parts[i].x+=x;parts[i].y+=y;parts[i].z+=A.currentZ;}
 if(!editor_apply_batch(parts,n,0,0,0,0))return 0;
 memcpy(E.parts,parts,n*sizeof(*parts));E.count=n;wcscpy(E.selectionName,n>1?tr(L"Assemblage pose"):tr(L"Piece posee"));return 1;
}
static void editor_tool(int tool){E.moving=0;E.tool=tool;A.painting=A.erasing=0;E.strokeGroup=0;A.blueprintCaptureMode=0;ReleaseCapture();set_status(tool==EDIT_SELECT?tr(L"Cliquez une piece. Maj+clic ajoute / retire. Choisissez 'Assemblage' pour reconnaitre un montage."):tool==EDIT_PLACE?tr(L"Choisissez une piece ou un assemblage, puis cliquez pour placer."):tool==EDIT_ERASE?tr(L"Gomme : clic gauche sur une piece du Z actif. Clic droit quitte la gomme."):tr(L"Pipette : cliquez une piece du Z actif pour la reprendre en main."));InvalidateRect(A.hwnd,NULL,FALSE);}
static void editor_cancel(void){
 int wasMoving=E.moving;E.moving=0;E.strokeGroup=0;E.planErase=0;A.painting=A.erasing=0;ReleaseCapture();
 if(A.planPasteMode)plan_cancel_paste();if(A.blueprintCaptureMode)blueprint_capture_cancel();A.blueprintMode=0;A.selectedLib=-1;E.tool=EDIT_SELECT;
 set_status(wasMoving?tr(L"Deplacement / duplication annule. Le montage d'origine est intact."):tr(L"Action terminee. Outil Selection actif. Aucun element supprime."));InvalidateRect(A.hwnd,NULL,FALSE);
}
static void editor_palette_selected(void){editor_selection_clear();E.tool=EDIT_PLACE;A.blueprintMode=0;A.blueprintIndex=A.customBlueprintIndex=-1;A.rmp.editMode=0;A.viewMode=0;set_status(tr(L"Piece seule en main. Cliquez pour placer ; Assemblages pour voir ses montages connus."));}
static void editor_working_copy(void){if(A.map.cells&&!A.scene.active)open_export_wizard();}
static int editor_left(int mx,int my,WPARAM mods){
 if(!editor_canvas(mx,my))return 1;
 if(A.blueprintCaptureMode){blueprint_capture_click(mx,my);return 1;}
 if(E.moving){int x,y;if(mouse_to_tile(mx,my,&x,&y))editor_commit_transform(x,y,A.currentZ);return 1;}
 if(E.tool==EDIT_SELECT){editor_select_at(mx,my,(mods&MK_SHIFT)!=0);return 1;}
 if(E.tool==EDIT_PICK||(mods&MK_CONTROL)){BlueprintCapturePart p;if(hit_piece_at_levels(mx,my,&p.x,&p.y,&p.z,&p.layer,&p.lib,&p.local,A.currentZ,A.currentZ)){select_library_tile(p.lib,p.local);A.selectedLayer=p.layer;}return 1;}
 if(!editor_editable())return 1;A.lastPaintX=A.lastPaintY=-999;E.strokeGroup=++A.nextEditGroup;
 if(E.tool==EDIT_ERASE){BlueprintCapturePart p;if(hit_piece_at_levels(mx,my,&p.x,&p.y,&p.z,&p.layer,&p.lib,&p.local,A.currentZ,A.currentZ)&&editor_locked(p.x,p.y,p.z)){set_status(tr(L"Case verrouillee."));return 1;}A.currentEditGroup=E.strokeGroup;erase_visible_piece_at(mx,my);A.currentEditGroup=0;A.erasing=1;SetCapture(A.hwnd);return 1;}
 int x,y;if(!mouse_to_tile(mx,my,&x,&y))return 1;
 if(A.blueprintMode||A.brushMode==0){editor_stamp(x,y);A.lastPaintX=x;A.lastPaintY=y;if(!A.blueprintMode){A.painting=1;SetCapture(A.hwnd);}else E.strokeGroup=0;return 1;}
 edit_at(mx,my,0);if(A.brushMode!=3){A.painting=1;SetCapture(A.hwnd);}return 1;
}
static int editor_key(int key,int ctrl){
 if(key==VK_ESCAPE){editor_cancel();return 1;}if(plan_view_active()&&!ctrl&&key=='E'){E.planErase=!E.planErase;A.planPasteMode=0;InvalidateRect(A.hwnd,NULL,FALSE);return 1;}if(plan_view_active()||A.rmp.editMode)return 0;
 if(ctrl&&key=='D'){editor_begin_transform(2);return 1;}if(ctrl&&key=='C'){E.clipCount=E.count;memcpy(E.clip,E.parts,E.count*sizeof(E.parts[0]));set_status(tr(L"Selection copiee. Ctrl+V la place dans ce document."));return 1;}
 if(ctrl&&key=='V'){if(E.clipCount){E.count=E.clipCount;memcpy(E.parts,E.clip,E.count*sizeof(E.parts[0]));editor_begin_transform(2);}return 1;}
 if(key==VK_DELETE){editor_delete_selection();return 1;}if(!ctrl&&key=='V'){editor_tool(EDIT_SELECT);return 1;}if(!ctrl&&key=='B'){editor_tool(EDIT_PLACE);return 1;}if(!ctrl&&key=='E'){editor_tool(EDIT_ERASE);return 1;}if(!ctrl&&key=='I'){editor_tool(EDIT_PICK);return 1;}if(!ctrl&&key=='M'){editor_begin_transform(1);return 1;}if(key==VK_HOME){editor_frame();return 1;}return 0;
}
static int editor_opacity_for_z(int z){if(z>A.currentZ&&E.above==1)return E.opacity;if(z<A.currentZ&&E.below==2)return 55;return 100;}

static void editor_apply_opacity(unsigned*pixel){if(editorRenderOpacity<100)*pixel=(*pixel&0x00ffffffu)|((unsigned)(((*pixel>>24)&255)*editorRenderOpacity/100)<<24);}
static void editor_draw_overlay(void){
 if(A.viewMode||A.rmp.editMode)return;
 for(int i=0;i<E.count;i++){BlueprintCapturePart*p=&E.parts[i];if(normal_z_visible(p->z))blueprint_outline_cell(p->x,p->y,p->z,0xFFFFD166u);}
 if(!E.moving||!E.count||!A.hoverValid)return;int dx=A.hoverX-E.parts[0].x,dy=A.hoverY-E.parts[0].y,dz=A.currentZ-E.parts[0].z,valid=editor_validate(E.parts,E.count,dx,dy,dz,E.moving==1),ox,oy;world_origin(&ox,&oy);
 for(int i=0;i<E.count;i++){BlueprintCapturePart*p=&E.parts[i];int x=p->x+dx,y=p->y+dy,z=p->z+dz,sx,sy;if(x<0||y<0||z<0||z>=doc_zmax())continue;project_tile(x,y,z,&sx,&sy);int saved=editorRenderOpacity;editorRenderOpacity=65;draw_sprite_scaled(p->lib,p->local,mcd_frame(p->lib,p->local),ox+sx*A.zoom,oy+(sy-mcd_plevel(p->lib,p->local))*A.zoom,A.zoom);editorRenderOpacity=saved;blueprint_outline_cell(x,y,z,valid?0xFF45DF9Bu:0xFFFF6470u);}
}
static void editor_frame(void){
 int w=A.scene.active?A.scene.x:A.map.x,h=A.scene.active?A.scene.y:A.map.y,z=doc_zmax();if(!w||!h)return;
 int vw=A.clientW-A.sidebarW-A.inspectorW-40,vh=A.clientH-EDITOR_TOP-80;int zw=vw/((w+h)*16+32),zh=vh/((w+h)*8+z*24+40);A.zoom=clampi(zw<zh?zw:zh,1,8);
 A.panX=(h-w)*8*A.zoom;A.panY=EDITOR_TOP+30-116+(z-1)*24*A.zoom;InvalidateRect(A.hwnd,NULL,FALSE);
}
