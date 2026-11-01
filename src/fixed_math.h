#ifndef FIXED_MATH_H
#define FIXED_MATH_H

#include <stddef.h>

#include "psx.h"

/* Native signed screen coordinates, distinct from packed GPU XY */
typedef struct VEC2
{
    sint32 x;
    sint32 y;
} VEC2;

typedef char VEC2_COMPONENT_LAYOUT_CHECK[sizeof(sint32) == 4 && sizeof(VEC2) == 8 && offsetof(VEC2, x) == 0 && offsetof(VEC2, y) == 4 ? 1 : -1];

/* Native ground-plane offsets with explicit low-word arithmetic */
typedef union VEC2_XZ
{
    struct
    {
        sint32 x;
        sint32 z;
    };

    struct
    {
        uint32 x;
        uint32 z;
    } bits;
} VEC2_XZ;

typedef char VEC2_XZ_COMPONENT_LAYOUT_CHECK[sizeof(sint32) == 4 && sizeof(uint32) == 4 && sizeof(VEC2_XZ) == 8 && offsetof(VEC2_XZ, x) == 0 && offsetof(VEC2_XZ, z) == 4 && offsetof(VEC2_XZ, bits) == 0 ? 1 : -1];

/* Signed components with explicit views for low-word arithmetic */
typedef union VEC3
{
    struct
    {
        sint32 x;
        sint32 y;
        sint32 z;
    };

    sint32 v[3];

    struct
    {
        uint32 x;
        uint32 y;
        uint32 z;
    } bits;

    uint32 u[3];
} VEC3;

typedef char VEC3_COMPONENT_LAYOUT_CHECK[sizeof(sint32) == 4 && sizeof(uint32) == 4 && sizeof(VEC3) == 12 && offsetof(VEC3, x) == 0 && offsetof(VEC3, y) == 4 && offsetof(VEC3, z) == 8 && offsetof(VEC3, bits) == 0 && offsetof(VEC3, v) == 0 && offsetof(VEC3, u) == 0 ? 1 : -1];

/* Signed16 coordinates with explicit narrow-word arithmetic views */
typedef union VEC3_16
{
    struct
    {
        sint16 x;
        sint16 y;
        sint16 z;
    };

    sint16 v[3];

    struct
    {
        uint16 x;
        uint16 y;
        uint16 z;
    } bits;

    uint16 u[3];
} VEC3_16;

typedef char VEC3_16_COMPONENT_LAYOUT_CHECK[sizeof(sint16) == 2 && sizeof(uint16) == 2 && sizeof(VEC3_16) == 6 && offsetof(VEC3_16, x) == 0 && offsetof(VEC3_16, y) == 2 && offsetof(VEC3_16, z) == 4 && offsetof(VEC3_16, bits) == 0 && offsetof(VEC3_16, v) == 0 && offsetof(VEC3_16, u) == 0 ? 1 : -1];

/* Return a nonnegative angle with 4096 units per turn */
sint32 fixed_angle_from_vector(sint32 y, sint32 x);

uint32 fixed_reflect_heading(sint32 angle);

uint32 fixed_sqrt(uint32 value);

sint32 fixed_divide_checked(sint32 numerator, sint32 denominator);

#endif
