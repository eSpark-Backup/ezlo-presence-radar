#pragma once

typedef struct point_2d{
    float x;
    float y;
} point_2d_t;

typedef struct point_3d{
    float x;
    float y;
    float z;
} point_3d_t;

point_2d_t trilaterate2d(point_2d_t anchor1, point_2d_t anchor2, point_2d_t anchor3, float range1, float range2, float range3);
point_3d_t trilaterate3d(point_3d_t anchor1, point_3d_t anchor2, point_3d_t anchor3, float range1, float range2, float range3);
void test();