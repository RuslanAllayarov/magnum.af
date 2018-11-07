#include "arrayfire.h"
#include "magnum_af.hpp"

using namespace af; 
typedef std::shared_ptr<LLGTerm> llgt_ptr; 

void calc_mean_m(const State& state, std::ostream& myfile, double hzee){
    const array sum_dim3 = sum(sum(sum(state.m,0),1),2);
    const int ncells = state.mesh.n0 * state.mesh.n1 * state.mesh.n2;
    myfile << std::setw(12) << state.t << "\t" << afvalue(sum_dim3(span,span,span,0))/ncells << "\t" << afvalue(sum_dim3(span,span,span,1))/ncells<< "\t" << afvalue(sum_dim3(span,span,span,2))/ncells << "\t" << hzee << std::endl;
}

const double hzee_max = 2; //[T]
const double simtime = 100e-9;
const double rate = hzee_max/simtime; //[T/s]

af::array zee_func(State state){
    double field_Tesla = 0;
    field_Tesla = rate *state.t; 
    array zee = constant(0.0,state.mesh.n0,state.mesh.n1,state.mesh.n2,3,f64);
    zee(span,span,span,2)=constant(field_Tesla/state.param.mu0 ,state.mesh.n0,state.mesh.n1,state.mesh.n2,1,f64);
    return  zee;
}

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
    const double dx=0.9e-9;
    const int nx = (int)(length/dx);
    std::cout << "nx = "<< nx << std::endl;
  
    //Generating Objects
    Mesh mesh(nx,nx,1,dx,dx,dx);
    Param param = Param();
    param.ms    = 580000;
    param.alpha = 1;
    param.A     = 15e-12;
    //param.D=3e-3;
    //param.Ku1=0.6e6;
  
    param.p=param.ms*pow(dx,3);//Compensate nz=1 instead of nz=4
    param.J_atom=2.*param.A*dx;
    //param.D_atom= param.D * pow(dx,2);
    //param.K_atom=param.Ku1*pow(dx,3);
  
     // Initial magnetic field
     array m = constant(0.0,mesh.n0,mesh.n1,mesh.n2,3,f64);
     m(span,span,span,0) = 1.;

    State state(mesh,param, m);
    vti_writer_atom(state.m, mesh ,(filepath + "minit").c_str());
  
    std::vector<llgt_ptr> llgterm;
    llgterm.push_back( llgt_ptr (new ATOMISTIC_DEMAG(mesh)));
    llgterm.push_back( llgt_ptr (new ATOMISTIC_EXCHANGE(mesh)));
    
    LLG Llg(state,llgterm);
  
    timer t = af::timer::start();
    double E_prev=1e20;
    while (fabs((E_prev-Llg.E(state))/E_prev) > 1e-8){
        E_prev=Llg.E(state);
        for ( int i = 0; i<100; i++){
            state.m=Llg.llgstep(state);
        }
        if( state.steps % 1000 == 0) std::cout << "step " << state.steps << " rdiff= " << fabs((E_prev-Llg.E(state))/E_prev) << std::endl;
    }
    std::cout << "time =" << state.t << " [s], E = " << Llg.E(state) << "[J]" << std::endl;
    std::cout<<"timerelax [af-s]: "<< af::timer::stop(t) << ", steps = " << state.steps << std::endl; 
    vti_writer_atom(state.m, mesh ,(filepath + "relax").c_str());
  
    // Hysteresis
    std::ofstream stream;
    stream.precision(12);
    stream.open ((filepath + "m.dat").c_str());
    stream << "# t	<mx>    <my>    <mz>    hzee" << std::endl;
    calc_mean_m(state, stream, 0);

    timer t_hys = af::timer::start();
    Llg.Fieldterms.push_back( llgt_ptr (new Zee(&zee_func))); //Rate in T/s
    while (state.t <  simtime){
         state.m=Llg.llgstep(state);
         if( state.steps % 1000 == 0){
             calc_mean_m(state, stream, afvalue(Llg.Fieldterms[Llg.Fieldterms.size()-1]->h(state)(0,0,0,2)));
             vti_writer_atom(state.m, mesh ,(filepath + "m_hysteresis_"+std::to_string(state.steps)).c_str());
         }
    }
    stream.close();
    std::cout<<"time full hysteresis [af-s]: "<< af::timer::stop(t_hys) <<std::endl;
    return 0;
}