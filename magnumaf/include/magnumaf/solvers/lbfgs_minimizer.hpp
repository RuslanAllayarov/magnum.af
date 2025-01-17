#pragma once
#include "field_terms/field_term.hpp"
#include <fstream>
#include <utility>

namespace magnumaf {

// For second Method, use interface class:
// https://stackoverflow.com/questions/40624175/c-how-to-implement-a-switch-between-class-members
//
// typedef double Dtype;// TODO: replace ?
// NOTE: Dtype ak in Schrefl::linesearch is moved into cvsrch

class LBFGS_Minimizer {
  public:
    LBFGS_Minimizer(double tolerance = 1e-6, size_t maxIter = 230, int verbose = 4);
    LBFGS_Minimizer(vec_uptr_FieldTerm llgterms, double tolerance_ = 1e-6, size_t maxIter_ = 230, int verbose = 4);

    double Minimize(State&) const;

    vec_uptr_FieldTerm fieldterms_{}; // default init, as not constructed in init list

    mutable std::ofstream of_convergence_; // stream to write additional convergence data to

  private:
    double linesearch(State& state, double& fval, const af::array& x_old, af::array& g, const af::array& searchDir,
                      const double tolf) const;
    int cvsrch(State& state, const af::array& wa, double& f, af::array& g, double& stp, const af::array& s,
               const double tolf) const;
    int cstep(double& stx, double& fx, double& dx, double& sty, double& fy, double& dy, double& stp, double& fp,
              double& dp, bool& brackt, double& stpmin, double& stpmax, int& info) const;

    const double tolerance_; ///< Error tolerance with default 1e-6
    const size_t maxIter_;   ///< Maximum number of iterations
    const int verbose_;      ///< Setting output options, valid values are 0, 1, 2, 3, 4
};

} // namespace magnumaf
