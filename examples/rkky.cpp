// RKKY example from https://mumax.github.io/examples.html
#include "../src/magnum_af.hpp"
#include "arrayfire.h"
#include <cmath>

using namespace magnumafcpp;


int main(int argc, char** argv)
{
    // Checking input variables and setting GPU Device
    for (int i=0; i<argc; i++){std::cout << "Parameter " << i << " was " << argv[i] << std::endl;}
    std::string filepath(argc>1? argv[1]: "output_magnum.af/");
    af::setDevice(argc>2? std::stoi(argv[2]):0);
    af::info();

    // Parameter initialization
    const int nx = 10, ny=10 , nz=2;
    const double dx = 1e-9;

    const double Ms = 1e6;
    const double A = 10e-12;
    const double RKKY = -1e-3 * dx;

    //Generating Objects
    Mesh mesh(nx, ny, nz, dx, dx, dx);

    // Initial magnetic field
    af::array m = af::constant(0.0, mesh.dims, f64);
    m(af::span, af::span, af::span, 0) = 1.;
    State state(mesh, Ms, m);
    state.write_vti(filepath + "minit");
    af::array rkkyvals = af::constant(RKKY/2., mesh.dims, f64);
    af::array exchvals = af::constant(A, mesh.dims, f64);
    auto rkky = LlgTerm (new RKKYExchangeField(RKKY_values(rkkyvals), Exchange_values(exchvals),  mesh, true));

    auto demag = LlgTerm (new DemagField(mesh, true, true, 0));
    LLGIntegrator Llg(1, {demag, rkky});

    std::ofstream stream(filepath + "m.dat");
    stream.precision(12);

    std::ofstream streamE(filepath + "E.dat");
    streamE.precision(12);

    std::vector<double> vecE;

    for (int i = 0; i < 360; i ++){
        const double mix = std::cos(i * M_PI/180.);
        const double miy = std::sin(i * M_PI/180.);
        state.m(af::span, af::span, 1, 0) = mix;
        state.m(af::span, af::span, 1, 1) = miy;

        double E = Llg.E(state);
        vecE.push_back(E);
        std::cout << "i = " << i <<  ", E= " << E << std::endl;
        streamE   << i << "\t" << E << std::endl;
        state.calc_mean_m(stream);
    }
    stream.close();
    streamE.close();

    double maxval = *std::max_element(std::begin(vecE), std::end(vecE));
    double minval = *std::min_element(std::begin(vecE), std::end(vecE));

    std::cout << "maxval = " << maxval << std::endl;
    std::cout << "minval = " << minval << std::endl;
    std::cout << "Diff maxval minval  = " << maxval - minval << std::endl;

    return 0;
}