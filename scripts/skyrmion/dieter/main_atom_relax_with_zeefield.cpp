#include "arrayfire.h"
#include "magnum_af.hpp"

using namespace af; 
typedef std::shared_ptr<LLGTerm> llgt_ptr; 

int main(int argc, char** argv)
{

    std::cout<<"argc = "<<argc<<std::endl;
     for (int i=0; i<argc; i++)
          cout << "Parameter " << i << " was " << argv[i] << "\n";
    
    std::string filepath(argc>1? argv[1]: "../Data/skyrmion_stoch");
    if(argc>0)filepath.append("/");
    std::cout<<"Writing into path "<<filepath.c_str()<<std::endl;

    setDevice(argc>2? std::stoi(argv[2]):0);
    info();

    // Parameter initialization
    double length = 90e-9; //[nm]
    const double dx=0.5e-9;
    const int nx = (int)(length/dx);
    std::cout << "nx = "<< nx << std::endl;
  
    //Generating Objects
    Mesh mesh(nx,nx,1,dx,dx,dx);
    Param param = Param();
    param.ms    = 580000;
    param.A     = 15e-12;
    param.alpha = 1;
    param.D=3e-3;
    param.Ku1=0.6e6;
  
    param.J_atom=2.*param.A*dx;
    param.D_atom= param.D * pow(dx,2);
    param.K_atom=param.Ku1*pow(dx,3);
    param.p=param.ms*pow(dx,3);//Compensate nz=1 instead of nz=4
  
     // Initial magnetic field
     array m = constant(0.0,mesh.n0,mesh.n1,mesh.n2,3,f64);
     m(span,span,span,2) = -1;
     for(int ix=0;ix<mesh.n0;ix++){
         for(int iy=0;iy<mesh.n1;iy++){
             const double rx=double(ix)-mesh.n0/2.;
             const double ry=double(iy)-mesh.n1/2.;
             const double r = sqrt(pow(rx,2)+pow(ry,2));
             if(r>nx/4.) m(ix,iy,span,2)=1.;
         }
     }
  
    State state(mesh,param, m);
    vti_writer_atom(state.m, mesh ,(filepath + "minit").c_str());
  
    std::vector<llgt_ptr> llgterm;
    llgterm.push_back( llgt_ptr (new ATOMISTIC_DEMAG(mesh)));
    llgterm.push_back( llgt_ptr (new ATOMISTIC_EXCHANGE(mesh)));
    llgterm.push_back( llgt_ptr (new ATOMISTIC_DMI(mesh,param)));
    llgterm.push_back( llgt_ptr (new ATOMISTIC_ANISOTROPY(mesh,param)));

    array zeeswitch = constant(0.0,1,1,1,3,f64);
    zeeswitch(0,0,0,2)= - 0.07 * pow(param.ms,2) * param.mu0;
    zeeswitch = tile(zeeswitch,mesh.n0,mesh.n1,mesh.n2);
    llgterm.push_back( llgt_ptr (new Zee(zeeswitch)));
    
    LLG Llg(state,llgterm);
    Llg.write_fieldterms_micro(state, filepath + "init_field_micro_");
    Llg.write_fieldterms_atom (state, filepath + "init_field_atom_");

    timer t = af::timer::start();
    double E_prev=1e20;
    while (fabs((E_prev-Llg.E(state))/E_prev) > 1e-10){
        E_prev=Llg.E(state);
        for ( int i = 0; i < 100; i++){
            state.m=Llg.llgstep(state);
        }
        if( state.steps % 1000 == 0) std::cout << "step " << state.steps << "reldiff= " << fabs((E_prev-Llg.E(state))/E_prev) << std::endl;
    }
    double timerelax= af::timer::stop(t);
    vti_writer_atom(state.m, mesh ,filepath + "relax");
    Llg.write_fieldterms_micro(state, filepath);
    Llg.write_fieldterms_micro(state, filepath + "field_micro_");
    Llg.write_fieldterms_atom (state, filepath + "field_atom_");
    //vti_writer_atom(Llg.Fieldterms[0]->h(state), mesh ,filepath + "aDemag");
    //vti_writer_atom(Llg.Fieldterms[1]->h(state), mesh ,filepath + "aExch");
    //vti_writer_atom(Llg.Fieldterms[2]->h(state), mesh ,filepath + "aDMI");
    //vti_writer_atom(Llg.Fieldterms[3]->h(state), mesh ,filepath + "aAni");
    //vti_writer_atom(Llg.Fieldterms[4]->h(state), mesh ,filepath + "aZee");
  
    std::cout<<"timerelax [af-s]: "<< timerelax << " for "<<Llg.counter_accepted+Llg.counter_reject<<" steps, thereof "<< Llg.counter_accepted << " Steps accepted, "<< Llg.counter_reject<< " Steps rejected" << std::endl;
    return 0;
}