#include "arrayfire.h"
#include "magnum_af.hpp"

using namespace magnumaf;

using namespace af;

int main(int argc, char** argv)
{

    std::cout<<"argc = "<<argc<<std::endl;
     for (int i=0; i<argc; i++)
          std::cout << "Parameter " << i << " was " << argv[i] << "\n";

    std::string filepath(argc>1? argv[1]: "./run/");
    if(argc>1)filepath.append("/");
    std::cout<<"Writing into path "<<filepath.c_str()<<std::endl;

    setDevice(argc>2? std::stoi(argv[2]):0);
    info();

    // Parameter initialization
    const float x=400e-9;
    const float y=400e-9;
    const float z=3e-9;

    const int nx = 128, ny=128 , nz=1;
    const float dx= x/nx;
    const float dy= y/ny;
    const float dz= z/nz;

    // SK layer params
    const float Ms =1371e3;// A/m
    const float A = 15e-12;// J/m
    const float Ku = 1.411e6;// J/m^3
    const float D =2.5e-3;// J/m^2
    const float Hz = 130e-3/constants::mu0;


    //Generating Objects
    Mesh mesh(nx, ny, nz, dx, dy, dz);

    // Initial magnetic field
    array m = constant(0.0, mesh.n0, mesh.n1, mesh.n2, 3, f32);
    m(af::span, af::span, af::span, 2) = -1;
    for(int ix=0;ix<mesh.n0;ix++){
        for(int iy=0;iy<mesh.n1;iy++){
            const float rx=float(ix)-mesh.n0/2.;
            const float ry=float(iy)-mesh.n1/2.;
            const float r = sqrt(pow(rx, 2)+pow(ry, 2));
            if(r>nx/4.) m(ix, iy, af::span, 2)=1.;
        }
    }

    State state(mesh, Ms, m);
    state.write_vti(filepath + "minit");

    // defining interactions
    auto demag = LlgTerm (new DemagField(mesh, true, true, 0));
    auto exch = LlgTerm (new ExchangeField(A));
    auto aniso = LlgTerm (new UniaxialAnisotropyField(Ku, {0, 0, 1}));

    Material material = Material();
    material.D=D;
    material.D_axis[2]= -1;
    auto dmi = LlgTerm (new DmiField(mesh, material));

    array zee = constant(0.0, mesh.n0, mesh.n1, mesh.n2, 3, f32);
    zee(af::span, af::span, af::span, 2) = Hz;
    auto external = LlgTerm (new ExternalField(zee));

    af::print("dmi", dmi->h(state));

    LLGIntegrator Llg(1, {demag, exch, aniso, dmi, external});
    //LLGIntegrator Llg(1, {demag, exch, aniso, dmi, external});
    while (state.t < 3e-9){
        if (state.steps % 100 == 0) state.write_vti(filepath + "m_step" + std::to_string(state.steps));
        Llg.step(state);
        std::cout << state.steps << "\t" << state.t << "\t" <<  state.meani(2) << "\t" << Llg.E(state) << std::endl;
    }
//    Llg.relax(state);
    state.write_vti(filepath + "m_relaxed");

    // preparing string method
//    float n_interp = 60;
//    float string_dt=1e-13;
//    const int string_steps = 10000;

//    array last   = constant( 0, mesh.dims, f32);
//    last(span, span, span, 2)=1;
//
//    std::vector<State> inputimages;
//    inputimages.push_back(state);
//    inputimages.push_back(State(mesh, material, last));
//
//    String string(state, inputimages, n_interp, string_dt , Llg.llgterms);
//    string.run(filepath);
    return 0;
}