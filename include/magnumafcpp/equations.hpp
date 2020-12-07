#pragma once
#include "arrayfire.h"
#include "constants.hpp"
#include "util/func.hpp" // for cross4
namespace magnumafcpp::equations {

/// LLG precession term
inline af::array LLG_precession(double alpha, const af::array& m_x_h) {
    return -constants::gamma / (1. + std::pow(alpha, 2)) * m_x_h;
}

/// LLG damping term
inline af::array LLG_damping(double alpha, const af::array& m, const af::array& m_x_h) {
    return -alpha * constants::gamma / (1. + std::pow(alpha, 2)) * cross4(m, m_x_h);
}

/// LLG equation
inline af::array LLG(double alpha, const af::array& m, const af::array& h_eff) {
    const af::array m_x_h = cross4(m, h_eff);
    return LLG_precession(alpha, m_x_h) + LLG_damping(alpha, m, m_x_h);
}

} // namespace magnumafcpp::equations
