#include "arrayfire.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <string>
#include <memory>
#include "llg.hpp"
#include "micro_demag.hpp"
#include "micro_exch.hpp"
#include "zee.hpp"
#include "atomistic_demag.hpp"
#include "atomistic_exchange.hpp"
#include "atomistic_anisotropy.hpp"
#include "atomistic_dmi.hpp"
#include "vtk_IO.hpp"
#include "string.hpp"
using namespace af; typedef std::shared_ptr<LLGTerm> llgt_ptr; 
int main(int argc, char** argv)
{
    if(argc>1) setDevice(std::stoi(argv[2]));
    info();
  
    std::cout<<"argc"<<argc<<std::endl;
     for (int i=0; i<argc; i++)
          cout << "Parameter " << i << " was " << argv[i] << "\n";
    //char* charptr;
    //std::cout<<"argv"<<std::strtod(argv[1],&charptr)<<std::endl;
    //std::cout<<"argv"<<std::strtod(argv[2],&charptr)<<std::endl;
    
    // Parameter initialization
    const int nx = 112, ny=112 ,nz=4;//nz=5 -> lz=(5-1)*dx
    const double dx=2.715e-10;
  
  
    //Simulation Parameters
    //double hmax = 3.5e-10;
    //double hmin = 1.0e-15;
    //double atol = 1e-6;
    //double rtol = atol;
    
    double n_interp = 60;
    double string_dt=5e-13;
    const int string_steps = 10000;
    std::string filepath(argc>0? argv[1]: "../Data/Testing/");
    if(argc>0)filepath.append("/");
    //else std::string filepath("../Data/Testing/");
    std::cout<<"Writing into path "<<filepath.c_str()<<std::endl;
  
  
    //Generating Objects
    Mesh mesh(nx,ny,nz,dx,dx,dx);
    Param param = Param();
    param.gamma = 2.211e5;
    param.ms    = 1.1e6;
    param.A     = 1.6e-11;
    param.alpha = 1;
    param.afsync  = false;
  
    //param.D=2*5.76e-3;
    param.D=std::stod(argv[3]);
    std::cout<<"D="<<param.D<<std::endl;
    //param.D_axis[2]=-1;
  
    //param.Ku1=6.4e6;
    param.Ku1=std::stod(argv[4]);
    std::cout<<"Ku1="<<param.Ku1<<std::endl;
    //param.Ku1_axis[2]=1;
  
    param.J_atom=2.*param.A*dx;
    param.D_atom= param.D * pow(dx,2)/2.;
    //old values in prev versions, this is now wrong in pthmag: 
    const double wrong_J_atom=4.*param.A*dx;
    const double wrong_D_atom= 2.* param.D * pow(dx,2);
    std::cout<<"D_atom="<<param.D_atom<<std::endl;
    param.K_atom=param.Ku1*pow(dx,3);
    std::cout<<"Ku1_atom="<<param.K_atom<<std::endl;
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
    //llgterm.push_back( llgt_ptr (new DemagSolver(mesh,param)));
    //llgterm.push_back( llgt_ptr (new ExchSolver(mesh,param)));
    //llgterm.push_back( llgt_ptr (new DMI(mesh,param)));
    //llgterm.push_back( llgt_ptr (new ANISOTROPY(mesh,param)));
    
    llgterm.push_back( llgt_ptr (new ATOMISTIC_DEMAG(mesh)));
    llgterm.push_back( llgt_ptr (new ATOMISTIC_EXCHANGE(mesh)));
    llgterm.push_back( llgt_ptr (new ATOMISTIC_DMI(mesh,param)));
    llgterm.push_back( llgt_ptr (new ATOMISTIC_ANISOTROPY(mesh,param)));
  
    LLG Llg(state,llgterm);
  
    timer t = af::timer::start();
    while (state.t < 8.e-10){
        state.m=Llg.llgstep(state);
    }
    double timerelax= af::timer::stop(t);
    vti_writer_atom(state.m, mesh ,(filepath + "relax").c_str());
  
    std::cout<<"timerelax [af-s]: "<< timerelax << " for "<<Llg.counter_accepted+Llg.counter_reject<<" steps, thereof "<< Llg.counter_accepted << " Steps accepted, "<< Llg.counter_reject<< " Steps rejected" << std::endl;
  
  
  
  
    array last   = constant( 0,mesh.dims,f64);
    last(span,span,span,2)=1;
    
    std::vector<State> inputimages; 
    inputimages.push_back(state);
    inputimages.push_back(State(mesh,param, last));
  
    String string(state,inputimages, n_interp, string_dt ,llgterm);
    //String* string = new String(state,inputimages, n_interp ,llgterm);
    std::cout.precision(12);
  
    std::ofstream stream_E_barrier;
    stream_E_barrier.precision(12);
  
    std::ofstream stream_steps;
    stream_steps.precision(12);
    stream_steps.open ((filepath + "steps.dat").c_str());
  
    std::ofstream stream_E_curves;
    stream_E_curves.precision(12);
    stream_E_curves.open ((filepath + "E_curves.dat").c_str());
  
    double max_lowest=1e100;
    double max_prev_step=1e100;
    int i_max_lowest=-1;
    std::vector<State> images_max_lowest; 
    std::vector<double> E_max_lowest;
    for(int i=0; i<string_steps;i++){
        af::printMemInfo();
        string.step();
        string.calc_E();
        auto max = std::max_element(std::begin(string.E), std::end(string.E));
        if(*max-string.E[0]<max_lowest) {
            max_lowest=*max-string.E[0];
            i_max_lowest=i;
            images_max_lowest=string.images;
            E_max_lowest=string.E;
        }    
        //Wrong approach
        //else if(i>50){
        //  std::cout   << "Exiting loop: Energy barrier after 50step relaxation becomes bigger "<<std::endl;
        //  stream_steps<<"#Exiting loop: Energy barrier after 50step relaxation becomes bigger "<<std::endl;
        //  break;
        //}
        
        //std::cout<<"Test: fabs= "<<fabs(2*(*max-string.E[0]-max_prev_step)/(*max-string.E[0]+max_prev_step))<<std::endl;
        
        if(i>25 && fabs(2*(*max-string.E[0]-max_prev_step)/(*max-string.E[0]+max_prev_step))<1e-6){
            std::cout   <<      "Exiting loop: Energy barrier relative difference smaller than 1e-6"<<std::endl;
            stream_steps<<     "#Exiting loop: Energy barrier relative difference smaller than 1e-6"<<std::endl;
            break;
        }
        if(i>25 && fabs(*max-string.E[0]-max_prev_step)<1e-27){
            std::cout   <<      "Exiting loop: Energy barrier difference smaller than 1e-27"<<std::endl;
            stream_steps<<     "#Exiting loop: Energy barrier difference smaller than 1e-27"<<std::endl;
            break;
        }
        std::cout   <<i<<"\t"<<*max-string.E[0]<<"\t"<<string.E[0]<<"\t"<<*max-string.E[-1]<< "\t"<<*max<<"\t"<<fabs(2*(*max-string.E[0]-max_prev_step)/(*max-string.E[0]+max_prev_step))<<std::endl;
        stream_steps<<i<<"\t"<<*max-string.E[0]<<"\t"<<string.E[0]<<"\t"<<*max-string.E[-1]<< "\t"<<*max<<"\t"<<fabs(2*(*max-string.E[0]-max_prev_step)/(*max-string.E[0]+max_prev_step))<<std::endl;
        stream_E_barrier.open ((filepath + "E_barrier.dat").c_str());
        stream_E_barrier<<max_lowest<<"\t"<<nx<<"\t"<<dx<<"\t"<<param.D<<"\t"<<param.Ku1<<"\t"<<wrong_D_atom<<"\t"<<param.K_atom<<"\t"<<param.D_atom<<std::endl;
        stream_E_barrier.close();
        for(unsigned j=0;j<string.E.size();++j)
        {
            stream_E_curves<<i<<" "<<j<<" "<<string.E[j]-string.E[0]<<" "<<string.E[j]-string.E[-1]<<" "<<string.E[j]<<std::endl;
        }
        stream_E_curves<<i<<"\n\n"<<std::endl;
        max_prev_step=*max-string.E[0];
        if(i%20==1){ 
            std::cout<<"Writing current skyrm images for iteration"<<i<<std::endl;
            for(unsigned j = 0; j < string.images.size(); j++){
                std::string name = filepath;
                name.append("current_skyrm_image");
                name.append(std::to_string(j));
                vti_writer_atom(string.images[j].m, mesh ,name.c_str());
            }
        }
    }
    std::cout   <<"#i ,lowest overall:   max-[0], max-[-1] max [J]: "<<i_max_lowest<<"\t"<<max_lowest<<"\t"<<max_lowest+E_max_lowest[0]-E_max_lowest[-1]<<"\t"<<max_lowest+E_max_lowest[0]<< std::endl;
    stream_steps<<"#i ,lowest overall:   max-[0], max-[-1] max [J]: "<<i_max_lowest<<"\t"<<max_lowest<<"\t"<<max_lowest+E_max_lowest[0]-E_max_lowest[-1]<<"\t"<<max_lowest+E_max_lowest[0]<< std::endl;
    stream_E_barrier.open ((filepath + "E_barrier.dat").c_str());
    stream_E_barrier<<max_lowest<<"\t"<<nx<<"\t"<<dx<<"\t"<<param.D<<"\t"<<param.Ku1<<"\t"<<wrong_D_atom<<"\t"<<param.K_atom<<"\t"<<param.D_atom<<std::endl;
    stream_E_barrier.close();
    //OLD: stream_E_barrier<<max_lowest<<"\t"<<nx<<"\t"<<dx<<"\t"<<param.D<<"\t"<<param.Ku1<<"\t"<<param.D_atom<<"\t"<<param.K_atom<<"\t"<<std::endl;
  
    std::ofstream myfileE;
    myfileE.precision(12);
    myfileE.open ((filepath + "E_last_step.dat").c_str());
  
    std::ofstream stream_max_lowest;
    stream_max_lowest.precision(12);
    stream_max_lowest.open ((filepath + "E_max_lowest.dat").c_str());
  
    std::cout<<string.E.size()<<"\t"<<string.images.size()<< "\t" <<std::endl;
    for(unsigned i = 0; i < string.images.size(); i++){
      std::cout<<"i="<<i<< "\t" << "E= "<<string.E[i]<<std::endl;
      myfileE<<i<< "\t" << string.E[i]<< "\t" << string.E[i]-string.E[0]<< "\t" << string.E[i]-string.E[-1]<<std::endl;
      std::string name = filepath;
      name.append("skyrm_image");
      name.append(std::to_string(i));
      vti_writer_atom(string.images[i].m, mesh ,name.c_str());
      //af_to_vtk(string.images_interp[i],name.c_str());
      stream_max_lowest<<i<< "\t" << E_max_lowest[i]<<"\t" << E_max_lowest[i]-E_max_lowest[0]<<"\t" << E_max_lowest[i]-E_max_lowest[-1]<<std::endl;
      name = filepath;
      name.append("skyrm_image_max_lowest");
      name.append(std::to_string(i));
      vti_writer_atom(images_max_lowest[i].m, mesh ,name.c_str());
    }
  
    for(unsigned i=0;i<Llg.Fieldterms.size();++i){
      std::cout<<"get_cpu_time()"<<std::endl;
      std::cout<<i<<"\t"<<Llg.cpu_time()<<std::endl;
      stream_steps<<"#"<<"get_cpu_time()"<<std::endl;
      stream_steps<<"#"<<i<<"\t"<<Llg.cpu_time()<<std::endl;
    }
  
  
    myfileE.close();
    stream_steps.close();
    stream_E_curves.close();
    stream_max_lowest.close();
    //delete[] string;
  
    return 0;
}
