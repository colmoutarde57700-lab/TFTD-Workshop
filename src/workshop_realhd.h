static const int RH_PROFILES[22][4] =
{
	{  0, -8,  0,  0 }, // profile 00
	{  0, -8, -8,  0 }, // profile 01
	{  0,  0, -8,  0 }, // profile 02
	{  0,  0, -8, -8 }, // profile 03
	{  0,  0,  0, -8 }, // profile 04
	{ -8, -8,  0,  0 }, // profile 05
	{ -8,  0,  0,  0 }, // profile 06
	{ -8,  0,  0, -8 }, // profile 07
	{ -8, -8, -8, -8 }, // profile 08
	{-16,-16,-16,-16 }, // profile 09
	{-24,-24,-24,-24 }, // profile 10
	{ -8, -8, -8,  0 }, // profile 11
	{ -8,  0, -8, -8 }, // profile 12
	{ -8, -8,  0, -8 }, // profile 13
	{  0, -8, -8, -8 }, // profile 14
	{  0,  0,  0,  0 }, // profile 15
	{  0,  0,  0,  0 }, // profile 16
	{  0,  0,  0,  0 }, // profile 17
	{  0,  0,  0,  0 }, // profile 18
	{ -8, -8, -8, -8 }, // profile 19
	{  0,  0,  0,  0 }, // profile 20
	{  0,  0,  0,  0 }  // profile 21
};
/* CPU Workshop preview of P2ZJ BedrockRender / Map.cpp geometry.
   Geometry is logical MCD-based. No shaders, parallax, fog or gameplay simulation. */
typedef struct {int valid,lib,local,layer;float h[4];} RhCell;
typedef struct {float x,y,u,v;} RhVertex;
typedef struct {uint32_t*p;int w,h,tried;} RhTexture;
static RhTexture rhTextures[2];
static int rhTextureMode=-1;
static void rh_clear(void){for(int i=0;i<2;i++){free(rhTextures[i].p);ZeroMemory(&rhTextures[i],sizeof(RhTexture));}rhTextureMode=-1;}
static int rh_profile(int lib,int local,float out[4]){
    if(lib<0||lib>=A.libraryCount||!library_load(lib)||local<0||local>=A.library[lib].mcdCount)return 0;
    static const int map[20]={0,1,2,3,4,5,6,7,8,11,12,13,14,15,16,17,18,19,20,21};
    int profile=-1;if(!_wcsicmp(A.library[lib].name,L"SAND")&&local<20)profile=map[local];
    else if(!_wcsicmp(A.library[lib].name,L"DEBRIS")&&local>=32&&local<=49)profile=local==32?9:local==33?10:(local-34)&7;
    if(profile<0)return 0;for(int i=0;i<4;i++)out[i]=(float)(RH_PROFILES[profile][i]-mcd_plevel(lib,local));return 1;
}
static RhCell rh_cell(int x,int y,int z){
    RhCell r={0};for(int k=0;k<2;k++){int part=k?0:3,li=-1,lo=-1;if(A.scene.active){SceneCell*c=scene_cell_at(x,y,z);if(c){li=c->lib[part];lo=c->local[part];}}else{MapCell*c=cell_at(x,y,z);if(c)active_resolve_raw(c->part[part],&li,&lo);}if(rh_profile(li,lo,r.h)){r.valid=1;r.lib=li;r.local=lo;r.layer=part;return r;}}return r;
}
/* Same shared-corner proposal/hard-break rule as P2ZJ. Adjacent discontinuities
   preserve authored heights; diagonal-only contact can share an average. */
static float rh_corner(int gx,int gy,int z,float fallback){
    RhCell c[4]={rh_cell(gx-1,gy-1,z),rh_cell(gx,gy-1,z),rh_cell(gx-1,gy,z),rh_cell(gx,gy,z)};
    const int corner[4]={2,3,1,0};float sum=0;int count=0,hard=0;
    for(int i=0;i<4;i++)if(c[i].valid){sum+=c[i].h[corner[i]];count++;}
    if(c[0].valid&&c[1].valid&&(c[0].h[1]!=c[1].h[0]||c[0].h[2]!=c[1].h[3]))hard=1;
    if(c[2].valid&&c[3].valid&&(c[2].h[1]!=c[3].h[0]||c[2].h[2]!=c[3].h[3]))hard=1;
    if(c[0].valid&&c[2].valid&&(c[0].h[3]!=c[2].h[0]||c[0].h[2]!=c[2].h[1]))hard=1;
    if(c[1].valid&&c[3].valid&&(c[1].h[3]!=c[3].h[0]||c[1].h[2]!=c[3].h[1]))hard=1;
    return hard||!count?fallback:sum/count;
}
static RhTexture*rh_texture(int side){
    if(rhTextureMode!=A.assetRenderMode){rh_clear();rhTextureMode=A.assetRenderMode;}RhTexture*t=&rhTextures[side];if(t->tried)return t;t->tried=1;
    wchar_t path[PATH_CAP];_snwprintf(path,PATH_CAP-1,L"%ls\\%ls\\Resources\\TFTD_HD\\RealHD\\Datasets\\SAND\\Materials\\%ls_BASE.png",A.modsRoot,A.assetRenderMode==4?L"TFTD_REAL_HD_DEBUG":L"TFTD_REAL_HD_TEXTURES",side?L"VERTICAL":L"TOP");
    GpBitmap*b=NULL;if(GdipCreateBitmapFromFile(path,&b)!=Ok||!b){set_status(tr(L"Materiau REAL HD introuvable : damier magenta. Verifiez le dossier MODS et les fichiers TOP_BASE / VERTICAL_BASE."));return t;}UINT w=0,h=0;GdipGetImageWidth((GpImage*)b,&w);GdipGetImageHeight((GpImage*)b,&h);if(!w||!h||w>8192||h>8192){GdipDisposeImage((GpImage*)b);return t;}
    GpRect r={0,0,(INT)w,(INT)h};BitmapData d={0};if(GdipBitmapLockBits(b,&r,ImageLockModeRead,PixelFormat32bppARGB,&d)==Ok){t->p=(uint32_t*)malloc((size_t)w*h*4);if(t->p){t->w=(int)w;t->h=(int)h;for(UINT y=0;y<h;y++)memcpy(t->p+(size_t)y*w,(BYTE*)d.Scan0+(ptrdiff_t)y*d.Stride,(size_t)w*4);}GdipBitmapUnlockBits(b,&d);}GdipDisposeImage((GpImage*)b);return t;
}
static int rh_bary(RhVertex a,RhVertex b,RhVertex c,float x,float y,float*w0,float*w1,float*w2){
    float den=(b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y);if(fabsf(den)<0.0001f)return 0;
    *w0=((b.y-c.y)*(x-c.x)+(c.x-b.x)*(y-c.y))/den;*w1=((c.y-a.y)*(x-c.x)+(a.x-c.x)*(y-c.y))/den;*w2=1-*w0-*w1;return *w0>=-0.00001f&&*w1>=-0.00001f&&*w2>=-0.00001f;
}
static void rh_triangle(RhVertex a,RhVertex b,RhVertex c,int side){
    int x0=max(0,(int)floorf(fminf(a.x,fminf(b.x,c.x)))),y0=max(0,(int)floorf(fminf(a.y,fminf(b.y,c.y))));
    int x1=min(A.backW-1,(int)ceilf(fmaxf(a.x,fmaxf(b.x,c.x)))),y1=min(A.backH-1,(int)ceilf(fmaxf(a.y,fmaxf(b.y,c.y))));if(x0>x1||y0>y1)return;
    RhTexture*t=rh_texture(side);for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){float w0,w1,w2;if(!rh_bary(a,b,c,x+.5f,y+.5f,&w0,&w1,&w2))continue;float u=w0*a.u+w1*b.u+w2*c.u,v=w0*a.v+w1*b.v+w2*c.v;uint32_t col;
        if(t->p){u-=floorf(u);v-=floorf(v);int tx=min(t->w-1,(int)(u*t->w)),ty=min(t->h-1,(int)(v*t->h));col=t->p[(size_t)ty*t->w+tx]|0xff000000u;}else col=((x/8+y/8)&1)?0xffff00ffu:0xff35213cu;
        editor_apply_opacity(&col);size_t i=(size_t)y*A.backW+x;A.backbuf[i]=alpha_over(A.backbuf[i],col);
    }
}
static void rh_vertices(int x,int y,int z,int sx,int sy,RhCell g,RhVertex v[4]){
    float s=(float)A.zoom;float hh[4]={rh_corner(x,y,z,g.h[0]),rh_corner(x+1,y,z,g.h[1]),rh_corner(x+1,y+1,z,g.h[2]),rh_corner(x,y+1,z,g.h[3])};
    v[0]=(RhVertex){sx+16*s,sy+(24+hh[0])*s,x/8.f,y/8.f};v[1]=(RhVertex){sx+32*s,sy+(32+hh[1])*s,(x+1)/8.f,y/8.f};v[2]=(RhVertex){sx+16*s,sy+(40+hh[2])*s,(x+1)/8.f,(y+1)/8.f};v[3]=(RhVertex){(float)sx,sy+(32+hh[3])*s,x/8.f,(y+1)/8.f};
}
static void rh_edge(float x0,float y0,float x1,float y1,float a,float b,float c,float d,float u0,float u1){
    if(a==b&&c==d)return;float s=(float)A.zoom;RhVertex v[4]={{x0,y0+fminf(a,b)*s,u0,-fminf(a,b)/16.f},{x1,y1+fminf(c,d)*s,u1,-fminf(c,d)/16.f},{x0,y0+fmaxf(a,b)*s,u0,-fmaxf(a,b)/16.f},{x1,y1+fmaxf(c,d)*s,u1,-fmaxf(c,d)/16.f}};
    rh_triangle(v[0],v[1],v[2],1);rh_triangle(v[1],v[3],v[2],1);
}
static int rh_draw_part(int x,int y,int z,int part,int lib,int local,int sx,int sy){
    float h[4];if(A.assetRenderMode<3||(part!=0&&part!=3)||!rh_profile(lib,local,h))return 0;RhCell g=rh_cell(x,y,z);if(g.layer!=part)return 1;
    RhVertex v[4];rh_vertices(x,y,z,sx,sy,g,v);rh_triangle(v[0],v[1],v[3],0);rh_triangle(v[1],v[2],v[3],0);
    RhCell east=rh_cell(x+1,y,z),south=rh_cell(x,y+1,z);float s=(float)A.zoom;
    if(east.valid)rh_edge(sx+32*s,sy+32*s,sx+16*s,sy+40*s,g.h[1],east.h[0],g.h[2],east.h[3],y/8.f,(y+1)/8.f);
    if(south.valid)rh_edge(sx,sy+32*s,sx+16*s,sy+40*s,g.h[3],south.h[0],g.h[2],south.h[1],x/8.f,(x+1)/8.f);
    if(A.assetRenderMode==4){for(int i=0;i<4;i++)line_px((int)v[i].x,(int)v[i].y,(int)v[(i+1)%4].x,(int)v[(i+1)%4].y,0xff45ddddu);line_px((int)v[1].x,(int)v[1].y,(int)v[3].x,(int)v[3].y,0xff498baau);}return 1;
}
static int rh_hit(int x,int y,int z,int part,int lib,int local,int sx,int sy,int mx,int my){
    float h[4];if(A.assetRenderMode<3||(part!=0&&part!=3)||!rh_profile(lib,local,h))return -1;RhCell g=rh_cell(x,y,z);if(g.layer!=part)return 0;RhVertex v[4];rh_vertices(x,y,z,sx,sy,g,v);float a,b,c;return rh_bary(v[0],v[1],v[3],mx+.5f,my+.5f,&a,&b,&c)||rh_bary(v[1],v[2],v[3],mx+.5f,my+.5f,&a,&b,&c);
}
