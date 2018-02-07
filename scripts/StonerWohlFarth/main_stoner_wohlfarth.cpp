#include <iostream>
#include <complex>
#include "arrayfire.h"
#include "pth_mag.hpp"
using namespace af;
typedef std::shared_ptr<LLGTerm> llgt_ptr;

void calcm(State state, std::ostream& myfile){
    myfile << std::setw(12) << state.t << "\t" <<meani(state.m,0)<< "\t" <<meani(state.m,1)<< "\t" <<meani(state.m,2)<< std::endl;
}

int main(int argc, char** argv)
{
    std::cout<<"argc"<<argc<<std::endl;
    for (int i=0; i<argc; i++){cout << "Parameter " << i << " was " << argv[i] << "\n";}
    std::string filepath(argc>1? argv[1]: "../Data/Testing");
    if(argc>0)filepath.append("/");
    std::cout<<"Writing into path "<<filepath.c_str()<<std::endl;
    std::ofstream stream;
    stream.precision(12);
    stream.open ((filepath + "m.dat").c_str());
    setDevice(argc>2? std::stoi(argv[2]):0);
    info();
  
    // Parameter initialization
    const double x=2.e-9, y=2.e-9, z=2.e-9;
    const int nx = 1, ny=1 ,nz=1;
    const double dt = 1e-14;
  
    //Generating Objects
    Mesh mesh(nx,ny,nz,x/nx,y/ny,z/nz);
    Param param = Param();
    param.ms    = 1/param.mu0;
    param.alpha = 0.008;
    param.T = 1;
  
    // Initial magnetic field
    array m = constant(0.0,mesh.n0,mesh.n1,mesh.n2,3,f64);
    m(0,0,0,2)=1.;
    std::vector<llgt_ptr> llgterm;
    array zeeswitch = constant(0.0,1,1,1,3,f64);
    zeeswitch(0,0,0,2)=1./param.mu0;
    llgterm.push_back( llgt_ptr (new Zee(zeeswitch,mesh,param)));
    State state(mesh, param, m);
    Stochastic_LLG Stoch(state, llgterm, dt, "Heun");
  
    while (state.t < 100e-9){
        Stoch.step(state, dt); 
        calcm(state,stream);
    }
    return 0;
}