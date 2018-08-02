#include "minimizer.hpp"

//Energy calculation
// only for testing, remove?
double Minimizer::E(const State& state){
  double solution = 0.;
  for(unsigned i=0;i<llgterms.size();++i){
    solution+=llgterms[i]->E(state);
  }
  return solution;
}

Minimizer::Minimizer(std::string scheme, double tau_min, double tau_max, double dm_max, int samples): scheme(scheme), tau_min(tau_min), tau_max(tau_max), dm_max(dm_max), samples(samples)
{
}

// Calculation of effective field
af::array Minimizer::h(const State& state){
    if( llgterms.size() == 0){
        std::cout<<"ERROR: minimizer.cpp: Number of llgterms == 0. Please add at least one term to Minimizer.llgterms! Aborting..."<<std::endl;
        exit (EXIT_FAILURE);
     }
    af::timer timer=af::timer::start();
    af::array solution = llgterms[0]->h(state);
    for(unsigned i = 1; i < llgterms.size() ; ++i ){
        solution+=llgterms[i]->h(state);
    }
    time_h+=af::timer::stop(timer);
    return solution;
}

af::array Minimizer::dm(const State& state){
    return cross4( state.m, cross4( state.m, h(state)));
}

af::array Minimizer::m_next(const State& state, const double tau){

    const af::array Mx = state.m(span,span,span,0);
    const af::array My = state.m(span,span,span,1);
    const af::array Mz = state.m(span,span,span,2);

    const af::array H = h(state);
    const af::array Hx = H(span,span,span,0);
    const af::array Hy = H(span,span,span,1);
    const af::array Hz = H(span,span,span,2);

    const af::array MxH=cross4(state.m, h(state));
    const af::array N = 4 + tau * tau * dotproduct(MxH, MxH);// TODO check whether tau and N are scalars or scalar fields: N_i or N is sum((mhx,mxh)0,1,2,3), i.e. constant for all indices
    const af::array MxH_x = MxH(span,span,span,0);
    const af::array MxH_y = MxH(span,span,span,1);
    const af::array MxH_z = MxH(span,span,span,2);


    af::array result = af::constant(0.,state.mesh.dims,f64);

    result(span,span,span,0)= (4*Mx + 4*tau * (MxH_y*Mz - MxH_z*My) + tau*tau*Mx * (  MxH_x*MxH_x - MxH_y*MxH_y - MxH_z*MxH_z) + 2*tau*tau*MxH_x * (MxH_y*My + MxH_z*Mz)) / N;
    result(span,span,span,1)= (4*My + 4*tau * (MxH_z*Mx - MxH_x*Mz) + tau*tau*My * (- MxH_x*MxH_x + MxH_y*MxH_y - MxH_z*MxH_z) + 2*tau*tau*MxH_y * (MxH_z*Mz + MxH_x*Mx)) / N;
    result(span,span,span,2)= (4*Mz + 4*tau * (MxH_x*My - MxH_y*Mx) + tau*tau*Mz * (- MxH_x*MxH_x - MxH_y*MxH_y + MxH_z*MxH_z) + 2*tau*tau*MxH_z * (MxH_x*Mx + MxH_y*My)) / N;

    return result;
}

void Minimizer::minimize(State& state){
    double tau = tau_min;
    unsigned long int step = 0;
    double dm_max = 1e18;
    std::list<double> last_dm_max;
    af::timer timer = af::timer::start();

    while (last_dm_max.size() < samples || *std::max_element(std::begin(last_dm_max), std::end(last_dm_max)) > dm_max){
        af::array m_next = this->m_next(state, tau);
        af::array dm = this->dm(state);

        // Calculate s^n-1 for step-size control
        af::array m_diff = m_next - state.m;

        // Update state
        state.m = m_next;
        //state.m = renormalize(state.m);// TODO check performance /w /wo

        // Calculate y^n-1 for step-size control
        af::array dm_next = this->dm(state);
        af::array dm_diff = dm_next - dm;

        // Compute dm_max for convergence estimation
        dm_max = maxnorm(dm_next);
        last_dm_max.push_back(dm_max);
        if(last_dm_max.size() > samples) last_dm_max.pop_front();

        // Next stepsize alternating tau1 and tau2

        if (step % 2 == 0){
            tau = full_inner_product(m_diff, m_diff)/full_inner_product(m_diff, dm_diff); 
        }
        else {
            tau = full_inner_product(m_diff, dm_diff)/full_inner_product(dm_diff, dm_diff); 
        }
        // TODO handly zero division

        if(tau >= 0) tau = std::max(std::min(fabs(tau),tau_max),tau_min);
        else tau = - std::max(std::min(fabs(tau),tau_max),tau_min);
        // Increase step count
        step ++;
        std::cout << "step "<< step << " tau= "<< tau << " last_dm_max.size()= "<< last_dm_max.size()<< " dm_max= " << dm_max <<"*std::max_element()"<< *std::max_element(std::begin(last_dm_max), std::end(last_dm_max)) << std::endl;    
        //std::cout << "step "<< step << " Energy= "<<E(state) << " tau= "<< tau << " last_dm_max.size()= "<< last_dm_max.size()<< " dm_max= " << dm_max <<"*std::max_element()"<< *std::max_element(std::begin(last_dm_max), std::end(last_dm_max)) << std::endl;    
    }
    std::cout << "Minimizer: time = " << af::timer::stop(timer) << std::endl;
}; 
