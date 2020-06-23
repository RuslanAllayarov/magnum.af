#pragma once
#include "integrators/new_llg.hpp"
#include "state.hpp"
#include <algorithm>
#include <vector>

namespace magnumafcpp {

class String {
  public:
    String(State state, std::vector<State> inputimages, int n_interp, double dt,
           LLGIntegrator llg);
    ///
    /// Runs the string method.
    /// This populates files in \param filepath.
    ///
    double run(const std::string filepath,
               const double string_abort_rel_diff = 1e-12,
               const double string_abort_abs_diff = 1e-27,
               const int string_steps = 10000,
               const int every_string_to_vti = 50, const bool verbose = true);

    LLGIntegrator Llg; //(state_relax, atol, rtol, hmax, hmin);
    const int n_interp;
    const double dt;
    double time{0};
    std::vector<double> x{}; // Current x values
    std::vector<double>
        x_interp{}; // x-values where to interpolate at (will be regular grid)
    std::vector<double> E{};   // Energy values
    std::vector<State> images{}; // Current images
    void calc_E();
    void calc_x();
    void calc_x(std::vector<State>);
    void lin_interpolate();
    void integrate(); // Integrate all images for dt
    void step();
    void vec_renormalize();
    void write_vti(std::string file);

  private:
};

} // namespace magnumafcpp