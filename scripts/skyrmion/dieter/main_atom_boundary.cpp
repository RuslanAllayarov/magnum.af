#include "arrayfire.h"
#include "magnum_af.hpp"

using namespace magnumafcpp;

using namespace af;

int main(int argc, char** argv) {
    std::string filepath(argc >= 1 ? argv[1] : "data");
    if (argc >= 1) {
        filepath.append("/");
    }
    if (argc >= 2) {
        setDevice(std::stoi(argv[2]));
    }
    jtd::cout << "Writing into path " << filepath.c_str() << std::endl;
    std::string path_mrelax(argc > 3 ? argv[3] : "");
    info();

    // Parameter initialization
    double length = 90e-9; //[nm]
    const double dx = 0.5e-9;
    const int nx = (int)(length / dx);
    std::cout << "nx = " << nx << std::endl;

    double n_interp = 60;
    double string_dt = 1e-13;

    // Generating Objects
    Mesh mesh(nx, nx, 1, dx, dx, dx);
    Material material = Material();
    state.Ms = 580000;
    material.A = 15e-12;
    material.alpha = 1;
    material.D = 3e-3;
    material.Ku1 = 0.6e6;

    material.set_atomistic_from_micromagnetic(mesh.dx);

    State state(mesh, material, util::skyrmconf(mesh));
    vti_writer_micro(state.m, mesh, (filepath + "minit").c_str());

    LLGIntegrator llg;
    llg.llgterms.push_back(uptr_FieldTerm(new AtomisticDipoleDipoleField(mesh)));
    llg.llgterms.push_back(uptr_FieldTerm(new AtomisticExchangeField(mesh)));
    llg.llgterms.push_back(uptr_FieldTerm(new AtomisticDmiField(mesh, material)));
    llg.llgterms.push_back(uptr_FieldTerm(new AtomisticUniaxialAnisotropyField(mesh, material)));

    if (!exists(path_mrelax)) {
        std::cout << "mrelax.vti not found, starting relaxation" << std::endl;
        llg.relax(state);
        vti_writer_micro(state.m, mesh, filepath + "relax");
        state.t = 0;
    } else {
        std::cout << "found mrelax. loading magnetization" << std::endl;
        vti_reader(state.m, state.mesh, path_mrelax);
    }

    array last = constant(0, mesh::dims_v(mesh), f64);
    last(span, span, span, 2) = 1;

    std::vector<State> inputimages;

    // Direct x-boundary
    for (int j = 0; j < n_interp; j++) {
        int i = (int)((mesh.nx / 2) / n_interp + 1) * j;
        if (i > mesh.nx)
            i = mesh.nx;
        std::cout << "i= " << i << std::endl;
        array mm = array(state.m);
        mm = shift(mm, i);
        mm(seq(0, i), span, span, span) = 0;
        mm(seq(0, i), span, span, 2) = 1.;
        inputimages.push_back(State(mesh, material, mm));
    }

    StringMethod string(state, inputimages, n_interp, string_dt, llg.llgterms);
    string.run(filepath);
    return 0;
}
