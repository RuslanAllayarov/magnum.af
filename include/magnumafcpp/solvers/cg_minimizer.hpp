#pragma once
#include "arrayfire.h"
#include "field_terms/field_term.hpp"
#include "state.hpp"
#include <utility>

namespace magnumafcpp {

// For second Method, use interface class:
// https://stackoverflow.com/questions/40624175/c-how-to-implement-a-switch-between-class-members

//! Preconditioned non-linear conjugate gradient minimizer.

//! Preconditioned Non-Linear Conjugate Gradient Minimizer (courtesy of Thomas
//! Schrefl et.al.)
//!
class CG_Minimizer {
  public:
    vec_uptr_FieldTerm fieldterms{};
    void Minimize(State&) const;                                // Minimization routine
    double GetTimeCalcHeff() const { return time_calc_heff_; }; ///< Accumulated time for calculation of Heff.
  private:
    af::array Heff(const State& m) const; ///< Effective Field
    std::pair<double, af::array> EnergyAndGradient(const State& state) const;
    mutable double time_calc_heff_{0}; ///< Timer measuring calls to effective field _h
};

} // namespace magnumafcpp
