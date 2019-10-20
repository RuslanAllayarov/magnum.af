#include "../../../src/mesh.cpp"
#include "../../../src/nonequispaced_mesh.cpp"
#include "../../../src/constants.hpp"
#include "../../../src/material.cpp"
#include "../../../src/state.cpp"
#include "../../../src/vtk_IO.cpp"
#include "../../../src/func.cpp"
#include "../../../src/misc.cpp"
#include "../../../src/llg_terms/micro_demag_nonequi.cpp"
#include "../../../src/llg_terms/micro_demag.cpp"
#include <gtest/gtest.h>

using namespace magnumaf;


TEST(NonEquiDemag, energy_homogenuous_cube) {
    // Compare SP4 layer with homogeneous z-magnetization once with nz = 3 equidistant and nz=2 non-equidistant discretization
    const float dx=1e-9, dy=1e-9, dz=1e-9;
    const int nx=10, ny=10, nz=10;
    const float Ms = 8e5;

    std::vector<float> z_spacing;
    for(int i=0; i<nz; i++){
        if( i%2 == 0){
            z_spacing.push_back(0.5 * dz);
        }
        else {
            z_spacing.push_back(1.5 * dz);
        }
    }
    NonequispacedMesh mesh_ne(nx, ny, dx, dy, z_spacing);
    af::array m2 = af::constant(0.0, mesh_ne.dims, f32);
    m2(af::span, af::span, af::span, 2) = 1;

    State state(mesh_ne, Ms, m2, false, true);
    NonEquiDemagField demag = NonEquiDemagField(mesh_ne, false, false, 1);

    float E_analytic = 1./6. * nx * pow(dx, 3) * pow(state.Ms, 2) * constants::mu0;

    af::array h_demag = demag.h(state);
    // testing state.Ms
    EXPECT_NEAR(demag.E(state), E_analytic, 3.7e-19);
    EXPECT_NEAR(demag.E(state, h_demag), E_analytic, 3.7e-19);

    // testing state.Ms_field switch
    state.Ms_field = af::constant(8e5, mesh_ne.dims, f32);
    h_demag = demag.h(state);
    EXPECT_NEAR(demag.E(state), E_analytic, 3.7e-19);
    EXPECT_NEAR(demag.E(state, h_demag), E_analytic, 3.7e-19);
}


TEST(NonEquiDemagField, EnergyTest) {
    // Compare SP4 layer with random z-magnetization once with nz = 3 equidistant and nz=2 non-equidistant discretization
    // NOTE: this test is only sensitive to i_source >= i_target (in NonEquiDemagField::h() method)
    const float x=5.e-7, y=1.25e-7, z=3.e-9;
    const int nx = 10, ny=10, nz=3;

    af::randomEngine rand_engine = util::rand_engine_current_time();
    af::array random_1 = af::randu(af::dim4(nx, ny, 1, 3), f32, rand_engine);
    af::array random_2 = af::randu(af::dim4(nx, ny, 1, 3), f32, rand_engine);

    // equi
    Mesh mesh_ed(nx, ny, nz, x/nx, y/ny, z/nz);

    af::array m = af::constant(0.0, mesh_ed.n0, mesh_ed.n1, mesh_ed.n2, 3, f32);
    m(af::span, af::span, 0, af::span) = random_1;
    m(af::span, af::span, 1, af::span) = random_2;
    m(af::span, af::span, 2, af::span) = random_2;

    State state_ed(mesh_ed, 8e5, m, false, true);
    DemagField demag_ed = DemagField(mesh_ed, false, false, 1);

    // nonequi
    std::vector<float> z_spacing = {z/nz, 2 * z/nz};
    NonequispacedMesh mesh_ne(nx, ny, x/nx, y/ny, z_spacing);
    af::array m2 = af::constant(0.0, mesh_ne.dims, f32);
    m2(af::span, af::span, 0, af::span) = random_1;
    m2(af::span, af::span, 1, af::span) = random_2;

    //TODO this is not throwing an error but segfaults. No compiler warnign even though constructor should not exist: //State state_ne(mesh_ne, (af::array) m2, false, true);
    State state_ne(mesh_ne, af::constant(8e5, mesh_ne.dims, f32), m2, false, true);
    NonEquiDemagField demag_ne = NonEquiDemagField(mesh_ne, false, false, 1);

    // testing E(state)
    EXPECT_NEAR(demag_ed.E(state_ed), demag_ne.E(state_ne), 2e-24); //Note: this is for opencl, cpu and cuda achieve 3e-27

    // testing E(state, heff)
    af::array h_ed = demag_ed.h(state_ed);
    af::array h_ne = demag_ne.h(state_ne);
    EXPECT_NEAR(demag_ed.E(state_ed, h_ed), demag_ne.E(state_ne, h_ne), 2e-24);//Note: this is for opencl, cpu and cuda achieve 3e-27
}


TEST(NonEquiDemagField, RandomMagnetizationHeffTest) {
    // Compare SP4 layer with random z-magnetization once with nz = 3 equidistant and nz=2 non-equidistant discretization
    // NOTE: this test is only sensitive to i_source >= i_target (in NonEquiDemagField::h() method)
    const float x=5.e-7, y=1.25e-7, z=3.e-9;
    const int nx = 10, ny=10, nz=3;

    af::randomEngine rand_engine = util::rand_engine_current_time();
    af::array random_1 = af::randu(af::dim4(nx, ny, 1, 3), f32, rand_engine);
    af::array random_2 = af::randu(af::dim4(nx, ny, 1, 3), f32, rand_engine);

    // equi
    Mesh mesh_ed(nx, ny, nz, x/nx, y/ny, z/nz);

    af::array m = af::constant(0.0, mesh_ed.n0, mesh_ed.n1, mesh_ed.n2, 3, f32);
    m(af::span, af::span, 0, af::span) = random_1;
    m(af::span, af::span, 1, af::span) = random_2;
    m(af::span, af::span, 2, af::span) = random_2;

    State state_ed(mesh_ed,  8e5, m, false, true);
    DemagField demag_ed = DemagField(mesh_ed, false, false, 1);

    // nonequi
    std::vector<float> z_spacing = {z/nz, 2 * z/nz};
    NonequispacedMesh mesh_ne(nx, ny, x/nx, y/ny, z_spacing);
    af::array m2 = af::constant(0.0, mesh_ne.dims, f32);
    m2(af::span, af::span, 0, af::span) = random_1;
    m2(af::span, af::span, 1, af::span) = random_2;

    State state_ne(mesh_ne, af::constant(8e5, mesh_ne.dims, f32), m2, false, true);
    NonEquiDemagField demag_ne = NonEquiDemagField(mesh_ne, false, false, 1);

    af::array demag_ed_h = demag_ed.h(state_ed)(af::span, af::span, 0, af::span);
    af::array demag_ne_h = demag_ne.h(state_ne)(af::span, af::span, 0, af::span);
    //af::print("demag_ed_h", demag_ed_h);
    //af::print("demag_ne_h", demag_ne_h);
    EXPECT_NEAR( max_abs_diff(demag_ed_h, demag_ne_h), 0, 0.04); // Note: this is for opencl, cpu and cuda achieve 0.007
    EXPECT_NEAR(mean_abs_diff(demag_ed_h, demag_ne_h), 0, 0.009); // Note: this is for opencl, cpu and cuda achieve 
}


TEST(NonEquiDemagField, RandomMagnetizationSwappedZindexHeffTest) {
    // Same as above but testing else{} in nedemag.h(state) method
    // NOTE: this test is only sensitive to i_source < i_target (in NonEquiDemagField::h() method)
    const float x=5.e-7, y=1.25e-7, z=3.e-9;
    const int nx = 10, ny=10, nz=3;

    af::randomEngine rand_engine = util::rand_engine_current_time();
    af::array random_1 = af::randu(af::dim4(nx, ny, 1, 3), f32, rand_engine);
    af::array random_2 = af::randu(af::dim4(nx, ny, 1, 3), f32, rand_engine);

    // equi
    Mesh mesh_ed(nx, ny, nz, x/nx, y/ny, z/nz);

    af::array m = af::constant(0.0, mesh_ed.n0, mesh_ed.n1, mesh_ed.n2, 3, f32);
    m(af::span, af::span, 0, af::span) = random_2;
    m(af::span, af::span, 1, af::span) = random_2;
    m(af::span, af::span, 2, af::span) = random_1;

    State state_ed(mesh_ed, 8e5, m, false, true);
    DemagField demag_ed = DemagField(mesh_ed, false, false, 1);

    // nonequi
    std::vector<float> z_spacing = {2 * z/nz, z/nz};
    NonequispacedMesh mesh_ne(nx, ny, x/nx, y/ny, z_spacing);
    af::array m2 = af::constant(0.0, mesh_ne.dims, f32);
    m2(af::span, af::span, 0, af::span) = random_2;
    m2(af::span, af::span, 1, af::span) = random_1;

    State state_ne(mesh_ne, af::constant(8e5, mesh_ne.dims, f32), m2, false, true);
    NonEquiDemagField demag_ne = NonEquiDemagField(mesh_ne, false, false, 1);

    af::array demag_ed_h = demag_ed.h(state_ed)(af::span, af::span, 2, af::span);
    af::array demag_ne_h = demag_ne.h(state_ne)(af::span, af::span, 1, af::span);
    //af::print("demag_ed_h", demag_ed_h);
    //af::print("demag_ne_h", demag_ne_h);
    EXPECT_NEAR( max_abs_diff(demag_ed_h, demag_ne_h), 0, 0.04); // Note: this is for opencl, cpu and cuda achieve 0.007  
    EXPECT_NEAR(mean_abs_diff(demag_ed_h, demag_ne_h), 0, 0.009); // Note: this is for opencl, cpu and cuda achieve 0.0009 
}

TEST(NonEquiDemagField, RandomMagnetizationWithZeroLayerHeffTest) {
    // Compare SP4 layer with homogeneous z-magnetization once with nz = 3 equidistant and nz=2 non-equidistant discretization
    const float x=5.e-7, y=1.25e-7, z=3.e-9;
    const int nx = 100, ny=25, nz=3;

    af::randomEngine rand_engine = util::rand_engine_current_time();
    af::array random = af::randu(af::dim4(nx, ny, 1, 3), f32, rand_engine);

    // equi
    Mesh mesh_ed(nx, ny, nz, x/nx, y/ny, z/nz);

    af::array m = af::constant(0.0, mesh_ed.n0, mesh_ed.n1, mesh_ed.n2, 3, f32);
    m(af::span, af::span, 1, af::span) = random;
    m(af::span, af::span, 2, af::span) = random;

    State state_ed(mesh_ed, 8e5, m, false, true);//Note: this implicitly sets Ms_filed internally
    DemagField demag_ed = DemagField(mesh_ed, false, false, 1);

    // nonequi
    std::vector<float> z_spacing = {z/nz, 2 * z/nz};
    NonequispacedMesh mesh_ne(nx, ny, x/nx, y/ny, z_spacing);
    af::array m2 = af::constant(0.0, mesh_ne.dims, f32);
    m2(af::span, af::span, 1, af::span) = random;

    //af::array Ms_field = af::constant(8e5, mesh_ne.dims, f32);
    //std::cout << "test" << std::endl;
    //State state_ne(mesh_ne, Ms_field, m2, false, true);
    //std::cout << "test" << std::endl;
    State state_ne(mesh_ne, 8e5, m2, false, true);
    NonEquiDemagField demag_ne = NonEquiDemagField(mesh_ne, false, false, 1);

    af::array demag_ed_h = demag_ed.h(state_ed)(af::span, af::span, 0, af::span);
    af::array demag_ne_h = demag_ne.h(state_ne)(af::span, af::span, 0, af::span);
    EXPECT_NEAR( max_abs_diff(demag_ed_h, demag_ne_h), 0, 0.01);
    EXPECT_NEAR(mean_abs_diff(demag_ed_h, demag_ne_h), 0, 0.003);
}


TEST(NonEquiDemagField, UMagnetizationHeffTest) {
    // Compare SP4 layer (with ↑ → → → → ↑ magnetization) once with nz = 3 equidistant and nz=2 non-equidistant discretization
    const float x=5.e-7, y=1.25e-7, z=3.e-9;
    const int nx = 100, ny=25, nz=3;

    // equi
    Mesh mesh_ed(nx, ny, nz, x/nx, y/ny, z/nz);

    af::array m = af::constant(0.0, mesh_ed.n0, mesh_ed.n1, mesh_ed.n2, 3, f32);
    m(af::seq(1, af::end-1), af::span, af::span, 0) = af::constant(1.0, mesh_ed.n0-2, mesh_ed.n1, mesh_ed.n2, 1, f32);
    m(0, af::span, af::span, 1 ) = af::constant(1.0, 1, mesh_ed.n1, mesh_ed.n2, 1, f32);
    m(-1, af::span, af::span, 1) = af::constant(1.0, 1, mesh_ed.n1, mesh_ed.n2, 1, f32);
    m(af::span, af::span, 0, af::span) = 0;

    State state_ed(mesh_ed, 8e5, m, false, true);
    DemagField demag_ed = DemagField(mesh_ed, false, false, 1);

    // nonequi
    std::vector<float> z_spacing = {z/nz, 2 * z/nz};
    NonequispacedMesh mesh_ne(nx, ny, x/nx, y/ny, z_spacing);
    af::array m2 = af::constant(0.0, mesh_ne.dims, f32);
    m2(af::seq(1, af::end-1), af::span, af::span, 0) = af::constant(1.0, mesh_ne.nx-2, mesh_ne.ny, mesh_ne.nz, 1, f32);
    m2(0, af::span, af::span, 1 ) = af::constant(1.0, 1, mesh_ne.ny, mesh_ne.nz, 1, f32);
    m2(-1, af::span, af::span, 1) = af::constant(1.0, 1, mesh_ne.ny, mesh_ne.nz, 1, f32);
    m2(af::span, af::span, 0, af::span) = 0;

    State state_ne(mesh_ne, 8e5, m2, false, true);
    state_ne.Ms_field = af::constant(8e5, mesh_ne.dims, f32);
    NonEquiDemagField demag_ne = NonEquiDemagField(mesh_ne, false, false, 1);

    af::array demag_ed_h = demag_ed.h(state_ed)(af::span, af::span, 0, af::span);
    af::array demag_ne_h = demag_ne.h(state_ne)(af::span, af::span, 0, af::span);
    EXPECT_NEAR( max_abs_diff(demag_ed_h, demag_ne_h), 0, 0.01); // Note: this is for opencl, cpu and cuda achieve 0.007
    EXPECT_NEAR(mean_abs_diff(demag_ed_h, demag_ne_h), 0, 0.0008); // Note: this is for opencl, cpu and cuda achieve 0.0006
}


TEST(NonEquiDemag, HomogenuousMagnetizationHeffTest) {
    // Compare SP4 layer with homogeneous z-magnetization once with nz = 3 equidistant and nz=2 non-equidistant discretization
    const float x=5.e-7, y=1.25e-7, z=3.e-9;
    const int nx = 100, ny=25, nz=3;

    // equi
    Mesh mesh_ed(nx, ny, nz, x/nx, y/ny, z/nz);

    af::array m = af::constant(0.0, mesh_ed.n0, mesh_ed.n1, mesh_ed.n2, 3, f32);
    m(af::span, af::span, af::span, 2) = 1;

    State state_ed(mesh_ed, 8e5, m, false, true);
    DemagField demag_ed = DemagField(mesh_ed, false, false, 1);

    // nonequi
    std::vector<float> z_spacing = {z/nz, 2 * z/nz};
    NonequispacedMesh mesh_ne(nx, ny, x/nx, y/ny, z_spacing);
    af::array m2 = af::constant(0.0, mesh_ne.dims, f32);
    m2(af::span, af::span, af::span, 2) = 1;

    State state_ne(mesh_ne, 8e5, m2, false, true);
    state_ne.Ms_field = af::constant(8e5, mesh_ne.dims, f32);
    NonEquiDemagField demag_ne = NonEquiDemagField(mesh_ne, false, false, 1);

    af::array demag_ed_h = demag_ed.h(state_ed)(af::span, af::span, 0, af::span);
    af::array demag_ne_h = demag_ne.h(state_ne)(af::span, af::span, 0, af::span);
    EXPECT_NEAR( max_abs_diff(demag_ed_h, demag_ne_h), 0, 0.04); // Note: this is for opencl, cpu and cuda achieve 0.007
    EXPECT_NEAR(mean_abs_diff(demag_ed_h, demag_ne_h), 0, 0.01); // Note: this is for opencl, cpu and cuda achieve 0.0003
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
