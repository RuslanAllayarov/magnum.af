#include "zee.hpp"

ExternalField::ExternalField(af::array zee_in) : zee_field(zee_in)
{
}


ExternalField::ExternalField(long int aptr) : zee_field(*( new af::array( *((void**) aptr))))
{
}


ExternalField::ExternalField(af::array (*callback_func_in)(State state)): callback_func(callback_func_in), callback(true)
{
}


ExternalField::ExternalField(std::function<af::array(State)> lamda_callback): lamda_callback(lamda_callback), is_lamda(true)
{
}


///< Sets internal af::array to a global field (x, y, z) for all spacial dimensions
void ExternalField::set_homogeneous_field(const double x, const double y, const double z){
    af::dim4 dim = af::dim4(zee_field.dims(0), zee_field.dims(1), zee_field.dims(2), 1);
    zee_field(af::span, af::span, af::span, 0) = af::constant(x, dim, f64);
    zee_field(af::span, af::span, af::span, 1) = af::constant(y, dim, f64);
    zee_field(af::span, af::span, af::span, 2) = af::constant(z, dim, f64);
}


af::array ExternalField::h(const State& state){
    return calc_heff(state);
}


long int ExternalField::h_ptr(const State& state){
    return (long int) (new af::array(calc_heff(state)))->get();
}


af::array ExternalField::calc_heff(const State& state){
    //af::timer timer = af::timer::start();
    //if(state.afsync) sync();
    //af_time += af::timer::stop(timer);
    if (is_lamda) {
        return lamda_callback(state);
    }
    else if(callback){
        return callback_func(state);
    }
    else return zee_field;
}


//Zeeman energy term
double ExternalField::E(const State& state){
    if( state.Ms_field.isempty() ){
        return - constants::mu0 * state.Ms * afvalue(sum(sum(sum(sum(h(state)*state.m, 0), 1), 2), 3)) * state.mesh.dx * state.mesh.dy *state.mesh.dz;
    }
    else{
        return - constants::mu0 * afvalue(sum(sum(sum(sum(state.Ms_field * h(state)*state.m, 0), 1), 2), 3)) * state.mesh.dx * state.mesh.dy *state.mesh.dz;
    }
}


double ExternalField::E(const State& state, const af::array& h){
    if( state.Ms_field.isempty() ){
        return - constants::mu0 * state.Ms * afvalue(sum(sum(sum(sum(h *state.m, 0), 1), 2), 3)) * state.mesh.dx * state.mesh.dy *state.mesh.dz;
    }
    else{
        return - constants::mu0 * afvalue(sum(sum(sum(sum(state.Ms_field * h * state.m, 0), 1), 2), 3)) * state.mesh.dx * state.mesh.dy *state.mesh.dz;
    }
}
