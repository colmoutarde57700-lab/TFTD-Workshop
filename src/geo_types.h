#include "geo_catalog.h"
#include "geo_ports.h"
#include "geo_dynamic.h"
#define GEO_MAX 8192
typedef struct {int mesh,x,y,rotation,mirror,support;float z;char uid[80];GeoDynamic*dynamic;} GeoInstance;
typedef struct {int x,y,family,local;float z;} GeoDecor;
