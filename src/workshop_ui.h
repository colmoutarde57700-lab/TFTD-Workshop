/* Responsive normal-view controls; legacy PLAN/RMP tools retain their own workspace. */
static int editor_text(HDC dc,int x,int y,int width,const wchar_t*text,COLORREF color){RECT r={x,y,x+width,y+1500};SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);DrawTextW(dc,text,-1,&r,DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_NOPREFIX);DrawTextW(dc,text,-1,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);return r.bottom+8;}
static void editor_more(int x,int y){HMENU m=CreatePopupMenu();AppendMenuW(m,MF_STRING|(E.scope?MF_CHECKED:0),UI_SCOPE,tr(L"Selectionner les assemblages connus (Bible / Hangar)"));AppendMenuW(m,MF_STRING|(E.replace?MF_CHECKED:0),UI_REPLACE,tr(L"Autoriser le remplacement des pieces occupees"));AppendMenuW(m,MF_STRING,UI_CLEAR,tr(L"Deselectionner"));AppendMenuW(m,MF_STRING,UI_SAVE_ASSEMBLY,tr(L"Garder la selection dans le Hangar..."));AppendMenuW(m,MF_SEPARATOR,0,NULL);AppendMenuW(m,MF_STRING|(E.above?MF_CHECKED:0),UI_ABOVE,tr(L"Niveaux superieurs translucides (vue complete)"));AppendMenuW(m,MF_STRING|(E.below==2?MF_CHECKED:0),UI_BELOW,tr(L"Attenuer les niveaux inferieurs"));AppendMenuW(m,MF_STRING,UI_OPACITY,tr(L"Opacite des niveaux superieurs : changer 25/50/75 %"));AppendMenuW(m,MF_STRING,UI_FRAME,tr(L"Cadrer la carte   Debut"));AppendMenuW(m,MF_STRING,UI_HELP,tr(L"Gestes et limites de l'editeur"));int id=choose_popup(m,x,y);if(id)editor_action(id);}
static void editor_draw_header(HDC dc){
 if(plan_view_active()||A.rmp.editMode)return;int l=A.sidebarW+12,r=A.clientW-A.inspectorW-12,w=r-l,bw=(w-12)/4;
 fill_rect_color(dc,l-6,30,r+6,EDITOR_TOP,RGB(17,28,35));draw_level_bar(dc);
 const wchar_t*tools[]={tr(L"Selection [V]"),tr(L"Placer [B]"),tr(L"Gomme [E]"),tr(L"Pipette [I]")};const wchar_t*actions[]={tr(L"Deplacer [M]"),tr(L"Dupliquer"),tr(L"Supprimer"),tr(L"Options >")};
 for(int i=0;i<4;i++){int x=l+i*(bw+4);draw_button(dc,x,72,x+bw,100,tools[i],E.tool==i);draw_button(dc,x,106,x+bw,134,actions[i],(i==0&&E.moving==1)||(i==1&&E.moving==2)||(i==3&&(E.scope||E.replace)));}
 int saved=SaveDC(dc);SetViewportOrgEx(dc,0,34,NULL);IntersectClipRect(dc,l-6,106,r+6,134);draw_blueprint_toolbar(dc);RestoreDC(dc,saved);
 saved=SaveDC(dc);SetViewportOrgEx(dc,0,102,NULL);draw_layer_bar(dc);RestoreDC(dc,saved);
}
static int editor_header_click(int x,int y){
 if(plan_view_active()||A.rmp.editMode)return 0;int l=A.sidebarW+12,r=A.clientW-A.inspectorW-12,w=r-l,bw=(w-12)/4;
 if(y<EDITOR_TOP){
  if(level_bar_click(x,y))return 1;
  if(x>=l&&x<r&&((y>=72&&y<100)||(y>=106&&y<134))){int i=(x-l)/(bw+4);if(i>3)return 1;if(y<100)editor_tool(i);else if(i==0)editor_begin_transform(1);else if(i==1)editor_begin_transform(2);else if(i==2)editor_delete_selection();else editor_more(x,134);return 1;}
  if(y>=140&&y<168){int hit=blueprint_toolbar_click(x,y-34);if(hit&&(A.selectedLib>=0||A.blueprintMode))E.tool=EDIT_PLACE;return 1;}
  if(y>=174&&y<202){layer_bar_click(x,y-102);return 1;}return 1;
 }return 0;
}
static void editor_plan_scroll(int dx,int dy){E.planPanelX=clampi(E.planPanelX+dx,0,max(0,350-A.inspectorW));E.planPanelY=clampi(E.planPanelY+dy,0,max(0,940-(A.clientH-40)));InvalidateRect(A.hwnd,NULL,FALSE);}
static void editor_draw_plan_inspector(HDC dc){
 int oldW=A.clientW,oldI=A.inspectorW,l=oldW-oldI;E.planPanelX=clampi(E.planPanelX,0,max(0,350-oldI));E.planPanelY=clampi(E.planPanelY,0,max(0,940-(A.clientH-40)));fill_rect_color(dc,l,0,oldW,A.clientH,RGB(16,22,27));int saved=SaveDC(dc);IntersectClipRect(dc,l,0,oldW,A.clientH-40);SetViewportOrgEx(dc,-E.planPanelX,-E.planPanelY,NULL);A.inspectorW=max(350,oldI);A.clientW=l+A.inspectorW;draw_inspector(dc);A.inspectorW=oldI;A.clientW=oldW;RestoreDC(dc,saved);
 int bw=(oldI-20)/4;const wchar_t*labels[]={L"<",L">",tr(L"Haut"),tr(L"Bas")};for(int i=0;i<4;i++)draw_button(dc,l+8+i*(bw+1),A.clientH-34,l+8+i*(bw+1)+bw,A.clientH-6,labels[i],0);
}
static void editor_plan_inspector_click(int x,int y){
 int oldW=A.clientW,oldI=A.inspectorW,l=oldW-oldI;if(y>=A.clientH-40){int i=(x-l-8)/((oldI-20)/4+1);if(i==0)editor_plan_scroll(-80,0);else if(i==1)editor_plan_scroll(80,0);else if(i==2)editor_plan_scroll(0,-120);else if(i==3)editor_plan_scroll(0,120);return;}
 A.inspectorW=max(350,oldI);A.clientW=l+A.inspectorW;plan_panel_click(x+E.planPanelX,y+E.planPanelY);A.inspectorW=oldI;A.clientW=oldW;
}
static void editor_draw_inspector(HDC dc){
 int l=A.clientW-A.inspectorW,w=A.inspectorW-28;
 if(plan_view_active()){editor_draw_plan_inspector(dc);return;}if(A.rmp.editMode){draw_inspector(dc);return;}
 if(E.details){draw_inspector(dc);draw_button(dc,l+12,8,A.clientW-12,34,tr(L"< Retour aux actions"),1);return;}
 fill_rect_color(dc,l,0,A.clientW,A.clientH,RGB(16,24,30));int y=14;wchar_t text[700];
 const wchar_t*tn[]={tr(L"Selection"),tr(L"Placement"),tr(L"Gomme"),tr(L"Pipette")};_snwprintf(text,699,L"%ls%ls",E.moving?tr(L"Destination du montage"):tn[E.tool],E.replace?tr(L"  / remplacement autorise"):L"");y=editor_text(dc,l+14,y,w,text,RGB(90,220,216));
 draw_button(dc,l+12,54,A.clientW-12,82,tr(L"Details techniques >"),0);
 draw_button(dc,l+12,88,A.clientW-12,116,tr(L"Vue : pieces / plan 2D / plan ISO"),0);
 draw_button(dc,l+12,122,A.clientW-12,150,current_map_is_protected()?tr(L"Creer une copie editable..."):tr(L"Exporter vers un mod..."),0);int saved=SaveDC(dc);IntersectClipRect(dc,l,158,A.clientW,A.clientH-30);y=164-E.inspectorScroll;
 if(E.count){_snwprintf(text,699,tr(L"%d piece(s) selectionnee(s)\n%ls\nSelection : %ls"),E.count,E.selectionName,E.scope?tr(L"assemblages connus"):tr(L"pieces individuelles"));y=editor_text(dc,l+14,y,w,text,RGB(240,213,138));}
 else y=editor_text(dc,l+14,y,w,E.tool==EDIT_PLACE?tr(L"Choisissez une piece a gauche. Les montages disponibles sont dans Assemblages."):tr(L"Cliquez une piece sur la carte. Maj+clic ajoute ou retire une piece. La selection porte sur le Z actif."),RGB(187,203,213));
 BlueprintCapturePart part;int has=0;
 if(E.count){part=E.parts[0];has=1;}else if(A.selectedLib>=0){part=(BlueprintCapturePart){.lib=A.selectedLib,.local=A.selectedLocal,.layer=A.selectedLayer};has=1;}
 if(has){int lib=part.lib,local=part.local;_snwprintf(text,699,tr(L"%ls / MCD %d\nFrame %d | %ls\nType declare : %ls\nCouche %ls : %ls"),A.library[lib].name,local,mcd_frame(lib,local),source_name(A.library[lib].source),layer_name(mcd_u8(lib,local,53)),E.count?tr(L"occupee"):tr(L"de pose"),layer_name(part.layer));y=editor_text(dc,l+14,y,w,text,RGB(215,229,233));
  const wchar_t*role=editor_role(lib,local);if(role){_snwprintf(text,699,tr(L"Role visuel signale : %ls. Le slot moteur est conserve. Voir Bible RC16."),role);y=editor_text(dc,l+14,y,w,text,RGB(239,193,111));}
  int bw=mcd_bigwall_effective(lib,local);if(bw){_snwprintf(text,699,tr(L"BigWall effectif %d (brut %d). Verifier collision, diagonale et occupation ; l'image seule ne suffit pas."),bw,mcd_u8(lib,local,33));y=editor_text(dc,l+14,y,w,text,RGB(239,193,111));}
  if(E.count&&part.layer==3){BlueprintCapturePart support;if(!editor_read(part.x,part.y,part.z,0,&support))y=editor_text(dc,l+14,y,w,tr(L"Pas de FLOOR sur cette case. Verifier le support inferieur et le role du vide avant toute correction."),RGB(239,193,111));}
  if(!_wcsicmp(A.library[lib].name,L"ATLANTIS")&&local>=31&&local<=33)y=editor_text(dc,l+14,y,w,tr(L"Attention aux versions : les pieces 31-33 ont ete redefinies dans Gabarits Universels. Un import original demande un controle."),RGB(239,193,111));
 }
 y=editor_text(dc,l+14,y,w,tr(L"Clic droit / Echap : terminer l'action.\nMolette : Z | Ctrl+molette : zoom\nBouton milieu : deplacer la vue\nCtrl+Z : annuler l'operation"),RGB(141,169,181));E.inspectorExtent=max(0,y+E.inspectorScroll-(A.clientH-35));RestoreDC(dc,saved);
}
static int editor_inspector_click(int x,int y){if(plan_view_active()){editor_plan_inspector_click(x,y);return 1;}if(A.rmp.editMode){inspector_level_click(x,y);return 1;}if(E.details){if(y<38)E.details=0;else inspector_level_click(x,y);InvalidateRect(A.hwnd,NULL,FALSE);return 1;}if(y>=54&&y<82)E.details=1;else if(y>=88&&y<116)handle_command(IDM_VIEW_PLAN);else if(y>=122&&y<150)editor_working_copy();InvalidateRect(A.hwnd,NULL,FALSE);return 1;}
static int editor_action(int id){
 if(id<UI_SELECT||id>=UI_END)return 0;
 switch(id){case UI_SELECT:case UI_PLACE:case UI_ERASE:case UI_PICK:editor_tool(id-UI_SELECT);break;
 case UI_MOVE:editor_begin_transform(1);break;case UI_DUPLICATE:editor_begin_transform(2);break;case UI_DELETE:editor_delete_selection();break;
 case UI_CLEAR:editor_selection_clear();break;case UI_SCOPE:E.scope=!E.scope;set_status(E.scope?tr(L"Selection : assemblages complets reconnus par la Bible ou le Hangar."):tr(L"Selection : pieces individuelles."));break;
 case UI_REPLACE:E.replace=!E.replace;set_status(E.replace?tr(L"Remplacement autorise explicitement. Ctrl+Z restaure les pieces remplacees."):tr(L"Emplacements occupes proteges."));break;
 case UI_ABOVE:E.above=!E.above;if(E.above)set_normal_visibility(2);break;case UI_BELOW:E.below=E.below==2?1:2;break;case UI_OPACITY:E.opacity=E.opacity==75?25:E.opacity+25;E.above=1;set_normal_visibility(2);break;
 case UI_DETAILS:E.details=!E.details;break;case UI_WORKING_COPY:editor_working_copy();break;case UI_FRAME:editor_frame();break;case UI_ASSEMBLIES:open_blueprint_hangar();break;
 case UI_SAVE_ASSEMBLY:if(E.count>1){A.blueprintCaptureMode=1;A.blueprintCaptureCount=E.count<MAX_BLUEPRINT_CAPTURE?E.count:MAX_BLUEPRINT_CAPTURE;memcpy(A.blueprintCapture,E.parts,A.blueprintCaptureCount*sizeof(E.parts[0]));open_blueprint_hangar();blueprint_hangar_refresh();set_status(tr(L"Selection reprise dans le Hangar. Nommez-la puis Enregistrer."));}else set_status(tr(L"Selectionnez au moins deux pieces pour garder un assemblage."));break;
 case UI_HELP:MessageBoxW(A.hwnd,tr(L"SELECTION : clic sur une piece du niveau actif. Maj+clic pour plusieurs pieces.\nDEPLACER / DUPLIQUER : cliquez une destination ; la molette change le Z. Les emplacements occupes sont proteges par defaut. Options permet de remplacer explicitement.\nCLIC DROIT / ECHAP : lacher ou annuler, sans supprimer. La gomme et Suppr sont explicites.\nASSEMBLAGES : Bible et Hangar ; reconnaissance complete, pas de liaison devinee.\nOPTIONS : selection par assemblage, niveaux translucides, opacite et cadrage.\nPLAN : annotations et outils existants ; un projet n'ajoute pas de nouvelles regles au moteur.\nLes alertes FLOOR / BigWall sont des aides a l'audit, pas des corrections automatiques."),tr(L"Gestes du Workshop"),MB_OK);break;
 default:break;}
 InvalidateRect(A.hwnd,NULL,FALSE);return 1;
}
