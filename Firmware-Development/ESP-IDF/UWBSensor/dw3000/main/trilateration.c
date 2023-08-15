#include <stdio.h>
#include <math.h>
#include "trilateration.h"

point_2d_t trilaterate2d(point_2d_t anchor1, point_2d_t anchor2, point_2d_t anchor3, float range1, float range2, float range3) {
    float x1 = anchor1.x;
    float y1 = anchor1.y;
    float x2 = anchor2.x;
    float y2 = anchor2.y;
    float x3 = anchor3.x;
    float y3 = anchor3.y;

    float A = 2 * (x2 - x1);
    float B = 2 * (y2 - y1);
    float C = pow(range1, 2) - pow(range2, 2) - pow(x1, 2) + pow(x2, 2) - pow(y1, 2) + pow(y2, 2);
    float D = 2 * (x3 - x2);
    float E = 2 * (y3 - y2);
    float F = pow(range2, 2) - pow(range3, 2) - pow(x2, 2) + pow(x3, 2) - pow(y2, 2) + pow(y3, 2);

    float denominator = A * E - B * D;
    
    if (denominator == 0) {
        // Anchors are in a straight line, trilateration is not possible
        point_2d_t target = {0, 0};
        return target;
    }

    float x = (C * E - B * F) / denominator;
    float y = (A * F - C * D) / denominator;

    point_2d_t target;
    target.x = x;
    target.y = y;

    return target;
}

point_3d_t trilaterate3d(point_3d_t anchor1, point_3d_t anchor2, point_3d_t anchor3, float range1, float range2, float range3) {
    float x1 = anchor1.x;
    float y1 = anchor1.y;
    float z1 = anchor1.z;
    float x2 = anchor2.x;
    float y2 = anchor2.y;
    float z2 = anchor2.z;
    float x3 = anchor3.x;
    float y3 = anchor3.y;
    float z3 = anchor3.z;

    float P1P2Square = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1);
    float P1P3Square = (x3 - x1) * (x3 - x1) + (y3 - y1) * (y3 - y1) + (z3 - z1) * (z3 - z1);

    float exx = (x2 - x1) / sqrt(P1P2Square);
    float exy = (y2 - y1) / sqrt(P1P2Square);
    float exz = (z2 - z1) / sqrt(P1P2Square);

    float i = exx * (x3 - x1) + exy * (y3 - y1) + exz * (z3 - z1);

    float eyx = (x3 - x1 - i * exx) / sqrt(P1P3Square - i * i);
    float eyy = (y3 - y1 - i * exy) / sqrt(P1P3Square - i * i);
    float eyz = (z3 - z1 - i * exz) / sqrt(P1P3Square - i * i);

    float ezx = exy * eyz - exz * eyy;
    float ezy = exz * eyx - exx * eyz;
    float ezz = exx * eyy - exy * eyx;

    float d = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));

    float j = eyx * (x3 - x1) + eyy * (y3 - y1) + eyz * (z3 - z1);

    float x = (pow(range1, 2) - pow(range2, 2) + pow(d, 2)) / (2 * d);
    float y = ((pow(range1, 2) - pow(range3, 2) + pow(i, 2) + pow(j, 2)) / (2 * j)) - (i / j) * x;
    float z = sqrt(pow(range1, 2) - pow(x, 2) - pow(y, 2));

    point_3d_t target;
    target.x = x1 + x * exx + y * eyx + z * ezx;
    target.y = y1 + x * exy + y * eyy + z * ezy;
    target.z = z1 + x * exz + y * eyz + z * ezz;

    return target;
}

void test() {
    // Anchor points
    point_3d_t anchor1 = {0.0f, 0.0f, 0.0f};
    point_3d_t anchor2 = {5.0f, 0.0f, 0.0f};
    point_3d_t anchor3 = {0.0f, 5.0f, 0.0f};

    // Ranges from anchors to target point
    float range1 = 9.947864f;
    float range2 = 7.743384f;
    float range3 = 9.693297f;

    // Perform trilateration
    point_3d_t target = trilaterate3d(anchor1, anchor2, anchor3, range1, range2, range3);

    // Print the coordinates of the target point
    printf ("Target coordinates: (%0.2f, %0.2f, %0.2f)\n",target.x, target.y, target.z);
}

