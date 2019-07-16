#pragma once
#include "mesh.hpp"
#include "nonequispaced_mesh.hpp"
#include "llg_terms/atomistic_anisotropy.hpp"
#include "llg_terms/atomistic_demag.hpp"
#include "llg_terms/atomistic_dmi.hpp"
#include "llg_terms/atomistic_exchange.hpp"
#include "llg_terms/atomistic_dmi.hpp"
#include "llg_terms/micro_anisotropy.hpp"
#include "llg_terms/micro_demag.hpp"
#include "llg_terms/micro_demag_nonequi.hpp"
#include "llg_terms/micro_dmi.hpp"
#include "llg_terms/micro_exch.hpp"
#include "llg_terms/micro_exch_sparse.hpp"
#include "llg_terms/micro_exch_nonequi.hpp"
#include "llg_terms/micro_dmi.hpp"
#include "llg_terms/zee.hpp"
#include "llg_terms/micro_spintransfertorque.hpp"
#include "string.hpp"
#include "integrators/stochastic_llg.hpp"
#include "integrators/adaptive_runge_kutta.hpp"
#include "integrators/new_llg.hpp"
#include "solvers/minimizer.hpp"
#include "solvers/cg_minimizer.hpp"
#include "solvers/lbfgs_minimizer.hpp"
#include "vtk_IO.hpp"
#include "pgfplot.hpp"
#include "misc.hpp"
#include "util/zero_crossing.hpp"
#include "util/util.hpp"
#include "util/timer.hpp"

namespace magnumaf{

}// namespace magnumaf
