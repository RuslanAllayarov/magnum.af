#include "arrayfire.h"
#include "magnum_af.hpp"

using namespace magnumafcpp;

using namespace af;

int main(int argc, char **argv)
{

    std::cout << "argc = " << argc << std::endl;
    for (int i = 0; i < argc; i++)
        std::cout << "Parameter " << i << " was " << argv[i] << "\n";

    std::string filepath(argc > 1 ? argv[1] : "../Data/skyrmion_stoch");
    if (argc > 0)
        filepath.append("/");
    std::cout << "Writing into path " << filepath.c_str() << std::endl;

    setDevice(argc > 2 ? std::stoi(argv[2]) : 0);
    //if(argc>1) setDevice(std::stoi(argv[2]));
    info();

    // Parameter initialization
    const int nx = 30, ny = 30, nz = 1;
    const double dx = 1e-9;

    double n_interp = 60;
    double string_dt = 1e-13;

    //Generating Objects
    Mesh mesh(nx, ny, nz, dx, dx, dx);
    const double Ms = 1.1e6;
    const double A = 1.6e-11;
    const double alpha = 1;
    const double D = 2 * 5.76e-3;
    const double Ku1 = 6.4e6;

    const double J_atom = 2. * A * dx;
    const double D_atom = D * pow(dx, 2);
    const double K_atom = Ku1 * pow(dx, 3);
    const double p = Ms * pow(dx, 3); //Compensate nz=1 instead of nz=4

    double bz_in_dims_of_J_atom(argc > 3 ? std::stod(argv[3]) : 0.);
    std::cout << "bz_in_dims_of_J_atom = " << bz_in_dims_of_J_atom << std::endl;
    const int demag(argc > 4 ? std::stoi(argv[4]) : 0); // 0 == false, else true

    // Initial magnetic field
    array m = constant(0.0, mesh.n0, mesh.n1, mesh.n2, 3, f64);
    m(span, span, span, 2) = -1;
    for (uint32_t ix = 0; ix < mesh.n0; ix++)
    {
        for (uint32_t iy = 0; iy < mesh.n1; iy++)
        {
            const double rx = double(ix) - mesh.n0 / 2.;
            const double ry = double(iy) - mesh.n1 / 2.;
            const double r = sqrt(pow(rx, 2) + pow(ry, 2));
            if (r > nx / 4.)
                m(ix, iy, span, 2) = 1.;
        }
    }

    State state(mesh, p, m);
    vti_writer_atom(state.m, mesh, (filepath + "minit").c_str());

    array zee = constant(0.0, 1, 1, 1, 3, f64);
    zee(0, 0, 0, 2) = bz_in_dims_of_J_atom * J_atom / (p * constants::mu0);
    af::print("zee_pre_tile", zee);
    zee = tile(zee, mesh.n0, mesh.n1, mesh.n2);

    LLGIntegrator Llg(alpha);
    if (demag)
    {
        Llg.llgterms.push_back(LlgTerm(new AtomisticDipoleDipoleField(mesh)));
    }
    Llg.llgterms.push_back(LlgTerm(new AtomisticExchangeField(J_atom)));
    Llg.llgterms.push_back(LlgTerm(new AtomisticDmiField(D_atom, {0, 0, -1})));
    Llg.llgterms.push_back(LlgTerm(new AtomisticUniaxialAnisotropyField(K_atom)));
    Llg.llgterms.push_back(LlgTerm(new AtomisticExternalField(zee)));

    Llg.relax(state);
    vti_writer_micro(state.m, mesh, filepath + "relax");
    state.t = 0;

    array last = constant(0, mesh.dims, f64);
    last(span, span, span, 2) = 1;

    std::vector<State> inputimages;
    inputimages.push_back(state);
    inputimages.push_back(State(mesh, p, last));

    String string(state, inputimages, n_interp, string_dt, Llg);
    double barrier = string.run(filepath);
    std::ofstream myfileE;
    myfileE.precision(12);
    myfileE.open((filepath + "bz_over_J.dat").c_str());
    myfileE << bz_in_dims_of_J_atom << "\t" << barrier << std::endl;
    myfileE.close();
    return 0;
}
