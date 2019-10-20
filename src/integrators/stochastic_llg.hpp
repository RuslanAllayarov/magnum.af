#pragma once
#include "../state.hpp"
#include "stochastic_integrator.hpp"
#include "arrayfire.h"

namespace magnumaf{


class Stochastic_LLG : public Stochastic_Integrator {
    public:
        Stochastic_LLG(float alpha, float T, float dt, State state, std::vector<std::shared_ptr<LLGTerm>> terms, std::string smode): Stochastic_Integrator (alpha, T, dt, state, terms, smode){}
        float E(const State& state); //Energy calculation
    private:
        af::array fheff(const State&);
        af::array detfdmdt(const State&);//only for reference in detRK4
        af::array stochfdmdt(const State&, const af::array& h_th);
};

}// namespace magnumaf
