#include "arrayfire.h"
#include "magnum_af.hpp"
#include <filesystem>
#include <numeric>

using namespace magnumafcpp;

int main(int argc, char** argv) {
    // Checking input variables and setting GPU Device
    for (int i = 0; i < argc; i++) {
        std::cout << "Parameter " << i << " was " << argv[i] << std::endl;
    }
    std::string filepath(argc > 1 ? argv[1] : "output_magnum.af/");
    std::filesystem::create_directories(filepath);
    af::setDevice(argc > 2 ? std::stoi(argv[2]) : 0);
    af::setBackend(AF_BACKEND_CPU);
    const double hzee_max = argc > 3 ? std::stod(argv[3]) : 0.100; //[Tesla]

    af::info();

    // Parameter initialization
    // z1: Pinned Layer
    // z2: Reference Layer
    const int nx = 1, ny = 1, nz = 2;
    const double dx = 10e-9;

    // const double dx = 10e-9;
    auto _1D_field = af::dim4(nx, ny, nz, 1);

    const double RKKY_mJ_per_m2 = argc > 4 ? std::stod(argv[4]) : -0.8;
    const double RKKY = RKKY_mJ_per_m2 * 1e-3 * dx;
    std::cout << "RKKY=" << RKKY << std::endl;

    const double Ms1 = argc > 5 ? std::stod(argv[5]) : 0.37 / constants::mu0;
    std::cout << "Ms1=" << Ms1 << std::endl;
    const double Ms2 = 1e6;
    const double A = 15e-12; // Note: A is replaced by RKKY here

    // Anisotropy
    const double K1 = 0.2e6;
    std::cout << "K1=" << K1 << std::endl;
    // const double K2 = 0.2e6;
    // const double K1 = -1 / 2. * std::pow(Ms1, 2) * constants::mu0;
    // const double K2 = -1 / 2. * std::pow(Ms2, 2) * constants::mu0;
    // std::cout << "K1=" << K1 << std::endl;
    // std::cout << "K2=" << K2 << std::endl;
    const std::array<double, 3> Ku1_axis = {1, 0, 0}; // TODO check dir
    af::array Ku1_field = af::constant(0.0, _1D_field, f64);
    Ku1_field(af::span, af::span, 0) = K1;
    // Ku1_field(af::span, af::span, 1) = K2;
    auto aniso = LlgTerm(new UniaxialAnisotropyField(Ku1_field, Ku1_axis));

    // Generating Objects
    Mesh mesh(nx, ny, nz, dx, dx, dx);

    // Initial magnetic field
    af::array m = af::constant(0.0, mesh.dims, f64);
    m(af::span, af::span, 0, 0) = 1.;
    m(af::span, af::span, 1, 0) = -1.;
    af::array Ms_field = af::constant(0.0, _1D_field, f64);
    Ms_field(af::span, af::span, 0) = Ms1;
    Ms_field(af::span, af::span, 1) = Ms2;
    State state(mesh, Ms_field, m);
    state.write_vti(filepath + "minit");

    auto rkky = LlgTerm(new RKKYExchangeField(
        RKKY_values(af::constant(RKKY, mesh.dims, f64)),
        Exchange_values(af::constant(A, mesh.dims, f64)), mesh));

    auto demag = LlgTerm(new DemagField(mesh, true, true, 0));

    unsigned current_step = 0;
    // Defining H_zee via lamdas
    auto zee_func = [&current_step, hzee_max](State state) -> af::array {
        const double hx =
            hzee_max / constants::mu0 * std::cos(current_step * M_PI / 180.);
        const double hy =
            hzee_max / constants::mu0 * std::sin(current_step * M_PI / 180.);

        af::array zee = af::constant(0.0, state.mesh.dims, f64);
        zee(af::span, af::span, af::span, 0) = hx;
        zee(af::span, af::span, af::span, 1) = hy;
        return zee;
    };
    auto external = LlgTerm(new ExternalField(zee_func));
    LLGIntegrator llg(1, {demag, rkky, aniso, external});
    // LLGIntegrator llg(1, {demag, rkky, external, aniso});

    std::ofstream stream(filepath + "m.dat");
    stream.precision(12);

    std::vector<double> abs_my_rl; // Ref Layer my list

    // for (unsigned i = 0; i < 360; i++) {
    for (unsigned i = 0; i <= 360; i += 20) {
        current_step = i;
        llg.relax(state);
        // const double Hx_component =
        //    llg.llgterms[2]->h(state)(0, 0, 1, 0).scalar<double>() *
        //    constants::mu0;
        const double Hx_component =
            external->h(state)(0, 0, 1, 0).scalar<double>() * constants::mu0;
        const double my_z0 = state.m(0, 0, 0, 1).scalar<double>();
        const double my_z1 = state.m(0, 0, 1, 1).scalar<double>();
        abs_my_rl.push_back(std::abs(my_z1));

        std::cout << i << "\t" << Hx_component << "\t" << my_z0 << "\t" << my_z1
                  << std::endl;
        stream << i << "\t" << Hx_component << "\t" << my_z0 << "\t" << my_z1
               << std::endl;
    }
    stream.close();

    // for (auto it : abs_my_rl) {
    //    std::cout << it << std::endl;
    //}
    double sum = std::accumulate(abs_my_rl.begin(), abs_my_rl.end(), 0.0);
    std::cout << "sum=" << sum << std::endl;
    double mean = sum / abs_my_rl.size();
    std::cout << "mean=" << mean << std::endl;
    double max = *std::max_element(abs_my_rl.begin(), abs_my_rl.end());
    std::cout << "max=" << max << std::endl;

    stream.open(filepath + "table.dat");
    stream << "# dx <<  Ms1[J/T/m3] << RKKY[mJ/m2] << max(abs(my)) << "
              "mean(abs(my))"
           << std::endl;
    stream << dx << "\t" << Ms1 << "\t" << RKKY_mJ_per_m2 << "\t" << max << "\t"
           << mean << "\t" << std::endl;
    stream.close();

    stream.open(filepath + "plotfile.gpi");
    stream << "set terminal pdf;" << std::endl;
    stream << "set xlabel 'H_x [T]'" << std::endl;
    stream << "set ylabel 'm_y'" << std::endl;
    stream << "set output 'saf_angle_error.pdf'" << std::endl;
    stream << "p 'm.dat' u 2:3 w l title 'Pinned'";
    stream << ", '' u 2:4 w l title 'Reference'" << std::endl << std::endl;
    stream << "set terminal jpeg" << std::endl;
    stream << "set output 'saf_angle_error.jpg'" << std::endl;
    stream << "replot" << std::endl;
    stream.close();

    int syscall =
        std::system(("cd " + filepath + " && gnuplot plotfile.gpi").c_str());
    if (syscall != 0) {
        std::cout << "syscall plotting with gnuplot failed" << std::endl;
    }

    return 0;
}