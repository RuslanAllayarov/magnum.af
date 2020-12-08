#pragma once
#include "arrayfire.h"
#include "field_terms/atom/atom_term.hpp"
#include "state.hpp"

namespace magnumafcpp {

class AtomisticDipoleDipoleField : public AtomTerm {
  public:
    virtual af::array h(const State& state) const override;
    explicit AtomisticDipoleDipoleField(Mesh);

  private:
    af::array Nfft;
};
} // namespace magnumafcpp
