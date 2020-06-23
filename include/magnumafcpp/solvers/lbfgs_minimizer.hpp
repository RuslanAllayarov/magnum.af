#pragma once
#include "../llg_terms/LLGTerm.hpp"
#include <fstream>

namespace magnumafcpp {

// For second Method, use interface class:
// https://stackoverflow.com/questions/40624175/c-how-to-implement-a-switch-between-class-members
//
// typedef double Dtype;// TODO: replace ?
// NOTE: Dtype ak in Schrefl::linesearch is moved into cvsrch

class LBFGS_Minimizer {
  public:
    LBFGS_Minimizer(double tolerance_ = 1e-6, size_t maxIter_ = 230,
                    int verbose = 4);
    LBFGS_Minimizer(LlgTerms llgterms, double tolerance_ = 1e-6,
                    size_t maxIter_ = 230, int verbose = 4);

    double Minimize(State&);

    LlgTerms llgterms_{}; // default init, as not constructed in init list

    double GetTimeCalcHeff() const {
        return time_calc_heff_;
    }; ///< Accumulated time for calculation of Heff.
    std::ofstream of_convergence{}; // optional output stream

  private:
    af::array Gradient(const State&); ///< Calculate gradient as
                                      ///< energy-dissipation term of llg
    af::array Heff(const State& m);   ///< Effective Field
    double E(const State&);           ///< Calculate Energy
    double EnergyAndGradient(const State& state, af::array& gradient);
    double time_calc_heff_{0}; ///< Timer measuring calls to effective field _h
    const double tolerance_;   ///< Error tolerance with default 1e-6
    const size_t maxIter_;     ///< Maximum number of iterations
    const int
        verbose; ///< Setting output options, valid values are 0, 1, 2, 3, 4
    double mxmxhMax(
        const State& state); ///< TODO investigate definition, init value etc
    double linesearch(State& state, double& fval, const af::array& x_old,
                      af::array& g, const af::array& searchDir,
                      const double tolf);
    // TODO//TODEL//int cvsrch(const State& state, const af::array &wa,
    // af::array &x, double &f, af::array &g, const af::array &s, double tolf);
    int cvsrch(State& state, const af::array& wa, double& f, af::array& g,
               double& stp, const af::array& s, const double tolf);
    int cstep(double& stx, double& fx, double& dx, double& sty, double& fy,
              double& dy, double& stp, double& fp, double& dp, bool& brackt,
              double& stpmin, double& stpmax, int& info);
};

} // namespace magnumafcpp