#ifndef Constants_H
#define Constants_H
#include<math.h>

namespace constants{
    // The following values are obtained from CODATA/NIST as of 04.03.2019:

    const double mu0 = 4e-7 * M_PI; ///< [H/m] magnetic constant mu_0
  
    const double gamma = 1.760859644e11 * mu0; ///< [m A^-1 s^-1] gyromagnetic ratio gamma. Electron gyromagnetic ratio gamma_e [s^-1 T^-1] times mu_0 [H/m], where [T/H] is [A/m^2]
  
    const double mu_b = 9.274009994e-24; ///< [J/T] Bohr magneton mu_bohr
  
    const double e = - 1.6021766208e-19; ///< [C] Elementary charge e (including minus sign)
  
    const double kb = 1.38064852e-23; ///< [J/K] Boltzmann constant kb
  
    const double hbar = 1.0545718e-34; ///< [J s] Reduced Planck constant
}
#endif