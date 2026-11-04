#ifndef TD5_HD_MATERIAL_H
#define TD5_HD_MATERIAL_H

// one response table supplies native metadata and shader constants
#define HD_SURFACE_MATERIALS(X) \
    X(RUBBER, 0.88, 0.035, 0.0, 0.0) \
    X(PAINT, 0.30, 0.04, 1.0, 0.0) \
    X(GLASS, 0.08, 0.04, 0.0, 0.0) \
    X(METAL, 0.35, 0.55, 0.0, 0.0) \
    X(OPTIC, 0.18, 0.04, 0.0, 0.0) \
    X(ROAD, 0.72, 0.04, 0.0, 0.50) \
    X(MASONRY, 0.95, 0.03, 0.0, 0.08) \
    X(SOIL, 1.0, 0.02, 0.0, 0.05) \
    X(COATED, 0.30, 0.0, 1.0, 0.0)

typedef enum HD_SURFACE
{
    HD_SURFACE_AUTHORED,
#define HD_SURFACE_ENUM(name, r, f, c, h) HD_SURFACE_##name,
    HD_SURFACE_MATERIALS(HD_SURFACE_ENUM)
#undef HD_SURFACE_ENUM
    HD_SURFACE_COUNT
} HD_SURFACE;

typedef char HD_SURFACE_TAG_LAYOUT[(HD_SURFACE_METAL == 4 && HD_SURFACE_ROAD == 6 && HD_SURFACE_COUNT <= 16) ? 1 : -1];

typedef struct HD_SURFACE_RESPONSE
{
    float roughness, reflectance, clearcoat, relief;
} HD_SURFACE_RESPONSE;

static HD_SURFACE_RESPONSE hd_surface_response(HD_SURFACE surface)
{
#define HD_SURFACE_ROW(name, r, f, c, h) {(float)r, (float)f, (float)c, (float)h},
    static const HD_SURFACE_RESPONSE responses[HD_SURFACE_COUNT] = {{0, 0, 0, 0}, HD_SURFACE_MATERIALS(HD_SURFACE_ROW)};
#undef HD_SURFACE_ROW
    return responses[(unsigned)surface < HD_SURFACE_COUNT ? surface : HD_SURFACE_AUTHORED];
}

#define HD_SURFACE_GLSL_ROW(name, r, f, c, h) ",vec4(" #r "," #f "," #c "," #h ")"
#define HD_SURFACE_GLSL "const vec4 surfaceProfiles[]=vec4[](vec4(0.0)" HD_SURFACE_MATERIALS(HD_SURFACE_GLSL_ROW) ");"

// both direct light owners use the same layered response
#define HD_SURFACE_BRDF_GLSL \
    "float surfaceFresnel(float f0,float cosine){return f0+(1.0-f0)*pow(1.0-clamp(cosine,0.0,1.0),5.0);}" \
    "float surfaceLobe(float roughness,float f0,float nl,float nv,float nh,float vh){" \
    "float a=max(0.04,roughness*roughness),a2=a*a,d=nh*nh*(a2-1.0)+1.0;" \
    "float distribution=a2/(3.14159265*d*d),k=(roughness+1.0)*(roughness+1.0)*0.125;" \
    "float geometry=nv/(nv*(1.0-k)+k)*nl/(nl*(1.0-k)+k);" \
    "return distribution*geometry*surfaceFresnel(f0,vh)/(4.0*max(nv,0.0001));}" \
    "float surfaceSpecular(vec4 profile,vec3 n,vec3 v,vec3 l){" \
    "if(profile.y<=0.0&&profile.z<=0.0)return 0.0;float nl=max(dot(n,l),0.0),nv=max(dot(n,v),0.0001);" \
    "vec3 h=(l+v)*inversesqrt(max(dot(l+v,l+v),0.000001));float nh=max(dot(n,h),0.0),vh=max(dot(v,h),0.0);" \
    "float coat=profile.z;return (profile.y>0.0?surfaceLobe(profile.x,profile.y,nl,nv,nh,vh):0.0)*(1.0-coat*surfaceFresnel(0.04,nv))+" \
    "coat*surfaceLobe(0.12,0.04,nl,nv,nh,vh);}" \
    "float surfaceTransmission(vec4 profile,float nv){return 1.0-0.25*profile.z*surfaceFresnel(0.04,nv);}"

// bounds are native128x256 body-image UVs, reviewed against original artwork
static HD_SURFACE hd_car_surface(unsigned car, unsigned u0, unsigned v0, unsigned u1, unsigned v1)
{
    if (u0 >= u1 || v0 >= v1)
        return HD_SURFACE_COATED;

    typedef struct
    {
        unsigned car, u0, v0, u1, v1;
        HD_SURFACE surface;
    } REGION;

    static const REGION regions[] = {
        {0, 12, 131, 60, 155, HD_SURFACE_GLASS},  // vip
        {0, 16, 59, 57, 93, HD_SURFACE_GLASS},    // vip
        {0, 15, 93, 55, 131, HD_SURFACE_PAINT},   // vip
        {8, 32, 63, 62, 113, HD_SURFACE_GLASS},   // nis
        {8, 4, 57, 31, 113, HD_SURFACE_GLASS},    // nis
        {1, 17, 108, 56, 141, HD_SURFACE_PAINT},  // vet
        {1, 13, 141, 62, 177, HD_SURFACE_GLASS},  // vet
        {2, 11, 65, 57, 100, HD_SURFACE_GLASS},   // van
        {2, 13, 100, 50, 140, HD_SURFACE_PAINT},  // van
        {2, 14, 140, 56, 170, HD_SURFACE_GLASS},  // van
        {3, 16, 104, 58, 129, HD_SURFACE_PAINT},  // tvr
        {3, 20, 129, 66, 152, HD_SURFACE_GLASS},  // tvr
        {4, 14, 64, 60, 97, HD_SURFACE_GLASS},    // xkr
        {4, 20, 97, 57, 144, HD_SURFACE_PAINT},   // xkr
        {4, 20, 144, 57, 178, HD_SURFACE_GLASS},  // xkr
        {5, 17, 100, 62, 136, HD_SURFACE_GLASS},  // frd
        {5, 15, 136, 62, 177, HD_SURFACE_PAINT},  // frd
        {5, 14, 177, 64, 193, HD_SURFACE_GLASS},  // frd
        {6, 16, 90, 60, 116, HD_SURFACE_GLASS},   // 97c
        {6, 16, 116, 60, 155, HD_SURFACE_PAINT},  // 97c
        {6, 10, 155, 67, 191, HD_SURFACE_GLASS},  // 97c
        {7, 12, 48, 64, 86, HD_SURFACE_GLASS},    // sky
        {7, 23, 86, 45, 132, HD_SURFACE_PAINT},   // sky
        {7, 19, 132, 49, 166, HD_SURFACE_GLASS},  // sky
        {18, 5, 234, 18, 247, HD_SURFACE_OPTIC},  // ss1 auxiliary lamp
        {18, 60, 234, 73, 247, HD_SURFACE_OPTIC}, // ss1 auxiliary lamp
        {20, 16, 59, 57, 93, HD_SURFACE_GLASS},   // gtr
        {20, 17, 97, 54, 124, HD_SURFACE_PAINT},  // gtr
        {20, 12, 131, 60, 155, HD_SURFACE_GLASS}, // gtr
        {9, 58, 123, 116, 201, HD_SURFACE_PAINT}, // cob
        {10, 17, 89, 61, 107, HD_SURFACE_GLASS},  // gto
        {10, 20, 107, 58, 158, HD_SURFACE_PAINT}, // gto
        {10, 15, 158, 63, 168, HD_SURFACE_GLASS}, // gto
        {11, 7, 79, 70, 100, HD_SURFACE_GLASS},   // mus
        {11, 6, 100, 71, 135, HD_SURFACE_PAINT},  // mus
        {11, 7, 134, 70, 155, HD_SURFACE_GLASS},  // mus
        {12, 16, 108, 74, 126, HD_SURFACE_GLASS}, // cam
        {12, 20, 127, 66, 176, HD_SURFACE_PAINT}, // cam
        {13, 11, 116, 65, 140, HD_SURFACE_GLASS}, // 69v
        {14, 13, 64, 70, 88, HD_SURFACE_GLASS},   // chv
        {14, 15, 90, 65, 144, HD_SURFACE_PAINT},  // chv
        {14, 17, 144, 68, 170, HD_SURFACE_GLASS}, // chv
        {15, 13, 70, 62, 93, HD_SURFACE_GLASS},   // cud
        {15, 13, 96, 63, 140, HD_SURFACE_PAINT},  // cud
        {16, 65, 96, 112, 143, HD_SURFACE_PAINT}, // crg
        {17, 9, 70, 54, 93, HD_SURFACE_GLASS},    // atp
        {17, 13, 93, 50, 121, HD_SURFACE_PAINT},  // atp
        {17, 9, 122, 56, 151, HD_SURFACE_GLASS},  // atp
        {18, 12, 160, 69, 214, HD_SURFACE_PAINT}, // ss1
        {19, 11, 88, 61, 109, HD_SURFACE_PAINT},  // 128
        {19, 9, 111, 61, 141, HD_SURFACE_GLASS},  // 128
        {21, 6, 146, 63, 189, HD_SURFACE_GLASS},  // jag
        {22, 10, 30, 85, 74, HD_SURFACE_PAINT},   // cat
        {23, 5, 122, 58, 156, HD_SURFACE_GLASS},  // pit
        {24, 10, 194, 78, 229, HD_SURFACE_PAINT}, // c21
        {25, 33, 106, 56, 128, HD_SURFACE_PAINT}, // cop
        {26, 65, 96, 112, 143, HD_SURFACE_PAINT}, // day
        {27, 16, 108, 74, 126, HD_SURFACE_GLASS}, // hot
        {27, 20, 127, 66, 176, HD_SURFACE_PAINT}, // hot
        {29, 15, 47, 43, 111, HD_SURFACE_PAINT},  // sp1
        {30, 6, 158, 65, 205, HD_SURFACE_PAINT},  // sp2
        {31, 15, 136, 62, 177, HD_SURFACE_PAINT}, // sp3
        {31, 14, 177, 64, 201, HD_SURFACE_GLASS}, // sp3
        {32, 17, 104, 58, 140, HD_SURFACE_PAINT}, // sp4
        {33, 15, 136, 62, 177, HD_SURFACE_PAINT}, // sp5
        {33, 14, 177, 64, 193, HD_SURFACE_GLASS}, // sp5
        {34, 65, 96, 112, 143, HD_SURFACE_PAINT}, // sp6
        {35, 9, 151, 68, 194, HD_SURFACE_GLASS},  // sp7
        {36, 12, 94, 75, 125, HD_SURFACE_GLASS},  // sp8
        {36, 3, 154, 75, 194, HD_SURFACE_GLASS},  // sp8
    };
    if (car == 37)
        car = 0; // identical four body images and high-detail UV packets
    for (unsigned index = 0; index < sizeof(regions) / sizeof(regions[0]); ++index)
    {
        const REGION *region = &regions[index];
        if (car == region->car && u0 >= region->u0 && v0 >= region->v0 && u1 <= region->u1 && v1 <= region->v1)
            return region->surface;
    }
    // unclassified body regions retain their base art beneath a dielectric coat
    return HD_SURFACE_COATED;
}

#endif
