#include <iostream>
#include <cmath>
#include "trilateration.hpp"

Point2D trilaterate2d(Point2D anchor1, Point2D anchor2, Point2D anchor3, float range1, float range2, float range3) {
    float x1 = anchor1.x;
    float y1 = anchor1.y;
    float x2 = anchor2.x;
    float y2 = anchor2.y;
    float x3 = anchor3.x;
    float y3 = anchor3.y;

    float A = 2 * (x2 - x1);
    float B = 2 * (y2 - y1);
    float C = std::pow(range1, 2) - std::pow(range2, 2) - std::pow(x1, 2) + std::pow(x2, 2) - std::pow(y1, 2) + std::pow(y2, 2);
    float D = 2 * (x3 - x2);
    float E = 2 * (y3 - y2);
    float F = std::pow(range2, 2) - std::pow(range3, 2) - std::pow(x2, 2) + std::pow(x3, 2) - std::pow(y2, 2) + std::pow(y3, 2);

    float denominator = A * E - B * D;
    
    if (denominator == 0) {
        // Anchors are in a straight line, trilateration is not possible
        return {0, 0};
    }

    float x = (C * E - B * F) / denominator;
    float y = (A * F - C * D) / denominator;

    Point2D target;
    target.x = x;
    target.y = y;

    return target;
}

Point3D trilaterate3d(Point3D anchor1, Point3D anchor2, Point3D anchor3, float range1, float range2, float range3) {
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

    float exx = (x2 - x1) / std::sqrt(P1P2Square);
    float exy = (y2 - y1) / std::sqrt(P1P2Square);
    float exz = (z2 - z1) / std::sqrt(P1P2Square);

    float i = exx * (x3 - x1) + exy * (y3 - y1) + exz * (z3 - z1);

    float eyx = (x3 - x1 - i * exx) / std::sqrt(P1P3Square - i * i);
    float eyy = (y3 - y1 - i * exy) / std::sqrt(P1P3Square - i * i);
    float eyz = (z3 - z1 - i * exz) / std::sqrt(P1P3Square - i * i);

    float ezx = exy * eyz - exz * eyy;
    float ezy = exz * eyx - exx * eyz;
    float ezz = exx * eyy - exy * eyx;

    float d = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));

    float j = eyx * (x3 - x1) + eyy * (y3 - y1) + eyz * (z3 - z1);

    float x = (std::pow(range1, 2) - std::pow(range2, 2) + std::pow(d, 2)) / (2 * d);
    float y = ((std::pow(range1, 2) - std::pow(range3, 2) + std::pow(i, 2) + std::pow(j, 2)) / (2 * j)) - (i / j) * x;
    float z = std::sqrt(std::pow(range1, 2) - std::pow(x, 2) - std::pow(y, 2));

    Point3D target;
    target.x = x1 + x * exx + y * eyx + z * ezx;
    target.y = y1 + x * exy + y * eyy + z * ezy;
    target.z = z1 + x * exz + y * eyz + z * ezz;
    printf ("Target coordinates: (%0.2f, %0.2f, %0.2f)\n",target.x, target.y, target.z);
    return target;
}

void test() {
    // Anchor points
    Point3D anchor1 = {0.0f, 0.0f, 0.0f};
    Point3D anchor2 = {5.0f, 0.0f, 0.0f};
    Point3D anchor3 = {0.0f, 5.0f, 0.0f};

    // Ranges from anchors to target point
    float range1 = 9.947864f;
    float range2 = 7.743384f;
    float range3 = 9.693297f;

    // Perform trilateration
    Point3D target = trilaterate3d(anchor1, anchor2, anchor3, range1, range2, range3);

    // Print the coordinates of the target point
    printf ("Target coordinates: (%0.2f, %0.2f, %0.2f)\n",target.x, target.y, target.z);
}

