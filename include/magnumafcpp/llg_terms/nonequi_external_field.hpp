#pragma once
#include "arrayfire.h"
#include "nonequi_term_base.hpp"
#include "nonequispaced_mesh.hpp"
#include "state.hpp"
#include <functional>

namespace magnumafcpp {

class NonequiExternalField : public NonequiTermBase {
  public:
    NonequiExternalField(NonequispacedMesh nemesh, const af::array& field)
        : nemesh(nemesh), external_field(field) {}

    NonequiExternalField(NonequispacedMesh nemesh,
                         std::function<af::array(State)> function)
        : nemesh(nemesh), callback_function(function),
          callback_is_defined(true) {}

    ///< For wrapping only
    NonequiExternalField(NonequispacedMesh nemesh, long int fieldptr)
        : nemesh(nemesh),
          external_field(*(new af::array(*((void**)fieldptr)))) {}

    af::array h(const State& state) {
        if (callback_is_defined) {
            return callback_function(state);
        } else {
            return external_field;
        }
    }

    // Energy contribution differs by factor of 2 compared to terms linear in m
    double E(const State& state) { return 2. * NonequiTermBase::E(state); };
    double E(const State& state, const af::array& h) {
        return 2. * NonequiTermBase::E(state, h);
    };

    double get_cpu_time() { return 0; } // use or remove

  private:
    NonequispacedMesh nemesh;
    af::array external_field;
    std::function<af::array(State)> callback_function;
    const bool callback_is_defined{false};
};

} // namespace magnumafcpp
