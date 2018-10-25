#include "arrayfire.h"
#include "magnum_af.hpp"

double hzee_max = 0.12; //[T]
int quater_steps=250; // One 4th of total steps

af::array zee_func(State state){
    double field_Tesla = 0;
    double rate = hzee_max/quater_steps; //[T/s]
    if(state.t < hzee_max/rate) field_Tesla = rate *state.t; 
    else if (state.t < 3*hzee_max/rate) field_Tesla = -rate *state.t + 2*hzee_max; 
    else if(state.t < 4*hzee_max/rate) field_Tesla = rate*state.t - 4*hzee_max; 
    else {field_Tesla = 0; std::cout << "WARNING ZEE time out of range" << std::endl;}
    array zee = constant(0.0,state.mesh.n0,state.mesh.n1,state.mesh.n2,3,f64);
    zee(span,span,span,2)=constant(field_Tesla/state.param.mu0 ,state.mesh.n0,state.mesh.n1,state.mesh.n2,1,f64);
    return  zee;
}
  
int main(int argc, char** argv)
{
    std::cout<<"argc"<<argc<<std::endl;
    for (int i=0; i<argc; i++) cout << "Parameter " << i << " was " << argv[i] << "\n";
    std::string filepath(argc>1? argv[1]: "../Data/Testing");
    if(argc>1)filepath.append("/");
    std::cout<<"Writing into path "<<filepath.c_str()<<std::endl;
    setDevice(argc>2? std::stoi(argv[2]):0);
    std::string path_mrelax(argc>3? argv[3]: "");
    info();
    std::cout.precision(24);

    // Parameter initialization
    Param param = Param();
    param.ms    = 2./param.mu0;//[J/T/m^3] == [Joule/Tesla/meter^3] = 1.75 T/mu_0
    param.A     = 1.5e-11;//[J/m]
    param.Ku1 = 1.4e6;
    param.alpha = 0.02;

    const double x=1000e-9, y=6000e-9, z=5e-9;//[m] // Physical dimensions
    const int nx = 343;
    const int ny = 1920;
    const int nz = 2;
  
    //Generating Objects
    Mesh mesh(nx,ny,nz,x/nx,y/ny,z/nz);

    long int n_cells=0;//Number of cells with Ms!=0
    State state(mesh,param, mesh.ellipse(n_cells));
    state.calc_mean_m(std::cout, n_cells);
    vti_writer_micro(state.m, mesh ,(filepath + "minit_nonnormalized").c_str());
    vti_writer_micro(state.Ms, mesh ,(filepath + "Ms").c_str());
    vti_writer_micro(state.m, mesh ,(filepath + "minit").c_str());
    mesh.print(std::cout);

    af::timer timer_llgterms = af::timer::start();
    Minimizer minimizer("BB", 1e-10, 1e-5, 1e4, 100);
    minimizer.llgterms.push_back( LlgTerm (new DemagSolver(mesh,param)));
    minimizer.llgterms.push_back( LlgTerm (new ExchSolver(mesh,param)));
    minimizer.llgterms.push_back( LlgTerm (new ANISOTROPY(mesh,param)));
    std::cout<<"Llgterms assembled in "<< af::timer::stop(timer_llgterms) <<std::endl;

    // Relaxation
    if(!exists (path_mrelax)){
        timer t = af::timer::start();
        minimizer.minimize(state);
        std::cout<<"timerelax [af-s]: "<< af::timer::stop(t) <<std::endl;
        vti_writer_micro(state.m, mesh ,(filepath + "mrelax").c_str());
    }
    else{
        std::cout << "found mrelax. loading magnetization" << std::endl;
        vti_reader(state.m, state.mesh, path_mrelax);
    }

    std::ofstream stream;
    stream.precision(12);
    stream.open ((filepath + "m.dat").c_str());
    stream << "# t	<mx>    <my>    <mz>    hzee" << std::endl;
    state.calc_mean_m(stream, n_cells);

    timer t_hys = af::timer::start();
    double rate = hzee_max/quater_steps; //[T/s]
    minimizer.llgterms.push_back( LlgTerm (new Zee(&zee_func)));
    while (state.t < 4* hzee_max/rate){
        minimizer.minimize(state);
        state.calc_mean_m(stream, n_cells, afvalue(minimizer.llgterms[3]->h(state)(0,0,0,2)));
        state.t+=1.;
        state.steps++;
        if( state.steps % 1 == 0){
            vti_writer_micro(state.m, mesh ,(filepath + "m_hysteresis_"+std::to_string(state.steps)).c_str());
        }
    }

    stream.close();
    std::cout<<"time full hysteresis [af-s]: "<< af::timer::stop(t_hys) <<std::endl;
    return 0;
}
