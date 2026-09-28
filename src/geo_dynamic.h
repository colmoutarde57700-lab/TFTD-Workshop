/* New profile evaluator. Ports use the source kernel's rounded finite differences. */
typedef struct {GeoMesh mesh;GeoPorts ports;GeoVec v[700];GeoTri t[650];GeoDouble points[4][17],normals[4][17];double controls[16],rise;int kind;} GeoDynamic;
static double geo_round(double x){return floor(x*1e9+.5)/1e9;}
static double geo_height(const GeoDynamic*d,double x,double y){
 if(d->kind==2)return d->rise*fmax(0,2*x-1)*(3*y*y-2*y*y*y);
 double bx[4]={(1-x)*(1-x)*(1-x),3*x*(1-x)*(1-x),3*x*x*(1-x),x*x*x};
 double by[4]={(1-y)*(1-y)*(1-y),3*y*(1-y)*(1-y),3*y*y*(1-y),y*y*y},z=0;
 for(int j=0;j<4;j++)for(int i=0;i<4;i++)z+=d->controls[j*4+i]*bx[i]*by[j];return z;
}
static GeoDouble geo_normal(const GeoDynamic*d,double x,double y){double e=1e-5,a=-1.5*(geo_height(d,x+e,y)-geo_height(d,x-e,y))/(2*e),b=-1.5*(geo_height(d,x,y+e)-geo_height(d,x,y-e))/(2*e),n=sqrt(a*a+b*b+1);a=geo_round(a/n);b=geo_round(b/n);double c=geo_round(1/n);n=sqrt(a*a+b*b+c*c);return(GeoDouble){geo_round(a/n),geo_round(b/n),geo_round(c/n)};}
static void geo_dynamic_build(GeoDynamic*d,const char*id){
 d->mesh=(GeoMesh){id,d->v,d->t,289,0,0};d->ports.minz=1;d->ports.maxz=0;d->ports.checkCorners=1;
 for(int y=0;y<=16;y++)for(int x=0;x<=16;x++){double z=geo_round(geo_height(d,x/16.,y/16.));d->v[y*17+x]=(GeoVec){x/16.f,y/16.f,(float)z};d->ports.minz=fmin(d->ports.minz,z);d->ports.maxz=fmax(d->ports.maxz,z);}
 d->mesh.maxz=(float)d->ports.maxz;
 for(int y=0;y<16;y++)for(int x=0;x<16;x++){int k=y*17+x;d->t[d->mesh.nt++]=(GeoTri){k,k+1,k+18,0};d->t[d->mesh.nt++]=(GeoTri){k,k+18,k+17,0};}
 for(int s=0;s<4;s++){
  d->ports.side[s]=(GeoPort){d->points[s],d->normals[s],17,0};
  for(int k=0;k<=16;k++){double x=s==1?1:s==3?0:k/16.,y=s==0?0:s==2?1:k/16.;d->points[s][k]=(GeoDouble){x,y,geo_round(geo_height(d,x,y))};d->normals[s][k]=geo_normal(d,x,y);}
  for(int k=0;k<16;k++){GeoDouble a=d->points[s][k],b=d->points[s][k+1];if(fmax(a.z,b.z)<=1e-7)continue;GeoVec v[4]={{(float)a.x,(float)a.y,(float)a.z},{(float)a.x,(float)a.y,0},{(float)b.x,(float)b.y,0},{(float)b.x,(float)b.y,(float)b.z}};int f[2][3]={{0,1,2},{0,2,3}};
   for(int q=0;q<2;q++){if((q==0?a.z:b.z)/16.<1e-7)continue;int n=d->mesh.nv;for(int j=0;j<3;j++)d->v[d->mesh.nv++]=v[f[q][j]];d->t[d->mesh.nt++]=(GeoTri){n,n+1,n+2,1};}
  }
 }
}
