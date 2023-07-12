#pragma once

struct Point2D {
    float x;
    float y;
};

struct Point3D {
    float x;
    float y;
    float z;
};

Point2D trilaterate2d(Point2D anchor1, Point2D anchor2, Point2D anchor3, float range1, float range2, float range3);
Point3D trilaterate3d(Point3D anchor1, Point3D anchor2, Point3D anchor3, float range1, float range2, float range3);
void test();