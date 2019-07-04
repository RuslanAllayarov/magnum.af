#pragma once
#include <iostream>
#include "arrayfire.h"

namespace magnumaf{

struct Mesh{
    Mesh (int, int, int, double, double, double);
    Mesh (){};

    int n0, n1, n2;               // Number of cells in x, y, z
    double dx, dy, dz;            // Distance between cells
    double V;                   // Volume of one cell
    int n0_exp, n1_exp, n2_exp; // Expanded cell sizes for demag FFT
    af::dim4 dims;
    af::dim4 dims_expanded;
    void print(std::ostream& stream);
    af::array skyrmconf(const bool point_up = false);
    af::array ellipse(const int xyz = 0, const bool positive_direction = true);
    af::array init_vortex(const bool positive_direction = true);
    af::array init_sp4();
};
}// namespace magnumaf
