#include "arrayfire.h"
#include "magnum_af.hpp"

using namespace magnumaf;


int main(int argc, char** argv)
{
    // Checking input variables and setting GPU Device
    af::timer total_time = af::timer::start();
    for (int i=0; i<argc; i++){std::cout << "Parameter " << i << " was " << argv[i] << std::endl;}
    std::string filepath(argc>1? argv[1]: "output_magnum.af/");
    af::setDevice(argc>2? std::stoi(argv[2]):0);
    af::info();

    // Parameter initialization
    const double x=5.e-7, y=1.25e-7, z=3.e-9;
    const int nx = 100, ny=25 , nz=1;
    const double A = 1.3e-11;

    //Generating Objects
    Mesh mesh(nx, ny, nz, x/nx, y/ny, z/nz);

    // Initial magnetic field
    State state(mesh, 8e5, mesh.init_sp4());
    state.write_vti(filepath + "minit");

    LlgTerms llgterm;
    llgterm.push_back( LlgTerm (new DemagField(mesh, true, true, 0)));
    llgterm.push_back( LlgTerm (new ExchangeField(A)));
    LLGIntegrator Llg(1, llgterm);

    std::ofstream stream;
    stream.precision(12);
    stream.open(filepath + "m.dat");

    // Relax
    af::timer t = af::timer::start();
    while (state.t < 1e-9){
        Llg.step(state);
        state.calc_mean_m(stream);
    }
    std::cout<<"relax     [s]: "<< af::timer::stop(t) <<std::endl;
    state.write_vti(filepath + "relax");

    // Prepare switch
    af::array zeeswitch = af::constant(0.0, nx, ny, nz, 3, f64);
    zeeswitch(af::span, af::span, af::span, 0) = -24.6e-3/constants::mu0;
    zeeswitch(af::span, af::span, af::span, 1) = +4.3e-3/constants::mu0;
    zeeswitch(af::span, af::span, af::span, 2) = 0.0;
    Llg.llgterms.push_back( LlgTerm (new ExternalField(zeeswitch)));
    Llg.alpha = 0.02;

    // Switch
    t = af::timer::start();
    while (state.t < 2e-9){
        Llg.step(state);
        state.calc_mean_m(stream);
    }
    std::cout<<"integrate [s]: "<< af::timer::stop(t) <<std::endl;
    state.write_vti(filepath + "2ns");
    stream.close();
    std::cout<<"total     [s]: "<< af::timer::stop(total_time) <<std::endl;
    return 0;
}
