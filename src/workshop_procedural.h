/* Procedural SAND laboratory v3: shared-height terrain and historical ecology.
   No new artwork, rotations, or gameplay/pathfinding claims. */
#define IDM_PROCEDURAL 1200
#define PROC_MAX_MOTIFS 128
#define PROC_MAX_PARTS 8192
#define PROC_GENERATE 7101
#define PROC_NEW_SEED 7102
#define PROC_SAVE 7103
#define PROC_CLOSE 7104

typedef struct { int w,h,z,count,relief; BlueprintCapturePart *parts; wchar_t name[96]; } ProcMotif;
typedef struct { int width,height,density,relief,gap,craft,uso; uint32_t seed; } ProcRecipe;
typedef struct { int motifs,placed,reliefs,open,total,skipped,weeds,rocks,wrecks,historical,lowReliefs,upperReliefs,upperDecor; } ProcReport;
/* Allowed ROCKS units. Original MAP evidence: SEABED02 single 0/1;
   SEABED04 & 08: [9,8;7,10]; SEABED12: [5,4;3,6], [9,8;7,6].
   Never infer autonomous pieces from sprite appearance. */
typedef struct {int w,h,count,ids[4];const char*source;} ProcRockUnit;
static const ProcRockUnit procRockUnits[]={
 {1,1,1,{0},"SEABED02"},{1,1,1,{1},"SEABED02"},
 {2,2,4,{9,8,7,10},"SEABED04/08"},
 {2,2,4,{5,4,3,6},"SEABED12"},
 {2,2,4,{9,8,7,6},"SEABED12"}
};
static ProcMotif procMotifs[PROC_MAX_MOTIFS];
static int procMotifCount,procSand=-1;
static wchar_t procError[512];
static struct { HWND hwnd,width,height,seed,density,relief,gap,craft,uso,report; } PUI;

static void proc_catalog_clear(void){for(int i=0;i<procMotifCount;i++)free(procMotifs[i].parts);procMotifCount=0;procSand=-1;}
static int proc_flat(int lib,int local){return lib==procSand&&(local==13||local==14||local==15||local==16||local==18||local==19);}
static int proc_add_motif(BlueprintCapturePart*p,int n,int w,int h,int z,const wchar_t*name){
    if(!n||procMotifCount>=PROC_MAX_MOTIFS)return 0;
    /* Deduplicate identical observed geometry, independent of source MAP name. */
    for(int i=0;i<procMotifCount;i++){ProcMotif*m=&procMotifs[i];if(m->w==w&&m->h==h&&m->z==z&&m->count==n&&!memcmp(m->parts,p,(size_t)n*sizeof(*p)))return 1;}
    ProcMotif*m=&procMotifs[procMotifCount];m->parts=(BlueprintCapturePart*)malloc((size_t)n*sizeof(*p));if(!m->parts)return 0;
    memcpy(m->parts,p,(size_t)n*sizeof(*p));for(int j=0;j<n;j++)if(m->parts[j].lib==procSand){if(m->parts[j].local==16)m->parts[j].local=13;else if(m->parts[j].local==17)m->parts[j].local=8;}m->count=n;m->w=w;m->h=h;m->z=1;m->relief=0;wcsncpy(m->name,name,95);
    for(int i=0;i<n;i++){if(p[i].z+1>m->z)m->z=p[i].z+1;if(p[i].z>0||(p[i].lib==procSand&&!proc_flat(p[i].lib,p[i].local)))m->relief=1;}
    procMotifCount++;return 1;
}
static int proc_read_map(int mi,SceneDoc*out){
    ZeroMemory(out,sizeof(*out));if(mi<0||mi>=A.mapCount)return 0;MapEntry*m=&A.maps[mi];int pi=m->profileIndex;
    if(pi<0||pi>=A.profileCount)return 0;for(int k=0;k<A.profiles[pi].dataSetCount;k++){int li=library_find_name_ctx(A.profiles[pi].dataSets[k],m->source,m->origin);if(li<0||!library_load(li))return 0;}DWORD size=0;uint8_t*b=read_all(m->path,&size);if(!b||size<3){free(b);return 0;}
    int h=b[0],w=b[1],z=b[2];size_t n=(size_t)w*h*z;if(!w||!h||!z||z>32||3+n*4>size){free(b);return 0;}
    SceneCell*c=(SceneCell*)malloc(n*sizeof(*c));if(!c){free(b);return 0;}memset(c,0xff,n*sizeof(*c));
    int ok=1;size_t off=3;
    for(int fz=0;fz<z;fz++)for(int y=0;y<h;y++)for(int x=0;x<w;x++)for(int p=0;p<4;p++){
        int raw=b[off++];if(!raw)continue;int li=-1,lo=-1;
        if(!profile_resolve_raw(pi,raw,m->source,m->origin,&li,&lo)||!library_load(li)||lo<0||lo>=A.library[li].mcdCount){ok=0;continue;}
        SceneCell*d=&c[((z-1-fz)*h+y)*w+x];d->lib[p]=li;d->local[p]=lo;
    }
    free(b);if(!ok){free(c);return 0;}out->cells=c;out->x=w;out->y=h;out->z=z;return 1;
}
static void proc_extract(int mi){
    SceneDoc s;if(!proc_read_map(mi,&s))return;int n=s.x*s.y;
    uint8_t*marked=(uint8_t*)calloc((size_t)n,1);int*q=(int*)malloc((size_t)n*sizeof(int));
    BlueprintCapturePart*parts=(BlueprintCapturePart*)calloc(PROC_MAX_PARTS,sizeof(*parts));
    if(!marked||!q||!parts){free(marked);free(q);free(parts);free(s.cells);return;}
    for(int i=0;i<n;i++)for(int z=0;z<s.z;z++)for(int p=0;p<4;p++){
        SceneCell*c=&s.cells[z*n+i];if(c->lib[p]>=0&&!(z==0&&p==0&&proc_flat(c->lib[p],c->local[p])))marked[i]=1;
    }
    for(int start=0;start<n;start++)if(marked[start]==1){
        int head=0,tail=1,minx=s.x,miny=s.y,maxx=0,maxy=0;q[0]=start;marked[start]=2;
        while(head<tail){int i=q[head++],x=i%s.x,y=i/s.x;if(x<minx)minx=x;if(y<miny)miny=y;if(x>maxx)maxx=x;if(y>maxy)maxy=y;
            for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){int xx=x+dx,yy=y+dy;if(xx<0||yy<0||xx>=s.x||yy>=s.y)continue;int j=yy*s.x+xx;if(marked[j]==1){marked[j]=2;q[tail++]=j;}}
        }
        /* Boundary-touching components may continue in another block: reject. */
        if(minx==0||miny==0||maxx==s.x-1||maxy==s.y-1)continue;
        int count=0,overflow=0;for(int z=0;z<s.z;z++)for(int y=miny;y<=maxy;y++)for(int x=minx;x<=maxx;x++){
            /* Copy only this component, retaining every source layer and Z. */
            int belongs=0;for(int k=0;k<tail;k++)if(q[k]==y*s.x+x){belongs=1;break;}if(!belongs)continue;
            SceneCell*c=&s.cells[(z*s.y+y)*s.x+x];for(int p=0;p<4;p++)if(c->lib[p]>=0){if(count==PROC_MAX_PARTS){overflow=1;break;}BlueprintCapturePart*d=&parts[count++];*d=(BlueprintCapturePart){.x=x-minx,.y=y-miny,.z=z,.layer=p,.lib=c->lib[p],.local=c->local[p]};}
        }
        if(!overflow)proc_add_motif(parts,count,maxx-minx+1,maxy-miny+1,s.z,A.maps[mi].name);
    }
    free(parts);free(q);free(marked);free(s.cells);
}
static int proc_catalog_build(void){
    proc_catalog_clear();procError[0]=0;procSand=library_find_name_ctx(L"SAND",SRC_TFTD,L"TFTD ORIGINAL");
    if(procSand<0||A.library[procSand].source!=SRC_TFTD||!library_load(procSand)||A.library[procSand].mcdCount!=20){wcscpy(procError,tr(L"SAND original introuvable. Configurez le dossier TFTD puis reindexez les ressources."));return 0;}
    /* This first adapter is explicitly for the original SAND MCD family. */
    for(int i=0;i<A.mapCount;i++)if(A.maps[i].source==SRC_TFTD&&wcsncmp(A.maps[i].name,L"SEABED",6)==0)proc_extract(i);
    BlueprintCapturePart p[32];
    for(int i=0;i<BLUEPRINT_RC15_COUNT;i++){const BlueprintDef*b=&BLUEPRINTS_RC15[i];if(wcsncmp(b->id,L"DEBRIS_STRUCTURAL_",18)!=0||b->partCount>32)continue;int ok=1;
        for(int j=0;j<b->partCount;j++){const BlueprintPartDef*d=&BLUEPRINT_PARTS_RC15[b->firstPart+j];int li=library_find_name_ctx(d->dataset,SRC_TFTD,L"TFTD ORIGINAL");if(li<0||!library_load(li)||d->mcd>=A.library[li].mcdCount){ok=0;break;}p[j]=(BlueprintCapturePart){.x=d->dx,.y=d->dy,.z=d->dz,.layer=d->layer,.lib=li,.local=d->mcd};}
        if(ok)proc_add_motif(p,b->partCount,b->spanX,b->spanY,b->spanZ,b->id);
    }
    if(!procMotifCount){wcscpy(procError,tr(L"Aucun assemblage SAND exploitable. Verifiez les MAP SEABED et les profils OXCE."));return 0;}return 1;
}
static uint32_t proc_random(uint32_t*s){/* xorshift32 avoids LCG low-bit stripes */uint32_t x=*s;x^=x<<13;x^=x>>17;x^=x<<5;return *s=x;}
static int proc_space(const uint8_t*busy,int w,int h,int x,int y,int rw,int rh,int gap){
    if(x<gap||y<gap||x+rw>w-gap||y+rh>h-gap)return 0;
    for(int yy=y-gap;yy<y+rh+gap;yy++)for(int xx=x-gap;xx<x+rw+gap;xx++)if(busy[yy*w+xx])return 0;return 1;
}
static void proc_mark(uint8_t*busy,int w,int x,int y,int rw,int rh){for(int yy=y;yy<y+rh;yy++)for(int xx=x;xx<x+rw;xx++)busy[yy*w+xx]=1;}
static int proc_connected(const uint8_t*busy,int w,int h,int*open){
    int n=w*h,start=-1,total=0;for(int i=0;i<n;i++)if(!busy[i]){total++;if(start<0)start=i;}*open=total;if(!total)return 0;
    uint8_t*seen=(uint8_t*)calloc((size_t)n,1);int*q=(int*)malloc((size_t)n*sizeof(int));if(!seen||!q){free(seen);free(q);return 0;}
    int head=0,tail=1;q[0]=start;seen[start]=1;while(head<tail){int i=q[head++],x=i%w,y=i/w;int nb[4]={x?i-1:-1,x<w-1?i+1:-1,y?i-w:-1,y<h-1?i+w:-1};for(int k=0;k<4;k++){int j=nb[k];if(j>=0&&!busy[j]&&!seen[j]){seen[j]=1;q[tail++]=j;}}}
    free(q);free(seen);return tail==total;
}
static int proc_stamp_ship(SceneDoc*s,const SceneDoc*ship,int x,int y){if(x<0||y<0||x+ship->x>s->x||y+ship->y>s->y||ship->z>s->z)return 0;
    for(int z=0;z<ship->z;z++)for(int yy=0;yy<ship->y;yy++)for(int xx=0;xx<ship->x;xx++){SceneCell*a=&ship->cells[(z*ship->y+yy)*ship->x+xx],*b=&s->cells[(z*s->y+y+yy)*s->x+x+xx];for(int p=0;p<4;p++)if(a->lib[p]>=0){b->lib[p]=a->lib[p];b->local[p]=a->local[p];}}return 1;
}

/* Assemble logical SAND/DEBRIS pieces from a shared height lattice. Heights are
   eighth-pixel steps (8 engine units), not inferred from any sprite. Every
   perimeter vertex is ground level; adjacent cells share the same four values. */
static int proc_hill_height(int x,int y,int w,int h,int peak){int d=min(min(x,w-x),min(y,h-y));return min(d,peak);}
static int __attribute__((unused)) proc_hill_parts(int w,int h,int peak,int debris,BlueprintCapturePart*p,int*count){
    *count=0;for(int y=0;y<h;y++)for(int x=0;x<w;x++){
        int heights[4]={proc_hill_height(x,y,w,h,peak),proc_hill_height(x+1,y,w,h,peak),proc_hill_height(x+1,y+1,w,h,peak),proc_hill_height(x,y+1,w,h,peak)};
        int base=min(min(heights[0],heights[1]),min(heights[2],heights[3])),z=base/3,li=-1,lo=-1,layer=3;
        if(heights[0]==base&&heights[1]==base&&heights[2]==base&&heights[3]==base&&base%3==0){li=procSand;lo=13;layer=0;}
        else for(int family=0;family<2&&li<0;family++){int lib=family?debris:procSand;int first=family?32:0,last=family?49:19;for(int local=first;local<=last;local++){
            if((!family&&(local==16||local==17))||mcd_u8(lib,local,53)!=3)continue;float hh[4];if(!rh_profile(lib,local,hh))continue;int ok=1;for(int k=0;k<4;k++)if((float)(z*24)-hh[k]!=(float)(heights[k]*8))ok=0;if(ok){li=lib;lo=local;break;}
        }}
        if(li<0)return 0;
        for(int zz=0;zz<z;zz++){if(*count+2>=PROC_MAX_PARTS)return 0;p[(*count)++]=(BlueprintCapturePart){.x=x,.y=y,.z=zz,.layer=0,.lib=procSand,.local=13};p[(*count)++]=(BlueprintCapturePart){.x=x,.y=y,.z=zz,.layer=3,.lib=debris,.local=33};}
        if(*count+2>=PROC_MAX_PARTS)return 0;
        p[(*count)++]=(BlueprintCapturePart){.x=x,.y=y,.z=z,.layer=0,.lib=procSand,.local=13};
        if(layer==3)p[(*count)++]=(BlueprintCapturePart){.x=x,.y=y,.z=z,.layer=3,.lib=li,.local=lo};
    }
    return 1;
}
/* Keep original terrain compositions intact but do not import their wrecks,
   vegetation or UFOBITS effects as if they were terrain or upper floors. */
static int proc_place_terrain(SceneDoc*s,uint8_t*busy,const ProcRecipe*r,uint32_t*rng,ProcReport*report,int*used,BlueprintCapturePart*p,int n,int w,int h,int kind){
    if(!n||*used+w*h>s->x*s->y*55/100)return 0;
    for(int attempt=0;attempt<1800;attempt++){
        int x=(int)(proc_random(rng)%(uint32_t)s->x),y=(int)(proc_random(rng)%(uint32_t)s->y);
        if(!proc_space(busy,s->x,s->y,x,y,w,h,r->gap))continue;
        for(int j=0;j<n;j++){BlueprintCapturePart*a=&p[j];SceneCell*c=&s->cells[(a->z*s->y+y+a->y)*s->x+x+a->x];c->lib[a->layer]=a->lib;c->local[a->layer]=a->local;}
        proc_mark(busy,s->x,x,y,w,h);*used+=w*h;report->placed++;report->reliefs++;
        if(kind==0)report->historical++;else if(kind==1)report->lowReliefs++;else report->upperReliefs++;
        return 1;
    }return 0;
}
/* Low dunes have an asymmetric outline: one broad flat crest and a short
   lateral lobe. Every tile uses one of the complete SAND 0..12 profiles. */
static int __attribute__((unused)) proc_low_parts(int w,int h,int variant,BlueprintCapturePart*p,int*count){
    int heights[32][32]={0};*count=0;
    for(int y=1;y<h;y++)for(int x=1;x<w;x++){
        int inside=x>=2&&x<=w-2&&y>=2&&y<=h-2;
        if(variant&1)inside|=x>=1&&x<=w/2&&y>=h/2&&y<h;
        else inside|=y>=1&&y<=h/2&&x>=w/2&&x<w;
        heights[y][x]=inside;
    }
    for(int y=0;y<h;y++)for(int x=0;x<w;x++){
        int hh[4]={heights[y][x],heights[y][x+1],heights[y+1][x+1],heights[y+1][x]},lo=-1;
        if(!hh[0]&&!hh[1]&&!hh[2]&&!hh[3])continue;
        for(int k=0;k<=12;k++){float profile[4];rh_profile(procSand,k,profile);int ok=1;for(int c=0;c<4;c++)if(-profile[c]!=hh[c]*8)ok=0;if(ok){lo=k;break;}}
        if(lo<0)return 0;
        p[(*count)++]=(BlueprintCapturePart){.x=x,.y=y,.layer=3,.lib=procSand,.local=lo};
    }
    return 1;
}
#include "workshop_procedural_geo.h"
static int proc_build_reliefs(SceneDoc*s,uint8_t*busy,const ProcRecipe*r,uint32_t*rng,ProcReport*report,int*used){
    int debris=library_find_name_ctx(L"DEBRIS",SRC_TFTD,L"TFTD ORIGINAL");if(debris<0||!library_load(debris))return 0;
    BlueprintCapturePart*p=(BlueprintCapturePart*)calloc(PROC_MAX_PARTS,sizeof(*p));if(!p)return 0;
    /* Large authored relief first: preserve concave/convex transitions and
       half plateaus rather than replacing everything by rectangular cones. */
    for(int pass=0;pass<(r->density==0?1:2);pass++){
        const wchar_t*name=pass?L"SEABED10":L"SEABED12";
        for(int i=0;i<procMotifCount;i++){ProcMotif*m=&procMotifs[i];if(!m->relief||wcscmp(m->name,name))continue;int n=0;
            for(int j=0;j<m->count;j++){BlueprintCapturePart a=m->parts[j];float hh[4];if(rh_profile(a.lib,a.local,hh))p[n++]=a;}
            proc_place_terrain(s,busy,r,rng,report,used,p,n,m->w,m->h,0);break;
        }
    }
    int geoOk=proc_geo_reliefs(s,busy,r,rng,report,used);
    free(p);return geoOk;
}
/* Decor units are authorized explicitly above, including all four-part rocks. */
static int proc_decorate(SceneDoc*s,uint8_t*busy,const ProcRecipe*r,uint32_t*rng,ProcReport*report,int*used){
    int weeds=library_find_name_ctx(L"WEEDS",SRC_TFTD,L"TFTD ORIGINAL"),rocks=library_find_name_ctx(L"ROCKS",SRC_TFTD,L"TFTD ORIGINAL");
    if(weeds<0||rocks<0||!library_load(weeds)||!library_load(rocks))return 0;
    const int weedIds[4]={0,3,5,8};
    struct {int x,y,family,key;} placed[512];int placedCount=0,area=s->x*s->y;
    int targets[3]={clampi(area/(r->density==0?400:r->density==1?130:55),2,180),clampi(area/(r->density==0?260:r->density==1?95:45),3,220),clampi(area/1400,1,6)};
    int motifUsed[PROC_MAX_MOTIFS]={0};
    for(int pass=0;pass<3;pass++){int family=(pass+2)%3;for(int group=0;group<targets[family];group++){
        for(int attempt=0;attempt<600;attempt++){
            BlueprintCapturePart parts[PROC_MAX_PARTS];int n=0,w=1,h=1,key=-1,mi=-1,plantCount=0;
            if(family==0){
                w=3;h=2;plantCount=2+(int)(proc_random(rng)%3);int first=(int)(proc_random(rng)%4),slots[6]={0,1,2,3,4,5};
                for(int k=5;k>0;k--){int j=(int)(proc_random(rng)%(uint32_t)(k+1)),t=slots[k];slots[k]=slots[j];slots[j]=t;}
                for(int k=0;k<plantCount;k++)parts[n++]=(BlueprintCapturePart){.x=slots[k]%3,.y=slots[k]/3,.z=0,.layer=3,.lib=weeds,.local=weedIds[(first+k)%4]};
            }else if(family==1){key=(int)(proc_random(rng)%(sizeof(procRockUnits)/sizeof(*procRockUnits)));const ProcRockUnit*u=&procRockUnits[key];w=u->w;h=u->h;for(int k=0;k<u->count;k++)parts[n++]=(BlueprintCapturePart){.x=k%w,.y=k/w,.layer=3,.lib=rocks,.local=u->ids[k]};}
            else{
                mi=(int)(proc_random(rng)%(uint32_t)procMotifCount);ProcMotif*m=&procMotifs[mi];
                /* Keep intact structural wrecks, never accidental rock clumps. */
                if(m->relief||wcsncmp(m->name,L"DEBRIS_STRUCTURAL_",18)||motifUsed[mi]>=max(1,area/7200))continue;
                w=m->w;h=m->h;n=m->count;key=mi;memcpy(parts,m->parts,(size_t)n*sizeof(*parts));
            }
            if(*used+w*h>area*70/100)break;
            int x=(int)(proc_random(rng)%(uint32_t)s->x),y=(int)(proc_random(rng)%(uint32_t)s->y);
            if(!proc_space(busy,s->x,s->y,x,y,w,h,family==2?r->gap:1)){report->skipped++;continue;}
            int ok=1;
            for(int j=0;j<placedCount;j++)if(placed[j].family==family){
                int dx=abs(x-placed[j].x),dy=abs(y-placed[j].y),distance=family==0?(r->density==2?4:6):family==1?(key==placed[j].key?(r->density==2?6:9):(r->density==2?3:4)):14;
                if(dx*dx+dy*dy<distance*distance){ok=0;break;}
            }
            if(!ok){report->skipped++;continue;}
            for(int j=0;j<n;j++){BlueprintCapturePart*a=&parts[j];SceneCell*c=&s->cells[(a->z*s->y+y+a->y)*s->x+x+a->x];c->lib[a->layer]=a->lib;c->local[a->layer]=a->local;}
            proc_mark(busy,s->x,x,y,w,h);*used+=w*h;report->placed++;
            placed[placedCount].x=x;placed[placedCount].y=y;placed[placedCount].family=family;placed[placedCount++].key=key;
            if(family==0)report->weeds+=plantCount;else if(family==1)report->rocks+=n;else{report->wrecks++;motifUsed[mi]++;}
            break;
        }
    }
    }
    return 1;
}
/* Build transactionally. Failure never changes A.scene, its selection or undo. */
static int proc_generate(const ProcRecipe*r,ProcReport*report){
    ZeroMemory(report,sizeof(*report));procError[0]=0;
    if(r->width<20||r->height<20||r->width>120||r->height>120||r->density<0||r->density>2||r->relief<0||r->relief>1||r->gap<2||r->gap>5||r->craft < -1||r->uso < -1){wcscpy(procError,tr(L"Parametres invalides : dimensions 20 a 120, passage 2 a 5 cases."));return 0;}
    if(!proc_catalog_build())return 0;report->motifs=procMotifCount;
    SceneDoc ships[2]={{0},{0}},s={0};int indices[2]={r->craft,r->uso},posx[2]={0},posy[2]={0};uint8_t*busy=NULL;
    int maxz=r->relief?6:1;for(int i=0;i<procMotifCount;i++)if((r->relief||!procMotifs[i].relief)&&procMotifs[i].z>maxz)maxz=procMotifs[i].z;
    for(int i=0;i<2;i++)if(indices[i]>=0){if(!proc_read_map(indices[i],&ships[i])){wcscpy(procError,tr(L"Appareil incomplet ou ressources manquantes : generation annulee."));goto fail;}if(ships[i].z>maxz)maxz=ships[i].z;}
    s.x=r->width;s.y=r->height;s.z=maxz;s.active=1;s.dirty=1;s.usoMapIndex=-1;s.macroW=(s.x+9)/10;s.macroH=(s.y+9)/10;
    size_t count=(size_t)s.x*s.y*s.z;s.cells=(SceneCell*)malloc(count*sizeof(SceneCell));s.plan=(PlanCell*)calloc(count,sizeof(PlanCell));s.macroFlags=(uint8_t*)calloc((size_t)s.macroW*s.macroH,1);busy=(uint8_t*)calloc((size_t)s.x*s.y,1);
    if(!s.cells||!s.plan||!s.macroFlags||!busy){wcscpy(procError,tr(L"Memoire insuffisante : la scene precedente est conservee."));goto fail;}memset(s.cells,0xff,count*sizeof(SceneCell));
    uint32_t rng=r->seed?r->seed:0x6d2b79f5u;int flat[5]={13,14,15,18,19};
    for(int y=0;y<s.y;y++)for(int x=0;x<s.x;x++){SceneCell*c=&s.cells[y*s.x+x];c->lib[0]=procSand;c->local[0]=flat[proc_random(&rng)%5];}
    /* Place largest ship first; no forced overlap when there is insufficient room. */
    int order[2]={0,1};if(ships[1].x*ships[1].y>ships[0].x*ships[0].y){order[0]=1;order[1]=0;}
    for(int k=0;k<2;k++){int i=order[k];if(indices[i]<0)continue;int found=0;
        for(int attempt=0;attempt<4000&&!found;attempt++){int x=(int)(proc_random(&rng)%(uint32_t)s.x),y=(int)(proc_random(&rng)%(uint32_t)s.y);if(proc_space(busy,s.x,s.y,x,y,ships[i].x,ships[i].y,r->gap)){posx[i]=x;posy[i]=y;proc_mark(busy,s.x,x,y,ships[i].x,ships[i].y);found=1;}}
        if(!found){wcscpy(procError,tr(L"Pas assez de place pour ces appareils et leurs acces. Agrandissez la carte ou retirez un appareil."));goto fail;}
    }

    int area=s.x*s.y,used=0;
    if(r->relief&&!proc_build_reliefs(&s,busy,r,&rng,report,&used)){wcscpy(procError,tr(L"Impossible de placer un relief multi-Z et de petites dunes avec leurs acces. Agrandissez la carte, reduisez l'intervalle ou choisissez Sol plat. La scene precedente est conservee."));goto fail;}
    if(!proc_decorate(&s,busy,r,&rng,report,&used)){wcscpy(procError,tr(L"WEEDS ou ROCKS historique introuvable. La scene precedente est conservee."));goto fail;}
    if(s.geoCount&&!proc_geo_decorate(&s,r,&rng,report))goto fail;
    if(!proc_connected(busy,s.x,s.y,&report->open)){wcscpy(procError,tr(L"Le controle des espaces libres a echoue. La scene precedente est conservee."));goto fail;}
    report->total=area;
    for(int i=0;i<2;i++)if(indices[i]>=0)proc_stamp_ship(&s,&ships[i],posx[i],posy[i]);
    if(r->craft>=0){s.craftX=posx[0];s.craftY=posy[0];s.craftW=ships[0].x;s.craftH=ships[0].y;}
    if(r->uso>=0){s.usoSlotActive=1;s.usoInserted=1;s.usoMapIndex=r->uso;s.usoSlotX=posx[1];s.usoSlotY=posy[1];s.usoSlotW=ships[1].x;s.usoSlotH=ships[1].y;wcsncpy(s.usoSlotName,A.maps[r->uso].name,95);}
    _snwprintf(s.title,159,tr(L"SAND + GEO procedural v5 | %dx%d | graine %u | densite %d | relief %d | passage %d"),s.x,s.y,(unsigned)r->seed,r->density,r->relief,r->gap);
    scene_free();A.scene=s;reset_undo();A.currentZ=0;A.viewMode=0;A.rmp.editMode=0;A.zoom=1;A.panX=A.panY=0;A.hoverValid=0;A.overviewDirty=1;scene_rebuild_macro_flags();
    free(busy);free(ships[0].cells);free(ships[1].cells);InvalidateRect(A.hwnd,NULL,FALSE);return 1;
fail:
    scene_geo_free(&s);free(busy);free(s.cells);free(s.plan);free(s.macroFlags);free(ships[0].cells);free(ships[1].cells);return 0;
}
static HWND proc_control(HWND parent,const wchar_t*cls,const wchar_t*text,DWORD style,int x,int y,int w,int h,int id){HWND c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,(HMENU)(INT_PTR)id,NULL,NULL);SendMessageW(c,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return c;}
static HWND proc_combo(HWND h,int y,int id,const wchar_t*label,const wchar_t**items,int count,int selected){proc_control(h,L"STATIC",label,0,20,y+4,180,22,0);HWND c=proc_control(h,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,208,y,390,200,id);for(int i=0;i<count;i++)SendMessageW(c,CB_ADDSTRING,0,(LPARAM)items[i]);SendMessageW(c,CB_SETCURSEL,selected,0);return c;}

static void proc_ship_combo(HWND combo,int kind){
    combo_add_map_by_kind(combo,kind,tr(L"Aucun"));
    const wchar_t*names[8]={tr(L"Enqueteur"),tr(L"Escorteur"),tr(L"Croiseur"),tr(L"Croiseur lourd"),tr(L"Chasseur"),L"Destroyer",tr(L"Cuirasse"),tr(L"Chaland")};
    int count=(int)SendMessageW(combo,CB_GETCOUNT,0,0);
    for(int j=1;j<count;j++){int mi=(int)SendMessageW(combo,CB_GETITEMDATA,j,0);if(mi<0||mi>=A.mapCount)continue;MapEntry*m=&A.maps[mi];const wchar_t*n=m->name;int sym=wcsncmp(n,L"JMUFO",5)==0,id=-1;if(wcsncmp(n,L"UFO0",4)==0&&wcslen(n)==5)id=n[4]-L'1';else if(sym&&wcslen(n)==7&&n[5]==L'0')id=n[6]-L'1';if(id<0||id>7)continue;
        wchar_t label[256];_snwprintf(label,255,tr(L"%ls%ls  |  %dx%d  |  %ls"),names[id],sym?tr(L" - sym"):L"",m->x,m->y,m->source==SRC_MOD?m->origin:L"Original");SendMessageW(combo,CB_DELETESTRING,j,0);SendMessageW(combo,CB_INSERTSTRING,j,(LPARAM)label);SendMessageW(combo,CB_SETITEMDATA,j,mi);
    }SendMessageW(combo,CB_SETCURSEL,0,0);
}
static int proc_can_replace(HWND h){
    if(A.scene.active&&A.scene.dirty){int answer=MessageBoxW(h,tr(L"Enregistrer la scene actuelle avant de continuer ?"),tr(L"Conserver votre travail"),MB_YESNOCANCEL|MB_ICONQUESTION);if(answer==IDCANCEL)return 0;if(answer==IDYES){scene_save_project();if(A.scene.dirty)return 0;}}
    /* A.map remains in memory under the scene; no unrelated MAP or RMP is erased. */
    return 1;
}
static int proc_combo_data(HWND c){int i=(int)SendMessageW(c,CB_GETCURSEL,0,0);return i<0?-1:(int)SendMessageW(c,CB_GETITEMDATA,i,0);}
#define PROC_PREFS L"Procedural"
static void proc_pref_int(const wchar_t*key,unsigned value){wchar_t b[32];_snwprintf(b,31,L"%u",value);WritePrivateProfileStringW(PROC_PREFS,key,b,A.configPath);}
static void proc_remember(const ProcRecipe*r){
    if(!A.configPath[0])return;
    proc_pref_int(L"Width",r->width);proc_pref_int(L"Height",r->height);proc_pref_int(L"Density",r->density);proc_pref_int(L"Relief",r->relief);proc_pref_int(L"Gap",r->gap);proc_pref_int(L"Seed",r->seed);
    WritePrivateProfileStringW(PROC_PREFS,L"CraftMap",r->craft>=0?A.maps[r->craft].path:L"",A.configPath);
    WritePrivateProfileStringW(PROC_PREFS,L"UsoMap",r->uso>=0?A.maps[r->uso].path:L"",A.configPath);
}
static int proc_restore_ship(HWND combo,const wchar_t*key){
    wchar_t path[PATH_CAP];GetPrivateProfileStringW(PROC_PREFS,key,L"",path,PATH_CAP,A.configPath);if(!path[0])return 1;
    int count=(int)SendMessageW(combo,CB_GETCOUNT,0,0);for(int j=1;j<count;j++){int mi=(int)SendMessageW(combo,CB_GETITEMDATA,j,0);if(mi>=0&&mi<A.mapCount&&!_wcsicmp(A.maps[mi].path,path)){SendMessageW(combo,CB_SETCURSEL,j,0);return 1;}}
    SendMessageW(combo,CB_SETCURSEL,0,0);return 0;
}
static void proc_restore(void){
    if(!A.configPath[0])return;wchar_t b[64];unsigned value;
    value=clampi(GetPrivateProfileIntW(PROC_PREFS,L"Width",60,A.configPath),20,120);_snwprintf(b,63,L"%u",value);SetWindowTextW(PUI.width,b);
    value=clampi(GetPrivateProfileIntW(PROC_PREFS,L"Height",60,A.configPath),20,120);_snwprintf(b,63,L"%u",value);SetWindowTextW(PUI.height,b);
    SendMessageW(PUI.density,CB_SETCURSEL,clampi(GetPrivateProfileIntW(PROC_PREFS,L"Density",1,A.configPath),0,2),0);
    SendMessageW(PUI.relief,CB_SETCURSEL,clampi(GetPrivateProfileIntW(PROC_PREFS,L"Relief",1,A.configPath),0,1),0);
    SendMessageW(PUI.gap,CB_SETCURSEL,clampi(GetPrivateProfileIntW(PROC_PREFS,L"Gap",3,A.configPath),2,5)-2,0);
    GetPrivateProfileStringW(PROC_PREFS,L"Seed",L"27092026",b,64,A.configPath);wchar_t*end;unsigned long long seed=wcstoull(b,&end,10);if(b[0]&&b[0]!=L'-'&&!*end&&seed<=UINT32_MAX)SetWindowTextW(PUI.seed,b);
    int craft=proc_restore_ship(PUI.craft,L"CraftMap"),uso=proc_restore_ship(PUI.uso,L"UsoMap");
    if(!craft||!uso)SetWindowTextW(PUI.report,tr(L"Derniers reglages restaures. Un appareil memorise est introuvable dans les ressources actuelles : selection remise sur Aucun. Verifiez l'appareil X-COM et l'USO avant de generer."));
}
static int proc_ui_generate(void){
    ProcRecipe r={0};r.width=edit_get_int(PUI.width,0);r.height=edit_get_int(PUI.height,0);r.density=(int)SendMessageW(PUI.density,CB_GETCURSEL,0,0);r.relief=(int)SendMessageW(PUI.relief,CB_GETCURSEL,0,0);r.gap=2+(int)SendMessageW(PUI.gap,CB_GETCURSEL,0,0);
    wchar_t b[64],*end=NULL;GetWindowTextW(PUI.seed,b,64);unsigned long long value=wcstoull(b,&end,10);if(!b[0]||*end||value>UINT32_MAX||b[0]==L'-'){SetWindowTextW(PUI.report,tr(L"La graine doit etre un entier entre 0 et 4294967295."));return 0;}r.seed=(uint32_t)value;
    r.craft=proc_combo_data(PUI.craft);r.uso=proc_combo_data(PUI.uso);if(!proc_can_replace(PUI.hwnd))return 0;
    ProcReport result;if(!proc_generate(&r,&result)){SetWindowTextW(PUI.report,procError);return 0;}
    proc_remember(&r);
    wchar_t summary[768];_snwprintf(summary,767,tr(L"Carte creee : %d x %d, graine %u.\r\n%d reliefs historiques, %d petites dunes Z0, %d relief multi-Z.\r\n%d plantes, %d pieces de roches, %d epaves ; %d decors sur plateaux.\r\n%d %% de cases hors emprises, toutes reliees entre elles.\r\nControle geometrique seulement : interieur des appareils, IA et mission a valider dans le moteur.\r\nVous pouvez fermer ce panneau, editer la carte et l'enregistrer en .JMW."),r.width,r.height,(unsigned)r.seed,result.historical,result.lowReliefs,result.upperReliefs,result.weeds,result.rocks,result.wrecks,result.upperDecor,result.open*100/result.total);SetWindowTextW(PUI.report,summary);set_status(tr(L"Carte procedurale creee. Edition et sauvegarde .JMW disponibles. Mission et RMP non generes."));return 1;
}
static LRESULT CALLBACK proc_wndproc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){case WM_CREATE:{
        PUI.hwnd=h;proc_control(h,L"STATIC",tr(L"FONDS SABLEUX - LABORATOIRE PROCEDURAL"),0,20,18,580,25,0);
        proc_control(h,L"STATIC",tr(L"Pieces et assemblages existants, places librement case par case.\r\nReliefs GEO raccordes, multi-Z et decors sur plateaux."),0,20,48,580,44,0);
        proc_control(h,L"STATIC",tr(L"Taille (20 a 120 cases)"),0,20,103,180,22,0);PUI.width=proc_control(h,L"EDIT",L"60",WS_BORDER|WS_TABSTOP|ES_NUMBER,208,100,75,25,7110);proc_control(h,L"STATIC",L"x",0,295,104,20,22,0);PUI.height=proc_control(h,L"EDIT",L"60",WS_BORDER|WS_TABSTOP|ES_NUMBER,320,100,75,25,7111);
        proc_control(h,L"STATIC",tr(L"Graine reproductible"),0,20,141,180,22,0);PUI.seed=proc_control(h,L"EDIT",L"27092026",WS_BORDER|WS_TABSTOP|ES_NUMBER,208,138,190,25,7112);proc_control(h,L"BUTTON",tr(L"Carte aleatoire"),WS_TABSTOP,420,138,178,27,PROC_NEW_SEED);
        const wchar_t*d[]={tr(L"Clairseme : quelques dunes et decors"),tr(L"Varie : reliefs mixtes et decors"),tr(L"Dense : nombreuses dunes et decors")};PUI.density=proc_combo(h,178,7113,tr(L"Richesse de la carte"),d,3,1);
        const wchar_t*r[]={tr(L"Sol plat et decors"),tr(L"Reliefs varies SAND + GEO, terrasses et multi-Z")};PUI.relief=proc_combo(h,218,7114,L"Relief",r,2,1);
        const wchar_t*g[]={tr(L"2 cases minimum"),tr(L"3 cases minimum"),tr(L"4 cases minimum"),tr(L"5 cases minimum")};PUI.gap=proc_combo(h,258,7115,tr(L"Espace entre ensembles"),g,4,1);
        PUI.craft=proc_combo(h,298,7116,tr(L"Appareil X-COM"),NULL,0,0);proc_ship_combo(PUI.craft,1);
        PUI.uso=proc_combo(h,338,7117,tr(L"Appareil alien"),NULL,0,0);proc_ship_combo(PUI.uso,2);
        proc_control(h,L"BUTTON",tr(L"Generer la carte"),WS_TABSTOP|BS_DEFPUSHBUTTON,20,385,185,34,PROC_GENERATE);proc_control(h,L"BUTTON",tr(L"Enregistrer .JMW"),WS_TABSTOP,220,385,185,34,PROC_SAVE);proc_control(h,L"BUTTON",tr(L"Retour a la carte"),WS_TABSTOP,420,385,178,34,PROC_CLOSE);
        PUI.report=proc_control(h,L"EDIT",tr(L"Choisissez les parametres puis cliquez sur Generer.\r\nLa meme graine et les memes ressources reproduisent la meme carte.\r\nLes appareils gardent leur orientation et un pourtour degage.\r\nAucun plafond interieur ajoute. Aucun fichier du jeu modifie."),WS_BORDER|ES_MULTILINE|ES_READONLY|WS_VSCROLL,20,438,578,138,7118);proc_restore();return 0;}
    case WM_COMMAND:switch(LOWORD(wp)){case IDOK:case PROC_GENERATE:proc_ui_generate();return 0;case PROC_NEW_SEED:{static uint32_t serial;wchar_t old[64],b[32];GetWindowTextW(PUI.seed,old,64);uint32_t previous=(uint32_t)wcstoull(old,NULL,10),next=(uint32_t)GetTickCount()^(++serial*0x9e3779b9u)^previous;next=proc_random(&next);if(next==previous)next++;_snwprintf(b,31,L"%u",(unsigned)next);SetWindowTextW(PUI.seed,b);if(!proc_ui_generate())SetWindowTextW(PUI.seed,old);return 0;}case PROC_SAVE:scene_save_project();return 0;case IDCANCEL:case PROC_CLOSE:DestroyWindow(h);return 0;}break;
    case WM_CLOSE:DestroyWindow(h);return 0;case WM_DESTROY:ZeroMemory(&PUI,sizeof(PUI));return 0;}
    return DefWindowProcW(h,msg,wp,lp);
}
static void open_procedural(void){if(PUI.hwnd){SetForegroundWindow(PUI.hwnd);return;}WNDCLASSW wc={0};wc.lpfnWndProc=proc_wndproc;wc.hInstance=GetModuleHandleW(NULL);wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"WorkshopProceduralV1";RegisterClassW(&wc);HWND h=CreateWindowW(wc.lpszClassName,tr(L"Generateur procedural - SAND Alpha"),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_VISIBLE, CW_USEDEFAULT,CW_USEDEFAULT,636,630,A.hwnd,NULL,wc.hInstance,NULL);ShowWindow(h,SW_SHOW);}


static void proc_close_panel(void){if(PUI.hwnd)DestroyWindow(PUI.hwnd);proc_catalog_clear();}

