/* Native GEO_TERRAIN preview laboratory. Logical MAP/MCD scene stays separate. */
#define IDM_GEO_NATIVE 1220
static struct {HWND hwnd,seed,size,reference,level,material,profile,info;GeoInstance ins[GEO_MAX];int count,dirty,grounded,crease,selected;GeoDecor decor[512];int nd;uint32_t seedValue;int width,height;float zoom,panX,panY;int dragging,mx,my;uint32_t*pixels;float*depth;int*pick;int bw,bh;float scale,ox,oy;} GEO;
static char*geoSource;
static int geoDomain,geoMinX,geoMaxX,geoMinY,geoMaxY;
static const GeoMesh*geo_mesh(const GeoInstance*i){return i->dynamic?&i->dynamic->mesh:&geo_meshes[i->mesh];}
static const GeoPorts*geo_interfaces(const GeoInstance*i){return i->dynamic?&i->dynamic->ports:&geo_ports[i->mesh];}
static int geo_visible(const GeoInstance*i){return !geoDomain||(i->x>=geoMinX&&i->x<geoMaxX&&i->y>=geoMinY&&i->y<geoMaxY);}
static void geo_clear(void){for(int i=0;i<GEO.count;i++)free(GEO.ins[i].dynamic);GEO.count=GEO.nd=0;free(geoSource);geoSource=NULL;geoDomain=0;}

static int geo_id(const char*s){for(int i=0;i<GEO_MESH_COUNT;i++)if(!strcmp(s,geo_meshes[i].id))return i;return -1;}
static GeoVec geo_transform(GeoVec v,const GeoInstance*i){float x=v.x-.5f,y=v.y-.5f;if(i->mirror)x=-x;for(int k=0;k<i->rotation;k++){float t=x;x=-y;y=t;}return (GeoVec){x+.5f+i->x,y+.5f+i->y,v.z+i->z};}
static GeoDouble geo_double_transform(GeoDouble v,const GeoInstance*i,int normal){double x=v.x-(normal?0:.5),y=v.y-(normal?0:.5);if(i->mirror)x=-x;for(int k=0;k<i->rotation;k++){double t=x;x=-y;y=t;}return (GeoDouble){x+(normal?0:.5+i->x),y+(normal?0:.5+i->y),v.z+(normal?0:i->z)};}
static GeoPort geo_port(const GeoInstance*i,int side,int*reverse){
    const GeoPorts*m=geo_interfaces(i);for(int k=0;k<4;k++){GeoPort p=m->side[k];GeoDouble a=geo_double_transform(p.p[0],i,0),b=geo_double_transform(p.p[p.count-1],i,0);int s=(fabs(a.x-i->x)<1e-7&&fabs(b.x-i->x)<1e-7)?3:(fabs(a.x-i->x-1)<1e-7&&fabs(b.x-i->x-1)<1e-7)?1:(fabs(a.y-i->y)<1e-7&&fabs(b.y-i->y)<1e-7)?0:2;if(s==side){*reverse=(s==0||s==2)?a.x>b.x:a.y>b.y;return p;}}*reverse=0;return m->side[side];
}
typedef struct {int x,y,z;double height;} GeoCorner;
static int geo_corner_order(const void*a,const void*b){const GeoCorner*x=a,*y=b;if(x->x!=y->x)return x->x-y->x;if(x->y!=y->y)return x->y-y->y;return x->z-y->z;}
static int geo_corners(const GeoInstance*ins,int count){GeoCorner*c=malloc((size_t)(count?count:1)*4*sizeof(*c));if(!c)return 0;int n=0;
 for(int i=0;i<count;i++){const GeoInstance*a=&ins[i];const GeoPorts*p=geo_interfaces(a);if(a->support||!p->checkCorners)continue;for(int side=0;side<=2;side+=2){GeoPort q=p->side[side];for(int k=0;k<2;k++){GeoDouble v=geo_double_transform(q.p[k?q.count-1:0],a,0);c[n++]=(GeoCorner){(int)llround(v.x),(int)llround(v.y),(int)floor(a->z+1e-7),v.z};}}}
 qsort(c,n,sizeof(*c),geo_corner_order);int ok=1;for(int i=0;i<n;){int j=i+1;double lo=c[i].height,hi=lo;while(j<n&&!geo_corner_order(c+i,c+j)){lo=fmin(lo,c[j].height);hi=fmax(hi,c[j].height);j++;}if(j-i>=3&&hi-lo>1e-7){ok=0;break;}i=j;}free(c);return ok;}
static int geo_validate(const GeoInstance*ins,int count,int grounded,int crease){
    int hasSupport=grounded;for(int i=0;i<count;i++)if(ins[i].support)hasSupport=1;
    for(int n=0;n<count;n++){const GeoInstance*a=&ins[n];const GeoPorts*ap=geo_interfaces(a);
        if(hasSupport){if(a->z<0||a->z!=floorf(a->z))return 0;if(a->support){if(!ap->supportKind)return 0;int above=0;for(int j=0;j<count;j++)if(j!=n&&ins[j].x==a->x&&ins[j].y==a->y&&fabs(ins[j].z-a->z-1)<1e-7)above=1;if(!above)return 0;}for(int z=0;z<(int)a->z;z++){int found=0;for(int j=0;j<count;j++)if(ins[j].x==a->x&&ins[j].y==a->y&&ins[j].support&&fabs(ins[j].z-z)<1e-7)found=1;if(!found)return 0;}}
        for(int j=0;j<n;j++){const GeoInstance*b=&ins[j];const GeoPorts*bp=geo_interfaces(b);int dx=b->x-a->x,dy=b->y-a->y;
            if(!dx&&!dy){double amin=a->z+ap->minz,amax=a->z+ap->maxz,bmin=b->z+bp->minz,bmax=b->z+bp->maxz;if(fmin(amax,bmax)>fmax(amin,bmin)+1e-7||fabs(a->z-b->z)<1e-7||(amax-amin<1e-7&&amin>bmin+1e-7&&amin<bmax-1e-7)||(bmax-bmin<1e-7&&bmin>amin+1e-7&&bmin<amax-1e-7))return 0;continue;}
            if(a->support||b->support||abs(dx)+abs(dy)!=1)continue;
            int side=dy==-1?0:dx==1?1:dy==1?2:3,ra,rb;GeoPort pa=geo_port(a,side,&ra),pb=geo_port(b,(side+2)%4,&rb);double alo=1e9,ahi=-1e9,blo=1e9,bhi=-1e9;
            for(int k=0;k<pa.count;k++){double z=pa.p[k].z+a->z;alo=fmin(alo,z);ahi=fmax(ahi,z);}for(int k=0;k<pb.count;k++){double z=pb.p[k].z+b->z;blo=fmin(blo,z);bhi=fmax(bhi,z);}
            if(floor(a->z+1e-7)!=floor(b->z+1e-7)&&(ahi<blo-1e-7||bhi<alo-1e-7))continue;if(pa.count!=pb.count||pa.jamb!=pb.jamb)return 0;
            for(int k=0;k<pa.count;k++){int ia=ra?pa.count-1-k:k,ib=rb?pb.count-1-k:k;GeoDouble x=geo_double_transform(pa.p[ia],a,0),y=geo_double_transform(pb.p[ib],b,0);if(fabs(x.x-y.x)>1e-7||fabs(x.y-y.y)>1e-7||fabs(x.z-y.z)>1e-7)return 0;x=geo_double_transform(pa.n[ia],a,1);y=geo_double_transform(pb.n[ib],b,1);if(!crease&&x.x*y.x+x.y*y.y+x.z*y.z<=1-1e-6)return 0;}
        }
    }return geo_corners(ins,count);
}
static GeoVec geo_project(GeoVec v){return (GeoVec){GEO.ox+(v.x-v.y)*GEO.scale,GEO.oy+((v.x+v.y)*.5f-v.z*1.5f)*GEO.scale,v.x+v.y+v.z*(2.f/3.f)};}
static void geo_add(const char*id,int x,int y,int z,int mirror,int support){if(GEO.count>=GEO_MAX)return;GeoInstance*i=&GEO.ins[GEO.count];*i=(GeoInstance){.mesh=geo_id(id),.x=x,.y=y,.z=(float)z,.mirror=mirror,.support=support};snprintf(i->uid,80,"g%d",GEO.count++);}
static void geo_json_string(FILE*f,const char*s){fputc('"',f);for(;*s;s++){if(*s=='"'||*s=='\\')fputc('\\',f);fputc(*s,f);}fputc('"',f);}
static int geo_save(const wchar_t*path){FILE*f=_wfopen(path,L"wb");if(!f)return 0;
    if(geoSource){int ok=fputs(geoSource,f)>=0;if(fclose(f))ok=0;return ok;}
    fprintf(f,"{\n\"format\":\"GEO_TERRAIN_SCENE\",\"version\":\"1.2.0\",\"id\":\"workshop_native\",\"allowCrease\":%s,\"requireGrounded\":%s,\"workshopSeed\":%u,\"instances\":[\n",GEO.crease?"true":"false",GEO.grounded?"true":"false",(unsigned)GEO.seedValue);
    for(int n=0;n<GEO.count;n++){GeoInstance*i=&GEO.ins[n];fprintf(f,"%s{\"uid\":",n?",\n":"");geo_json_string(f,i->uid);fprintf(f,",\"id\":\"%s\",\"x\":%d,\"y\":%d,\"z\":%.9g,\"rotation\":%d,\"mirror\":%s%s}",geo_meshes[i->mesh].id,i->x,i->y,i->z,i->rotation,i->mirror?"true":"false",i->support?",\"role\":\"support\"":"");}
    fprintf(f,"\n],\"workshopDecor\":[");for(int n=0;n<GEO.nd;n++){GeoDecor*d=&GEO.decor[n];fprintf(f,"%s{\"x\":%d,\"y\":%d,\"z\":%.9g,\"family\":%d,\"local\":%d}",n?",":"",d->x,d->y,d->z,d->family,d->local);}fprintf(f,"]}\n");int ok=!ferror(f);if(fclose(f))ok=0;return ok;
}
/* Bounded JSON import; unsupported profile families fail without replacing the current scene. */
typedef struct {int start,end,next;} GeoToken;
static void geo_ws(const char*s,int*p){while(s[*p]&&s[*p]<=32)(*p)++;}
static int geo_tokens(const char*s,int*p,GeoToken*t,int*n){
    int k=++*n;if(k>=65535)return 0;geo_ws(s,p);t[k].start=*p;char c=s[*p];if(!c)return 0;(*p)++;
    if(c=='{'||c=='['){char close=c=='{'?'}':']';geo_ws(s,p);if(s[*p]!=close)for(;;){if(c=='{'){if(s[*p]!='"'||!geo_tokens(s,p,t,n))return 0;geo_ws(s,p);if(s[(*p)++]!=':')return 0;}if(!geo_tokens(s,p,t,n))return 0;geo_ws(s,p);if(s[*p]==close)break;if(s[(*p)++]!=',')return 0;geo_ws(s,p);}if(s[*p]!=close)return 0;(*p)++;}
    else if(c=='"'){while(s[*p]&&s[*p]!='"'){if((unsigned char)s[*p]<32)return 0;if(s[*p]=='\\'){(*p)++;if(!s[*p]||!strchr("\"\\/bfnrtu",s[*p]))return 0;if(s[*p]=='u')for(int j=0;j<4;j++){(*p)++;if(!s[*p]||!isxdigit((unsigned char)s[*p]))return 0;}}(*p)++;}if(s[*p]!='"')return 0;(*p)++;}
    else{int start=t[k].start;while(s[*p]&&s[*p]!=','&&s[*p]!=']'&&s[*p]!='}'&&s[*p]>32)(*p)++;int len=*p-start;int literal=(len==4&&!memcmp(s+start,"true",4))||(len==5&&!memcmp(s+start,"false",5))||(len==4&&!memcmp(s+start,"null",4));if(!literal){if(c!='-'&&(c<'0'||c>'9'))return 0;char*end;double v=strtod(s+start,&end);if(end!=s+*p||!isfinite(v))return 0;}}
    t[k].end=*p;t[k].next=*n+1;return k;
}
static int geo_eq(const char*s,GeoToken*t,int k,const char*v){return k>0&&s[t[k].start]=='"'&&t[k].end-t[k].start==(int)strlen(v)+2&&!memcmp(s+t[k].start+1,v,strlen(v));}
static int geo_field(const char*s,GeoToken*t,int obj,const char*name){for(int k=obj+1;k<t[obj].next;){int v=k+1;if(geo_eq(s,t,k,name))return v;k=t[v].next;}return 0;}
static int geo_string(const char*s,GeoToken*t,int k,char*out,int cap){if(!k||s[t[k].start]!='"')return 0;int len=t[k].end-t[k].start-2;if(len<0||len>=cap)return 0;memcpy(out,s+t[k].start+1,len);out[len]=0;return !strchr(out,'\\');}
static double geo_number(const char*s,GeoToken*t,int k,double fallback){if(!k)return fallback;char*end;double v=strtod(s+t[k].start,&end);return end==s+t[k].end&&isfinite(v)?v:NAN;}
static int geo_bool(const char*s,GeoToken*t,int k){return k&&t[k].end-t[k].start==4&&!memcmp(s+t[k].start,"true",4);}
static int geo_custom(const char*s,GeoToken*t,int k,GeoInstance*i){
 if(s[t[k].start]!='{')return 0;char kind[64];if(!geo_string(s,t,geo_field(s,t,k,"kind"),kind,64))return 0;
 GeoDynamic*d=calloc(1,sizeof(*d));if(!d)return 0;i->dynamic=d;
 if(!strcmp(kind,"bezier_patch")){d->kind=1;int a=geo_field(s,t,k,"controls"),n=0;if(!a||s[t[a].start]!='[')return 0;for(int j=a+1;j<t[a].next;j=t[j].next){if(n==16)return 0;double v=geo_number(s,t,j,NAN);if(!isfinite(v)||v<0||v>1)return 0;d->controls[n++]=v;}if(n!=16)return 0;}
 else if(!strcmp(kind,"lateral_half_extent")){d->kind=2;d->rise=geo_number(s,t,geo_field(s,t,k,"rise"),.5);if(!isfinite(d->rise)||d->rise<=0||d->rise>1)return 0;}
 else return 0;
 for(int j=k+1;j<t[k].next;){if(!geo_eq(s,t,j,"kind")&&!geo_eq(s,t,j,d->kind==1?"controls":"rise"))return 0;j=t[j+1].next;}
 geo_dynamic_build(d,geo_meshes[i->mesh].id);return 1;
}
static int geo_load(const wchar_t*path){DWORD len=0;uint8_t*raw=read_all(path,&len);if(!raw||len>16*1024*1024){free(raw);return 0;}char*s=(char*)calloc((size_t)len+1,1);GeoToken*t=(GeoToken*)calloc(65536,sizeof(*t));GeoInstance*ins=(GeoInstance*)calloc(GEO_MAX,sizeof(*ins));GeoDecor dec[512];int nd=0,ok=0,count=0,domain=0,minX=0,maxX=0,minY=0,maxY=0;if(!s||!t||!ins)goto done;memcpy(s,raw,len);int pos=len>=3&&raw[0]==239&&raw[1]==187&&raw[2]==191?3:0,n=0;if(!geo_tokens(s,&pos,t,&n))goto done;while(s[pos]&&s[pos]<=32)pos++;if(s[pos])goto done;
    if(s[t[1].start]!='{')goto done;
    if(!geo_eq(s,t,geo_field(s,t,1,"format"),"GEO_TERRAIN_SCENE")||!geo_eq(s,t,geo_field(s,t,1,"version"),"1.2.0"))goto done;int caps=geo_field(s,t,1,"capabilitiesRequired");if(caps){if(s[t[caps].start]!='[')goto done;for(int k=caps+1;k<t[caps].next;k=t[k].next)if(!geo_eq(s,t,k,"bezier_patch_v1")&&!geo_eq(s,t,k,"lateral_half_extent_v1"))goto done;}
    int vd=geo_field(s,t,1,"visibleDomain");if(vd){if(s[t[vd].start]!='{')goto done;const char*keys[]={"minX","maxX","minY","maxY"};int*vals[]={&minX,&maxX,&minY,&maxY};for(int j=0;j<4;j++){double v=geo_number(s,t,geo_field(s,t,vd,keys[j]),NAN);if(!isfinite(v)||v!=floor(v)||fabs(v)>120)goto done;*vals[j]=(int)v;}if(minX>=maxX||minY>=maxY)goto done;domain=1;}
    int a=geo_field(s,t,1,"instances");if(!a||s[t[a].start]!='[')goto done;
    for(int k=a+1;k<t[a].next;k=t[k].next){if(count>=GEO_MAX||s[t[k].start]!='{')goto done;GeoInstance*i=&ins[count];char id[96],role[32];if(!geo_string(s,t,geo_field(s,t,k,"id"),id,96)||(i->mesh=geo_id(id))<0||!geo_string(s,t,geo_field(s,t,k,"uid"),i->uid,80))goto done;
        int params=geo_field(s,t,k,"params");if(params&&!geo_custom(s,t,params,i))goto done;
        double x=geo_number(s,t,geo_field(s,t,k,"x"),NAN),y=geo_number(s,t,geo_field(s,t,k,"y"),NAN),z=geo_number(s,t,geo_field(s,t,k,"z"),NAN),rot=geo_number(s,t,geo_field(s,t,k,"rotation"),0);
        if(!isfinite(x)||!isfinite(y)||!isfinite(z)||!isfinite(rot)||x!=floor(x)||y!=floor(y)||rot!=floor(rot)||fabs(x)>120||fabs(y)>120||fabs(z)>32||rot<0||rot>3)goto done;
        i->x=(int)x;i->y=(int)y;i->z=(float)z;i->rotation=(int)rot;int mk=geo_field(s,t,k,"mirror");if(mk&&!geo_bool(s,t,mk)&&!(t[mk].end-t[mk].start==5&&!memcmp(s+t[mk].start,"false",5)))goto done;i->mirror=geo_bool(s,t,mk);i->support=geo_string(s,t,geo_field(s,t,k,"role"),role,32)&&!strcmp(role,"support");for(int j=0;j<count;j++)if(!strcmp(i->uid,ins[j].uid))goto done;count++;
    }
    a=geo_field(s,t,1,"workshopDecor");if(a){if(s[t[a].start]!='[')goto done;for(int k=a+1;k<t[a].next;k=t[k].next){if(nd>=512)goto done;double x=geo_number(s,t,geo_field(s,t,k,"x"),NAN),y=geo_number(s,t,geo_field(s,t,k,"y"),NAN),z=geo_number(s,t,geo_field(s,t,k,"z"),NAN),family=geo_number(s,t,geo_field(s,t,k,"family"),NAN),local=geo_number(s,t,geo_field(s,t,k,"local"),NAN);if(!isfinite(x)||!isfinite(y)||!isfinite(z)||!isfinite(family)||!isfinite(local)||fabs(x)>120||fabs(y)>120||fabs(z)>32||x!=floor(x)||y!=floor(y)||(family!=0&&family!=1)||local!=floor(local)||local<0||local>10)goto done;dec[nd++]=(GeoDecor){(int)x,(int)y,(int)family,(int)local,(float)z};}}
    for(int j=0;j<nd;j++){GeoDecor*d=&dec[j];int valid=d->family?(d->local==0||d->local==1||d->local==10):(d->local==0||d->local==3||d->local==5||d->local==8);if(!valid)goto done;int surface=0;for(int i=0;i<count;i++)if(!ins[i].support&&ins[i].x==d->x&&ins[i].y==d->y){int m=ins[i].mesh;if((m==geo_id("flat")||m==geo_id("block_1_third")||m==geo_id("block_2_third"))&&fabs(ins[i].z+geo_meshes[m].maxz-d->z)<1e-6)surface=1;}if(!surface)goto done;for(int i=0;i<j;i++)if(dec[i].x==d->x&&dec[i].y==d->y&&fabs(dec[i].z-d->z)<1e-7)goto done;}
    if(!geo_validate(ins,count,geo_bool(s,t,geo_field(s,t,1,"requireGrounded")),geo_bool(s,t,geo_field(s,t,1,"allowCrease"))))goto done;
    geo_clear();geoSource=s;geoDomain=domain;geoMinX=minX;geoMaxX=maxX;geoMinY=minY;geoMaxY=maxY;
    memcpy(GEO.ins,ins,(size_t)count*sizeof(*ins));GEO.count=count;memcpy(GEO.decor,dec,(size_t)nd*sizeof(*dec));GEO.nd=nd;GEO.grounded=geo_bool(s,t,geo_field(s,t,1,"requireGrounded"));GEO.crease=geo_bool(s,t,geo_field(s,t,1,"allowCrease"));double seed=geo_number(s,t,geo_field(s,t,1,"workshopSeed"),0);GEO.seedValue=isfinite(seed)&&seed>=0&&seed<=UINT32_MAX?(uint32_t)seed:0;GEO.dirty=0;GEO.selected=-1;GEO.zoom=1;GEO.panX=GEO.panY=0;ok=1;s=NULL;
done:if(!ok&&ins)for(int j=0;j<GEO_MAX;j++)free(ins[j].dynamic);free(raw);free(s);free(t);free(ins);return ok;
}
static int geoRasterOpacity=100;
static void geo_triangle(GeoVec a,GeoVec b,GeoVec c,GeoVec wa,GeoVec wb,GeoVec wc,int zone,int owner,RhTexture*tex){
    float den=(b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y);if(fabsf(den)<.001f)return;
    int x0=max(0,(int)floorf(fminf(a.x,fminf(b.x,c.x)))),x1=min(GEO.bw-1,(int)ceilf(fmaxf(a.x,fmaxf(b.x,c.x))));int y0=max(0,(int)floorf(fminf(a.y,fminf(b.y,c.y)))),y1=min(GEO.bh-1,(int)ceilf(fmaxf(a.y,fmaxf(b.y,c.y))));
    float ux=wb.x-wa.x,uy=wb.y-wa.y,uz=(wb.z-wa.z)*1.5f,vx=wc.x-wa.x,vy=wc.y-wa.y,vz=(wc.z-wa.z)*1.5f,nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx,l=sqrtf(nx*nx+ny*ny+nz*nz);float shade=l?.55f+.45f*fabsf((-.3f*nx-.5f*ny+.81f*nz)/l):1;
    for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){float w0=((b.y-c.y)*(x+.5f-c.x)+(c.x-b.x)*(y+.5f-c.y))/den,w1=((c.y-a.y)*(x+.5f-c.x)+(a.x-c.x)*(y+.5f-c.y))/den,w2=1-w0-w1;if(w0<-.00001f||w1<-.00001f||w2<-.00001f)continue;size_t p=(size_t)y*GEO.bw+x;float depth=w0*a.z+w1*b.z+w2*c.z;if(depth<GEO.depth[p])continue;
        uint32_t col=zone?0xff8e9d9cu:0xffc5d5ceu;if(tex&&tex->p){float xx=w0*wa.x+w1*wb.x+w2*wc.x,yy=w0*wa.y+w1*wb.y+w2*wc.y,zz=w0*wa.z+w1*wb.z+w2*wc.z;float u=(zone?(fabsf(nx)>fabsf(ny)?yy:xx):xx)/8.f,v=zone?zz*1.5f:yy/8.f;u-=floorf(u);v-=floorf(v);col=tex->p[(size_t)min(tex->h-1,(int)(v*tex->h))*tex->w+min(tex->w-1,(int)(u*tex->w))];}
        if(owner==GEO.selected)col=0xffffcf60u;unsigned r=(unsigned)(((col>>16)&255)*shade),g=(unsigned)(((col>>8)&255)*shade),b2=(unsigned)((col&255)*shade);uint32_t out=0xff000000u|(r<<16)|(g<<8)|b2;if(geoRasterOpacity<100){uint32_t old=GEO.pixels[p];unsigned a=geoRasterOpacity,ia=100-a;out=0xff000000u|((((out>>16&255)*a+(old>>16&255)*ia)/100)<<16)|((((out>>8&255)*a+(old>>8&255)*ia)/100)<<8)|(((out&255)*a+(old&255)*ia)/100);}GEO.pixels[p]=out;GEO.depth[p]=depth;GEO.pick[p]=owner;
    }
}
static void geo_draw_decor(int level){
    int oldMode=A.assetRenderMode;A.assetRenderMode=1;
    for(int n=0;n<GEO.nd;n++){GeoDecor*d=&GEO.decor[n];if(geoDomain&&(d->x<geoMinX||d->x>=geoMaxX||d->y<geoMinY||d->y>=geoMaxY))continue;if(level>=0&&floorf(d->z+1e-7f)>level)continue;int li=library_find_name_ctx(d->family?L"ROCKS":L"WEEDS",SRC_TFTD,L"TFTD ORIGINAL");if(li<0||!library_load(li))continue;HDCacheEntry*hd=hd_load_entry(li,d->local);LibrarySet*lib=&A.library[li];int fr=mcd_frame(li,d->local);if((!hd||!hd->pixels)&&(!lib->sprites||fr<0||fr>=lib->frameCount))continue;
        GeoVec anchor=geo_project((GeoVec){(float)d->x,(float)d->y,d->z});int left=(int)(anchor.x-GEO.scale),top=(int)(anchor.y-GEO.scale*1.5f),w=(int)(GEO.scale*2),h=(int)(GEO.scale*2.5f);
        for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++){int x=left+xx,y=top+yy;if(x<0||y<0||x>=GEO.bw||y>=GEO.bh)continue;uint32_t col;if(hd&&hd->pixels)col=hd->pixels[(size_t)(yy*hd->h/h)*hd->w+xx*hd->w/w];else{int q=lib->sprites[(size_t)fr*TILE_W*TILE_H+(yy*TILE_H/h)*TILE_W+xx*TILE_W/w];col=q?pal_color((uint8_t)q):0;}if(!(col>>24))continue;float xy=d->x+d->y+1.f,z=(xy*.5f-(y-GEO.oy)/GEO.scale)/1.5f,depth=xy+z*(2.f/3.f)+.01f;size_t k=(size_t)y*GEO.bw+x;if(depth<GEO.depth[k])continue;GEO.pixels[k]=alpha_over(GEO.pixels[k],col);GEO.depth[k]=depth;}
    }A.assetRenderMode=oldMode;
}
static int geo_render(int w,int h,int level,int material){
    if(w<=0||h<=0)return 0;if(GEO.bw!=w||GEO.bh!=h){size_t count=(size_t)w*h;uint32_t*p=malloc(count*4);float*d=malloc(count*sizeof(float));int*i=malloc(count*sizeof(int));if(!p||!d||!i){free(p);free(d);free(i);return 0;}free(GEO.pixels);free(GEO.depth);free(GEO.pick);GEO.pixels=p;GEO.depth=d;GEO.pick=i;GEO.bw=w;GEO.bh=h;}
    for(int i=0;i<w*h;i++){GEO.pixels[i]=0xff101b22u;GEO.depth[i]=-1e9f;GEO.pick[i]=-1;}float minx=0,maxx=1,miny=0,maxy=1;
    if(GEO.count){minx=miny=1e9f;maxx=maxy=-1e9f;for(int n=0;n<GEO.count;n++){GeoInstance*i=&GEO.ins[n];if(!geo_visible(i))continue;for(int k=0;k<4;k++){float x=i->x+(k&1),y=i->y+(k>>1),z=i->z+geo_mesh(i)->maxz;float sx=x-y,sy=(x+y)*.5f;minx=fminf(minx,sx);maxx=fmaxf(maxx,sx);miny=fminf(miny,sy-z*1.5f);maxy=fmaxf(maxy,sy);}}}
    GEO.scale=fminf((w-60)/fmaxf(1,maxx-minx),(h-60)/fmaxf(1,maxy-miny))*GEO.zoom;GEO.ox=w*.5f-(minx+maxx)*.5f*GEO.scale+GEO.panX;GEO.oy=h*.5f-(miny+maxy)*.5f*GEO.scale+GEO.panY;
    RhTexture*top=NULL,*side=NULL;int oldMode=A.assetRenderMode;if(material){A.assetRenderMode=3;top=rh_texture(0);side=rh_texture(1);}
    for(int n=0;n<GEO.count;n++){GeoInstance*i=&GEO.ins[n];if(!geo_visible(i)||(level>=0&&i->z>level))continue;const GeoMesh*m=geo_mesh(i);GeoVec world[2048],screen[2048];if(m->nv>2048)continue;for(int v=0;v<m->nv;v++){world[v]=geo_transform(m->v[v],i);screen[v]=geo_project(world[v]);}for(int t=0;t<m->nt;t++){GeoTri tr=m->t[t];geo_triangle(screen[tr.a],screen[tr.b],screen[tr.c],world[tr.a],world[tr.b],world[tr.c],tr.zone,n,tr.zone?side:top);}}
    A.assetRenderMode=oldMode;geo_draw_decor(level);return 1;
}
static int geo_save_dialog(void){wchar_t path[PATH_CAP]=L"terrain.geo.scene.json";OPENFILENAMEW of={0};of.lStructSize=sizeof(of);of.hwndOwner=GEO.hwnd;of.lpstrFilter=tr(L"Scene GEO_TERRAIN (*.json)\0*.json\0\0");of.lpstrFile=path;of.nMaxFile=PATH_CAP;of.lpstrDefExt=L"json";of.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;if(!GetSaveFileNameW(&of))return 0;if(!geo_save(path)){MessageBoxW(GEO.hwnd,tr(L"Enregistrement impossible."),L"GEO_TERRAIN",MB_ICONERROR);return 0;}GEO.dirty=0;return 1;}
static int geo_replace(void){if(!GEO.dirty)return 1;int r=MessageBoxW(GEO.hwnd,tr(L"Enregistrer la scene GEO_TERRAIN avant de continuer ?"),tr(L"Conserver votre travail"),MB_YESNOCANCEL);return r==IDNO||(r==IDYES&&geo_save_dialog());}
static void geo_status(void){wchar_t text[512];if(GEO.selected>=0&&GEO.selected<GEO.count){GeoInstance*i=&GEO.ins[GEO.selected];wchar_t id[96];MultiByteToWideChar(CP_UTF8,0,geo_meshes[i->mesh].id,-1,id,96);_snwprintf(text,511,tr(L"%ls | case %d,%d | Z %.3g | %ls | rotation %d | miroir %ls"),id,i->x,i->y,(double)i->z,i->support?tr(L"soutien"):L"surface",i->rotation,i->mirror?tr(L"oui"):tr(L"non"));}else _snwprintf(text,511,tr(L"%d pieces GEO, %d decors | Molette : zoom ; bouton central : deplacer ; clic : identifier. Apercu, comportement OXCE non raccorde."),GEO.count,GEO.nd);SetWindowTextW(GEO.info,text);}
static void geo_paint(HWND h,HDC dc){RECT r;GetClientRect(h,&r);int level=(int)SendMessageW(GEO.level,CB_GETCURSEL,0,0)-1,material=(int)SendMessageW(GEO.material,CB_GETCURSEL,0,0);if(geo_render(r.right,r.bottom-142,level,material)){BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=GEO.bw;bi.bmiHeader.biHeight=-GEO.bh;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;SetDIBitsToDevice(dc,0,142,GEO.bw,GEO.bh,0,0,0,GEO.bh,GEO.pixels,&bi,DIB_RGB_COLORS);}}
static LRESULT CALLBACK geo_wndproc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){case WM_GETMINMAXINFO:{MINMAXINFO*m=(MINMAXINFO*)lp;m->ptMinTrackSize.x=1120;m->ptMinTrackSize.y=600;return 0;}case WM_CREATE:{GEO.hwnd=h;GEO.zoom=1;GEO.selected=-1;
        proc_control(h,L"STATIC",tr(L"GEO_TERRAIN - 102 GABARITS ET ASSEMBLAGES (scene independante)"),0,12,8,900,22,0);
        proc_control(h,L"BUTTON",tr(L"Enregistrer GEO"),0,12,36,150,28,7204);proc_control(h,L"BUTTON",tr(L"Ouvrir GEO"),0,180,36,140,28,7205);
        proc_control(h,L"STATIC",tr(L"Generation de cartes : menu Procedural"),0,340,42,445,24,0);
        GEO.level=proc_control(h,L"COMBOBOX",L"",CBS_DROPDOWNLIST,805,38,110,150,7206);SendMessageW(GEO.level,CB_ADDSTRING,0,(LPARAM)tr(L"Vue complete"));for(int i=0;i<=8;i++){wchar_t b[24];_snwprintf(b,23,tr(L"Jusqu'au Z%d"),i);SendMessageW(GEO.level,CB_ADDSTRING,0,(LPARAM)b);}SendMessageW(GEO.level,CB_SETCURSEL,0,0);
        GEO.material=proc_control(h,L"COMBOBOX",L"",CBS_DROPDOWNLIST,925,38,160,100,7207);SendMessageW(GEO.material,CB_ADDSTRING,0,(LPARAM)tr(L"Geometrie neutre"));SendMessageW(GEO.material,CB_ADDSTRING,0,(LPARAM)tr(L"Materiau SAND HD"));SendMessageW(GEO.material,CB_SETCURSEL,0,0);
        GEO.reference=proc_control(h,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,12,74,470,420,7208);for(int i=0;i<GEO_MESH_COUNT;i++){wchar_t b[96];MultiByteToWideChar(CP_UTF8,0,geo_meshes[i].id,-1,b,96);SendMessageW(GEO.reference,CB_ADDSTRING,0,(LPARAM)b);}SendMessageW(GEO.reference,CB_SETCURSEL,0,0);proc_control(h,L"BUTTON",tr(L"Voir cette tuile"),0,495,72,130,28,7209);proc_control(h,L"BUTTON",tr(L"Montages de reference..."),0,640,72,190,28,7210);
        GEO.info=proc_control(h,L"STATIC",L"",0,12,106,1150,30,0);geo_clear();geo_add("flat",0,0,0,0,0);wchar_t demo[PATH_CAP];_snwprintf(demo,PATH_CAP-1,L"%ls\\TFTD_HD_Gabarits_Universels\\Datasets\\GEO_TERRAIN\\data\\18_A_colline_isolee.scene.json",A.modsRoot);geo_load(demo);GEO.dirty=0;geo_status();return 0;}
    case WM_COMMAND:{int id=LOWORD(wp);if(id==7204)geo_save_dialog();else if(id==7205||id==7210){if(!geo_replace())return 0;wchar_t path[PATH_CAP]=L"",dir[PATH_CAP];_snwprintf(dir,PATH_CAP-1,L"%ls\\TFTD_HD_Gabarits_Universels\\Datasets\\GEO_TERRAIN\\data",A.modsRoot);OPENFILENAMEW of={0};of.lStructSize=sizeof(of);of.hwndOwner=h;of.lpstrFilter=tr(L"Scene GEO_TERRAIN (*.json)\0*.json\0\0");of.lpstrFile=path;of.nMaxFile=PATH_CAP;of.lpstrInitialDir=id==7210?dir:NULL;of.Flags=OFN_FILEMUSTEXIST;if(GetOpenFileNameW(&of)){if(!geo_load(path))MessageBoxW(h,tr(L"Import refuse : raccord, volume ou soutien invalide, ou format non pris en charge. Verifier le profil et les capacites requises. Scene precedente conservee."),L"GEO_TERRAIN",MB_ICONWARNING);else SendMessageW(GEO.level,CB_SETCURSEL,0,0);}}
        else if(id==7209){if(!geo_replace())return 0;int m=(int)SendMessageW(GEO.reference,CB_GETCURSEL,0,0);if(m>=0&&m<GEO_MESH_COUNT){geo_clear();geo_add(geo_meshes[m].id,0,0,0,0,0);GEO.zoom=1;GEO.panX=GEO.panY=0;GEO.dirty=0;GEO.selected=0;GEO.grounded=0;GEO.crease=0;SendMessageW(GEO.level,CB_SETCURSEL,0,0);}}
        geo_status();InvalidateRect(h,NULL,FALSE);return 0;}
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);geo_paint(h,dc);EndPaint(h,&ps);return 0;}
    case WM_PRINTCLIENT:geo_paint(h,(HDC)wp);return 0;
    case WM_SIZE:InvalidateRect(h,NULL,FALSE);return 0;
    case WM_MOUSEWHEEL:GEO.zoom=fmaxf(.3f,fminf(5,GEO.zoom*(GET_WHEEL_DELTA_WPARAM(wp)>0?1.15f:1/1.15f)));InvalidateRect(h,NULL,FALSE);return 0;
    case WM_MBUTTONDOWN:GEO.dragging=1;GEO.mx=GET_X_LPARAM(lp);GEO.my=GET_Y_LPARAM(lp);SetCapture(h);return 0;
    case WM_MBUTTONUP:GEO.dragging=0;ReleaseCapture();return 0;
    case WM_MOUSEMOVE:if(GEO.dragging){int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp);GEO.panX+=x-GEO.mx;GEO.panY+=y-GEO.my;GEO.mx=x;GEO.my=y;InvalidateRect(h,NULL,FALSE);}return 0;
    case WM_LBUTTONDOWN:{int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp)-142;if(GEO.pick&&x>=0&&y>=0&&x<GEO.bw&&y<GEO.bh){GEO.selected=GEO.pick[y*GEO.bw+x];geo_status();InvalidateRect(h,NULL,FALSE);}return 0;}
    case WM_CLOSE:if(geo_replace())DestroyWindow(h);return 0;
    case WM_DESTROY:geo_clear();free(GEO.pixels);free(GEO.depth);free(GEO.pick);ZeroMemory(&GEO,sizeof(GEO));return 0;}
    return DefWindowProcW(h,msg,wp,lp);
}
static void geo_open(void){if(GEO.hwnd){SetForegroundWindow(GEO.hwnd);return;}WNDCLASSW wc={0};wc.lpfnWndProc=geo_wndproc;wc.hInstance=GetModuleHandleW(NULL);wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"WorkshopGeoTerrain";RegisterClassW(&wc);CreateWindowW(wc.lpszClassName,tr(L"Workshop - GEO_TERRAIN - Gabarits et assemblages"),WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,1240,900,A.hwnd,NULL,wc.hInstance,NULL);}
