#include "micro_dampinglike.hpp"

double DampinglikeTorque::E(const State& state){
    return - state.material.mu0/2. * state.material.ms * afvalue(sum(sum(sum(sum(h(state)*state.m,0),1),2),3)) * state.mesh.dx * state.mesh.dy * state.mesh.dz; 
}

double DampinglikeTorque::E(const State& state, const af::array& h){
    return -state.material.mu0/2. * state.material.ms * afvalue(sum(sum(sum(sum(h * state.m,0),1),2),3)) * state.mesh.dx * state.mesh.dy * state.mesh.dz; 
}


DampinglikeTorque::DampinglikeTorque (af::array polarization_field, double nu_damp, double j_e) : polarization_field(polarization_field), nu_damp(nu_damp), j_e(j_e) {
}

af::array DampinglikeTorque::h(const State& state){
    //af::timer timer_fieldlike = af::timer::start();
    //evaluation_timing += af::timer::stop(timer_fieldlike);
    return nu_damp * j_e * constants::gamma * constants::hbar / (2. * constants::e * constants::mu0 * state.material.ms) * cross4(state.m, cross4(state.m, polarization_field));
}
