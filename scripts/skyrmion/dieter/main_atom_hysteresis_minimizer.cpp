#include "arrayfire.h"
#include "magnum_af.hpp"

using namespace af; 
typedef std::shared_ptr<LLGTerm> llgt_ptr; 

void calc_mean_m(const State& state, std::ostream& myfile, double hzee){
    array sum_dim3 = sum(sum(sum(state.m,0),1),2);
    myfile << std::setw(12) << state.t << "\t" << afvalue(sum_dim3(span,span,span,0)) << "\t" << afvalue(sum_dim3(span,span,span,1))<< "\t" << afvalue(sum_dim3(span,span,span,2)) << "\t" << hzee << std::endl;
}

double hzee_max = 2.; //[T]
int quater_steps=100; // One 4th of total steps

af::array zee_func(State state){
    double field_Tesla = 0;
    double rate = hzee_max/quater_steps; //[T/s]
    if(state.t < hzee_max/rate) field_Tesla = rate *state.t; 
    else if (state.t < 3*hzee_max/rate) field_Tesla = -rate *state.t + 2*hzee_max; 
    else if(state.t < 4*hzee_max/rate) field_Tesla = rate*state.t - 4*hzee_max; 
    else {field_Tesla = 0; std::cout << "WARNING ZEE time out of range" << std::endl;}
    array zee = constant(0.0,state.mesh.n0,state.mesh.n1,state.mesh.n2,3,f64);
    zee(span,span,span,0)=constant(field_Tesla/state.param.mu0 ,state.mesh.n0,state.mesh.n1,state.mesh.n2,1,f64);
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

    // Relax
    af::timer timer_llgterms = af::timer::start();
    Minimizer minimizer("BB", 1e-10, 1e-5, 1e4, 100);
    //minimizer.llgterms.push_back( LlgTerm (new ATOMISTIC_DEMAG(mesh)));
    minimizer.llgterms.push_back( LlgTerm (new ATOMISTIC_EXCHANGE(mesh)));
    minimizer.llgterms.push_back( LlgTerm (new ATOMISTIC_DMI(mesh,param)));
    minimizer.llgterms.push_back( LlgTerm (new ATOMISTIC_ANISOTROPY(mesh,param)));
    std::cout<<"Llgterms assembled in "<< af::timer::stop(timer_llgterms) <<std::endl;

    //obtaining relaxed magnetization
    timer t = af::timer::start();
    minimizer.minimize(state);
    std::cout<<"timerelax [af-s]: "<< af::timer::stop(t) <<std::endl;
    vti_writer_micro(state.m, mesh ,(filepath + "relax").c_str());

    // Hysteresis
    std::ofstream stream;
    stream.precision(12);
    stream.open ((filepath + "m.dat").c_str());
    stream << "# t	<mx>    <my>    <mz>    hzee" << std::endl;
    calc_mean_m(state, stream, 0);

    timer t_hys = af::timer::start();
    double rate = hzee_max/quater_steps; //[T/s]
    minimizer.llgterms.push_back( LlgTerm (new Zee(&zee_func)));
    while (state.t < 4* hzee_max/rate){
        minimizer.minimize(state);
        calc_mean_m(state, stream, afvalue(minimizer.llgterms[3]->h(state)(0,0,0,0)));
        state.t+=1.;
        state.steps++;
        if( state.steps % 1 == 0){
            vti_writer_micro(state.m, mesh ,(filepath + "m_hysteresis_"+std::to_string(state.steps)).c_str());
        }
    }
    std::cout<<"time full hysteresis [af-s]: "<< af::timer::stop(t_hys) <<std::endl;
    return 0;
}