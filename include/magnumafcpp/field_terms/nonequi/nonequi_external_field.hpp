#pragma once
#include "arrayfire.h"
#include "field_terms/nonequi/nonequi_term.hpp"
#include "nonequispaced_mesh.hpp"
#include "state.hpp"
#include <functional>

namespace magnumafcpp {

class NonequiExternalField : public NonequiTerm {
  public:
    NonequiExternalField(NonequiMesh nemesh, const af::array& field) : NonequiTerm(nemesh), external_field(field) {}

    NonequiExternalField(NonequiMesh nemesh, std::function<af::array(State)> function)
        : NonequiTerm(nemesh), callback_function(function), callback_is_defined(true) {}

    ///< For wrapping only
    NonequiExternalField(NonequiMesh nemesh, long int fieldptr)
        : NonequiTerm(nemesh), external_field(*(new af::array(*((void**)fieldptr)))) {}

    virtual af::array h(const State& state) const override {
        if (callback_is_defined) {
            return callback_function(state);
        } else {
            return external_field;
        }
    }

    // Energy contribution differs by factor of 2 compared to terms linear in m
    using NonequiTerm::E;
    virtual double E(const State& state, const af::array& h) const override { return 2. * NonequiTerm::E(state, h); };

  private:
    af::array external_field;
    std::function<af::array(State)> callback_function;
    bool callback_is_defined{false};
};

} // namespace magnumafcpp
