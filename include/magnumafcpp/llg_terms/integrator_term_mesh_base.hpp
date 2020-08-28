#pragma once
#include "../constants.hpp"
#include "../state.hpp"
#include "arrayfire.h"
// TODO #include "llg_term_interface.hpp"
#include "LLGTerm.hpp"
#include <memory>

namespace magnumafcpp {

class IntegratorTermMeshBase : public LLGTerm {
  public:
    /// Calculating the micromagnetic energy \f$E\f$.
    /// This is a prototype for all llgterms with are linear in m and must be
    /// overwritten in e.g. zeeman where factor 1/2 becomes 1.
    virtual double E(const State& state) {
        if (state.Ms_field.isempty()) {
            return -constants::mu0 / 2. * state.Ms *
                   af::sum(af::sum(af::sum(af::sum(h(state) * state.m, 0), 1), 2), 3).as(f64).scalar<double>() *
                   state.mesh.dx * state.mesh.dy * state.mesh.dz;
        } else {
            return -constants::mu0 / 2. *
                   af::sum(af::sum(af::sum(af::sum(state.Ms_field * h(state) * state.m, 0), 1), 2), 3)
                       .as(f64)
                       .scalar<double>() *
                   state.mesh.dx * state.mesh.dy * state.mesh.dz;
        }
    }
    ///< Calculating the micromagnetic energy for a already calculated h field
    ///< (to save computational cost)
    virtual double E(const State& state, const af::array& h) {
        if (state.Ms_field.isempty()) {
            return -constants::mu0 / 2. * state.Ms *
                   af::sum(af::sum(af::sum(af::sum(h * state.m, 0), 1), 2), 3).as(f64).scalar<double>() *
                   state.mesh.dx * state.mesh.dy * state.mesh.dz;
        } else {
            return -constants::mu0 / 2. *
                   af::sum(af::sum(af::sum(af::sum(state.Ms_field * h * state.m, 0), 1), 2), 3)
                       .as(f64)
                       .scalar<double>() *
                   state.mesh.dx * state.mesh.dy * state.mesh.dz;
        }
    }

    virtual ~IntegratorTermMeshBase(){};
};

} // namespace magnumafcpp
