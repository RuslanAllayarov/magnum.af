#include <gtest/gtest.h>
#include <vector>
#include "../../../src/mesh.cpp"
#include "../../../src/func.cpp"
#include "../../../src/misc.cpp"
#include "../../../src/llg_terms/micro_demag_nonequi.cpp"
#include "../../../src/llg_terms/micro_demag.cpp"
 
// Exemplary unit test
TEST(NonEquiDemagNxxNxyNearTest, n) {
    int ix = 1;
    int iy = 2;
    int iz = 3;
    double dx = 1;
    double dy = 2;
    double dz = 3;
    double dX = dx;
    double dY = dy;
    double dZ = dz;
    double x = (double) ix * dx;
    double y = (double) iy * dy;
    double z = (double) iz * dz;

    double Nxx_non = newell_nonequi::Nxx(x, y, z, dx, dy, dz, dX, dY, dZ);
    double Nxx_f = newell::Nxx(ix, iy, iz, dx, dy, dz);
    double Nxy_non = newell_nonequi::Nxy(x, y, z, dx, dy, dz, dX, dY, dZ);
    double Nxyg = newell::Nxy(ix, iy, iz, dx, dy, dz);

    EXPECT_NEAR(Nxx_non, Nxx_f, 1e-14);
    double Nxx_rel_diff = fabs(2*(Nxx_non-Nxx_f)/(Nxx_non+Nxx_f));
    EXPECT_NEAR(Nxx_rel_diff, 0, 1e-11);

    EXPECT_NEAR(Nxy_non, Nxyg, 1e-15);
    double Nxy_rel_diff = fabs(2*(Nxy_non-Nxyg)/(Nxy_non+Nxyg));
    EXPECT_NEAR(Nxy_rel_diff, 0, 1e-10);
}

TEST(NonEquiDemagNxxNxyFarTest, n) {
    int ix = 1;
    int iy = 2;
    int iz = 300;
    double dx = 1;
    double dy = 2;
    double dz = 3;
    double dX = dx;
    double dY = dy;
    double dZ = dz;
    double x = (double) ix * dx;
    double y = (double) iy * dy;
    double z = (double) iz * dz;

    double Nxx_non = newell_nonequi::Nxx(x, y, z, dx, dy, dz, dX, dY, dZ);
    double Nxx_f = newell::Nxx(ix, iy, iz, dx, dy, dz);
    double Nxy_non = newell_nonequi::Nxy(x, y, z, dx, dy, dz, dX, dY, dZ);
    double Nxyg = newell::Nxy(ix, iy, iz, dx, dy, dz);

    double Nxx_rel_diff = fabs(2*(Nxx_non-Nxx_f)/(Nxx_non+Nxx_f));
    double Nxx_abs_diff = fabs(Nxx_non-Nxx_f);

    EXPECT_NEAR(Nxx_rel_diff, 0, 1e1);
    EXPECT_NEAR(Nxx_abs_diff, 0, 1e-8);

    double Nxy_rel_diff = fabs(2*(Nxy_non-Nxyg)/(Nxy_non+Nxyg));
    double Nxy_abs_diff = fabs(Nxy_non-Nxyg);

    EXPECT_NEAR(Nxy_rel_diff, 0, 1e1);
    EXPECT_NEAR(Nxy_abs_diff, 0, 1e-12);
}

TEST(NonEquiDemagDistanceFromIndexTest, n) {
    std::vector<double> vz = {1, 1, 1};
    EXPECT_EQ(newell_nonequi::nonequi_index_distance(vz, 0, 1), 1);
    EXPECT_EQ(newell_nonequi::nonequi_index_distance(vz, 0, 2), 2);
    EXPECT_EQ(newell_nonequi::nonequi_index_distance(vz, 1, 0), -1);
    EXPECT_EQ(newell_nonequi::nonequi_index_distance(vz, 2, 0), -2);
    EXPECT_EQ(newell_nonequi::nonequi_index_distance(vz, 0, 3, false), 3);//Note: No bound error as the last element is not included by design.
}

TEST(NonEquiDemagUnitCubeTest, n) {
    const double a = 2;
    const int  ix = 2;
    double Nxx = newell::Nxx(ix, 0, 0, a, a, a);
    double Nxx_ne = newell_nonequi::Nxx(ix * a, 0, 0, a, a, a, a, a, a);
    EXPECT_NEAR(Nxx, Nxx_ne, 1e-16);
}

TEST(NonEquiDemagSymmetryTest, n) {
    const double dx = 2, dy = 3, dz = 4;
    const double dX = 3, dY = 4, dZ = 5;
    const double x = 20, y = 22, z = 24;
    double F = newell_nonequi::F_test(x, y, z, dx, dy, dz, dX, dY, dZ);
    //std::cout << "F = " << F << std::endl;
    double F_switch = newell_nonequi::F_test(-x, -y, -z, dX, dY, dZ, dx, dy, dz);
    //std::cout << "F_switch = " << F_switch << std::endl;
    EXPECT_NEAR(F, F_switch, 1e-11);

    double Nxx = newell_nonequi::Nxx(x, y, z, dx, dy, dz, dX, dY, dZ);
    double Nxx_switch = newell_nonequi::Nxx(-x, -y, -z, dX, dY, dZ, dx, dy, dz);
    EXPECT_NEAR(Nxx * (dX * dY * dZ), Nxx_switch * (dx * dy * dz), 1e-11);
}

//Testing layout in x: #|##
TEST(NonEquiDemagTwoCubesVersusOneCubeTest_x, n) {
    const double a = 2;
    const int ix = 1;

    const double Nxx_1 = newell::Nxx(  ix, 0, 0, a, a, a);
    const double Nxx_2 = newell::Nxx(2*ix, 0, 0, a, a, a);
    const double Nxx = Nxx_1 + Nxx_2;
    const double Nxx_ne = newell_nonequi::Nxx(- ix * a, 0, 0, 2*a, a, a, a, a, a);
    const double Nxx_ne2 = newell_nonequi::Nxx(2 * ix * a, 0, 0, 2*a, a, a, a, a, a);// x-symmetric case
    EXPECT_NEAR(Nxx, Nxx_ne, 1e-16);
    EXPECT_NEAR(Nxx, Nxx_ne2, 1e-16);

    const double Nxy_1 = newell::Nxy(  ix, 0, 0, a, a, a);
    const double Nxy_2 = newell::Nxy(2*ix, 0, 0, a, a, a);
    const double Nxy = Nxy_1 + Nxy_2;
    const double Nxy_ne = newell_nonequi::Nxy(- ix * a, 0, 0, 2*a, a, a, a, a, a);
    const double Nxy_ne2 = newell_nonequi::Nxy(2 * ix * a, 0, 0, 2*a, a, a, a, a, a);// x-symmetric case
    EXPECT_NEAR(Nxy, Nxy_ne, 1e-16);
    EXPECT_NEAR(Nxy, Nxy_ne2, 1e-16);
    //std::cout << " Nxy1   " << Nxy_1 << std::endl;
    //std::cout << " Nxy2   " << Nxy_2 << std::endl;
    //std::cout << " Nxy    " << Nxy << std::endl;
    ////const double Nxy_ne = newell_nonequi::Nxy(ix * a, 0, 0, a, a, a, 2*a, a, a);
    ////const double Nxy_ne = newell_nonequi::Nxy(-ix * a, 0, 0, 2*a, a, a, a, a, a);
    //std::cout << " Nxy_ne " << Nxy_ne << std::endl;
    //std::cout << " Nxy_2" << Nxy_ne2 << std::endl;
}

//Testing layout in y: #|##
TEST(NonEquiDemagTwoCubesVersusOneCubeTest_y, n) {
    const double a = 2;
    const int iy = 1;

    const double Nxx_1 = newell::Nxx(0,   iy, 0, a, a, a);
    const double Nxx_2 = newell::Nxx(0, 2*iy, 0, a, a, a);
    const double Nxx = Nxx_1 + Nxx_2;
    const double Nxx_ne = newell_nonequi::Nxx(0, -iy * a, 0, a, 2*a, a, a, a, a);
    EXPECT_NEAR(Nxx, Nxx_ne, 1e-15);

    const double Nxy_1 = newell::Nxy(0,   iy, 0, a, a, a);
    const double Nxy_2 = newell::Nxy(0, 2*iy, 0, a, a, a);
    const double Nxy = Nxy_1 + Nxy_2;
    const double Nxy_ne = newell_nonequi::Nxy(0, -iy * a, 0, a, 2*a, a, a, a, a);
    EXPECT_NEAR(Nxy, Nxy_ne, 1e-15);
}

//Testing layout in z: #|##
TEST(NonEquiDemagTwoCubesVersusOneCubeTest_z, n) {
    const double a = 2;
    const int iz = 1;

    const double Nxx_1 = newell::Nxx(0, 0,   iz, a, a, a);
    const double Nxx_2 = newell::Nxx(0, 0, 2*iz, a, a, a);
    const double Nxx = Nxx_1 + Nxx_2;
    const double Nxx_ne = newell_nonequi::Nxx(0, 0, -iz * a, a, a, 2*a, a, a, a);
    EXPECT_NEAR(Nxx, Nxx_ne, 1e-15);
}

//                             |#
//Testing layout in x and +y: #|#
TEST(NonEquiDemagTwoCubesVersusOneCubeTest_xy, n) {
    const double a = 2;
    const int ixy = 1;

    const double Nxx_1 = newell::Nxx(ixy,   ixy, 0, a, a, a);
    const double Nxx_2 = newell::Nxx(ixy, 2*ixy, 0, a, a, a);
    const double Nxx = Nxx_1 + Nxx_2;
    const double Nxx_ne = newell_nonequi::Nxx(-ixy * a, -ixy * a, 0, a, 2*a, a, a, a, a);
    EXPECT_NEAR(Nxx, Nxx_ne, 1e-15);

    const double Nxy_1 = newell::Nxy(ixy,   ixy, 0, a, a, a);
    const double Nxy_2 = newell::Nxy(ixy, 2*ixy, 0, a, a, a);
    const double Nxy = Nxy_1 + Nxy_2;
    const double Nxy_ne = newell_nonequi::Nxy(-ixy * a, -ixy * a, 0, a, 2*a, a, a, a, a);
    EXPECT_NEAR(Nxy, Nxy_ne, 1e-15);
}

//Testing layout in x and -y: #|#
//                             |#
TEST(NonEquiDemagTwoCubesVersusOneCubeTest_x_minus_y, n) {
    const double a = 2;
    const int ixy = 1;

    const double Nxx_1 = newell::Nxx(ixy, -  ixy, 0, a, a, a);
    const double Nxx_2 = newell::Nxx(ixy, -2*ixy, 0, a, a, a);
    const double Nxx = Nxx_1 + Nxx_2;
    const double Nxx_ne = newell_nonequi::Nxx(-ixy * a, 2*ixy * a, 0, a, 2*a, a, a, a, a);
    EXPECT_NEAR(Nxx, Nxx_ne, 1e-15);

    const double Nxy_1 = newell::Nxy(ixy, -  ixy, 0, a, a, a);
    const double Nxy_2 = newell::Nxy(ixy, -2*ixy, 0, a, a, a);
    const double Nxy = Nxy_1 + Nxy_2;
    const double Nxy_ne = newell_nonequi::Nxy(-ixy * a, 2*ixy * a, 0, a, 2*a, a, a, a, a);
    EXPECT_NEAR(Nxy, Nxy_ne, 1e-15);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
